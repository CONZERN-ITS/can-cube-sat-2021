#include <sx126x_board.h>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include <string.h>
#include <stdbool.h>

#include <gpiod.h>

#define SX126X_RPI_GPIO_CONSUMER_PREFIX "sx126x_svc_"

#define SX126X_RPI_GPIO_CHIP_PATH "/dev/gpiochip0"
#define SX126X_RPI_GPIO_RXEN 27
#define SX126X_RPI_GPIO_TXEN 22
#define SX126X_RPI_GPIO_NRST 23
#define SX126X_RPI_GPIO_BUSY 24
#define SX126X_RPI_GPIO_DIO1 25


#define SX126X_RPI_SPI_DEVICE_PATH "/dev/spidev0.0"
#define SX126X_RPI_SPI_MAX_SPEED (10*1000*1000)


struct sx126x_board_t
{
	//! Дескриптор SPI шины
	int spidev_fd;
	//! Время начала нашей работы (для get_time в монотонных миллисекундах)
	struct timespec start_time;

	//! Чип GPIO на котором сидят нужные нам пины
	struct gpiod_chip * gpio_chip;

	//! Отдельные пины, с которыми мы работаем
	/*! Конкретные назначения пинов согласно sx126x_rpi_gpio_lineid_t */
	struct gpiod_line * line_nrst;
	struct gpiod_line * line_rxen;
	struct gpiod_line * line_txen;
	struct gpiod_line * line_busy;
	struct gpiod_line * line_dio1;
};


static sx126x_board_t _dev;


static void _gpio_deinit(sx126x_board_t * dev)
{
	struct gpiod_line ** lines[] = {
			&dev->line_nrst,
			&dev->line_rxen,
			&dev->line_txen,
			&dev->line_busy,
			&dev->line_dio1,
	};

	for (size_t i = 0; i < sizeof(lines)/sizeof(*lines); i++)
	{
		struct gpiod_line ** line = lines[i];
		if (line && !gpiod_line_is_free(*line))
			gpiod_line_release(*line);

		*line = NULL;
	}

	gpiod_chip_close(dev->gpio_chip);
	dev->gpio_chip = NULL;
}


static int _gpio_init(sx126x_board_t * dev)
{
	dev->gpio_chip = gpiod_chip_open(SX126X_RPI_GPIO_CHIP_PATH);
	if (NULL == dev->gpio_chip)
		return -ENOENT;

	// Окей, девайс открыли, запрашиваем линии
	struct gpiod_line_bulk line_bulk;
	gpiod_line_bulk_init(&line_bulk);
	unsigned int line_offsets[] = {
			SX126X_RPI_GPIO_NRST,
			SX126X_RPI_GPIO_RXEN,
			SX126X_RPI_GPIO_TXEN,
			SX126X_RPI_GPIO_BUSY,
			SX126X_RPI_GPIO_DIO1
	};
	unsigned int line_count = sizeof(line_offsets);
	int rc = gpiod_chip_get_lines(dev->gpio_chip, line_offsets, line_count, &line_bulk);
	if (rc < 0)
		goto bad_exit;

	// вытаскиваем линии в дескриптор
	dev->line_nrst = gpiod_line_bulk_get_line(&line_bulk, 0);
	dev->line_rxen = gpiod_line_bulk_get_line(&line_bulk, 1);
	dev->line_txen = gpiod_line_bulk_get_line(&line_bulk, 2);
	dev->line_busy = gpiod_line_bulk_get_line(&line_bulk, 3);
	dev->line_dio1 = gpiod_line_bulk_get_line(&line_bulk, 4);

	// Теперь запрашиваем их все для себя
	rc = gpiod_line_request_output(dev->line_txen, SX126X_RPI_GPIO_CONSUMER_PREFIX "nrst", 1);
	if (rc < 0) goto bad_exit;

	rc = gpiod_line_request_output(dev->line_rxen, SX126X_RPI_GPIO_CONSUMER_PREFIX "rxen", 0);
	if (rc < 0) goto bad_exit;

	rc = gpiod_line_request_output(dev->line_txen, SX126X_RPI_GPIO_CONSUMER_PREFIX "txen", 0);
	if (rc < 0) goto bad_exit;

	// С аутпутами закончили... теперь инпуты и прерывания
	rc = gpiod_line_request_input(dev->line_busy, SX126X_RPI_GPIO_CONSUMER_PREFIX "busy");
	if (rc < 0) goto bad_exit;

	rc = gpiod_line_request_rising_edge_events(
			dev->line_dio1,
			SX126X_RPI_GPIO_CONSUMER_PREFIX "dio1"
	);
	if (rc < 0) goto bad_exit;

	// Кажется, тут у нас все
	return 0;

bad_exit:
	_gpio_deinit(dev);
	return -1;
}


static void _spi_deinit(sx126x_board_t * dev)
{
	if (dev->spidev_fd)
	{
		close(dev->spidev_fd);
		dev->spidev_fd = 0;
	}
}


static int _spi_init(sx126x_board_t * dev)
{
	dev->spidev_fd = open(SX126X_RPI_SPI_DEVICE_PATH, O_RDWR);
	if (dev->spidev_fd < 0)
		return errno;

	// Выставляем максимальную скорость
	const uint32_t speed = SX126X_RPI_SPI_MAX_SPEED;
	int rc = ioctl(dev->spidev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (rc < 0)
		goto bad_exit;

	rc = ioctl(dev->spidev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (rc < 0)
		goto bad_exit;

	// Кажется, все готово
	return 0;

bad_exit:
	_spi_deinit(dev);
	return -1;
}



int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg)
{
	(void)user_arg;

	memset(&_dev, 0x00, sizeof(_dev));

	// Настраиваем SPI
	int rc = _spi_init(&_dev);
	if (rc != 0)
		return rc;

	// Настраиваем GPIO
	rc = _gpio_init(&_dev);

	// Настраиваем время
	rc = clock_gettime(CLOCK_MONOTONIC, &_dev.start_time);
	if (rc < 0)
		return rc;

	// Кажется, у нас все хорошо?
	*brd = &_dev;
	return 0;
}


void sx126x_brd_dtor(sx126x_board_t * brd)
{
	if (!brd)
		return;

	if (brd != &_dev)
		return; // ЭТО НЕ НАШИ

	_spi_deinit(brd);
	_gpio_deinit(brd);
}


uint32_t sx126x_brd_get_time(sx126x_board_t * brd)
{
	struct timespec current_global_time;
	int rc = clock_gettime(CLOCK_MONOTONIC, &current_global_time);
	if (rc < 0)
		return 0; // Вот тут честно говоря чуток не продумано. FIXME

	const time_t seconds_delta = current_global_time.tv_sec - brd->start_time.tv_sec;
	const long nseconds_delta = current_global_time.tv_nsec - brd->start_time.tv_nsec;

	// Пересчитываем в миллисекунды
	return seconds_delta * 1000 + nseconds_delta / (1000 * 1000);
}


int sx126x_brd_get_chip_type(sx126x_board_t * brd, sx126x_chip_type_t * chip_type)
{
	// У нас используется именно такой
	*chip_type = SX126X_CHIPTYPE_SX1268;
	return 0;
}


int sx126x_brd_reset(sx126x_board_t * brd)
{
	// Опускаем ресет линию и чуточку ждем
	int rc = gpiod_line_set_value(brd->line_nrst, 0);
	if (rc < 0)
		return rc;

	// Чуточку поспим
	usleep(50);

	// Включаем линию сброса обратноs
	rc = gpiod_line_set_value(brd->line_nrst, 1);
	if (rc < 0)
		return rc;

	return 0;
}


int sx126x_brd_wait_on_busy(sx126x_board_t * brd)
{
	int rc;
	do
	{
		int rc = gpiod_line_get_value(brd->line_busy);
		if (rc < 0)
			return rc;

	} while(rc != 0);

	return 0;
}


void sx126x_brd_enable_irq(sx126x_board_t * brd)
{
	// Ничего не делаем. Эти функции вообще убрать бы
}


void sx126x_brd_disable_irq(sx126x_board_t * brd)
{
	// Снова ничего не делаем. Эти функции убрать бы!
}


int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode)
{
	int rc;

	// Глушим оба пина
	rc = gpiod_line_set_value(brd->line_rxen, 0);
	if (rc < 0)
		return rc;

	rc = gpiod_line_set_value(brd->line_txen, 0);
	if (rc < 0)
		return rc;

	switch (mode)
	{
	case SX126X_ANTENNA_OFF:
		// Уже все сделано
		break;

	case SX126X_ANTENNA_TX:
		rc = gpiod_line_set_value(brd->line_txen, 1);
		if (rc < 0)
			return rc;
		break;

	case SX126X_ANTENNA_RX:
		rc = gpiod_line_set_value(brd->line_rxen, 1);
		if (rc < 0)
			return rc;
		break;

	default:
		return -1;
	}

	return 0;
}


int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size)
{
	struct spi_ioc_transfer tran[2] = { 0 };

	// Первый проход - отправляем код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Второй проход - отправляем аргументы
	tran[1].tx_buf = (uint64_t)args;
	tran[1].len = args_size;
	tran[1].cs_change = true;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}


int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size)
{
	struct spi_ioc_transfer tran[3] = {0};

	// Загоняем код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Читаем статус устройства
	*status = 0xFF; // Гоним единички на TX
	tran[1].tx_buf = (uint64_t)status;
	tran[1].rx_buf = (uint64_t)status;
	tran[1].len = 1;
	tran[1].cs_change = false;

	memset(data, 0xFF, data_size);
	tran[2].tx_buf = (uint64_t)data;
	tran[2].rx_buf = (uint64_t)data;
	tran[2].len = data_size;
	tran[2].cs_change = true;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}


int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_WRITE_REGISTER;

	const uint8_t addr_bytes[2] = {
			(addr >> 8) & 0xFF,
			(addr >> 0) & 0xFF,
	};

	struct spi_ioc_transfer tran[3] = {0};

	// Код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Адрес регистра
	tran[1].tx_buf = (uint64_t)&addr_bytes;
	tran[1].len = sizeof(addr_bytes);
	tran[1].cs_change = false;

	// Значения
	tran[2].tx_buf = (uint64_t)data;
	tran[2].len = data_size;
	tran[2].cs_change = false;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}


int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_READ_REGISTER;

	const uint8_t addr_bytes[2] = {
			(addr >> 8) & 0xFF,
			(addr >> 0) & 0xFF,
	};

	struct spi_ioc_transfer tran[3] = {0};

	// Код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Адрес регистра
	tran[1].tx_buf = (uint64_t)&addr_bytes;
	tran[1].len = sizeof(addr_bytes);
	tran[1].cs_change = false;

	// Читаем статус (выкидываем его вернее)
	uint8_t _status = 0xFF;
	tran[2].tx_buf = (uint64_t)_status;
	tran[2].len = data_size;
	tran[2].cs_change = false;

	// Читаем данные
	memset(data, 0xFF, data_size);
	tran[3].tx_buf = (uint64_t)data;
	tran[3].rx_buf = (uint64_t)data;
	tran[3].len = data_size;
	tran[3].cs_change = true;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}


int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_WRITE_BUFFER;

	struct spi_ioc_transfer tran[3] = {0};

	// Код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Оффсет в буфере
	tran[1].tx_buf = (uint64_t)&offset;
	tran[1].len = 1;
	tran[1].cs_change = false;

	// Значения
	tran[2].tx_buf = (uint64_t)data;
	tran[2].len = data_size;
	tran[2].cs_change = true;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}


int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_READ_BUFFER;

	struct spi_ioc_transfer tran[3] = {0};
	// Код команды
	tran[0].tx_buf = (uint64_t)&cmd_code;
	tran[0].len = 1;
	tran[0].cs_change = false;

	// Адрес регистра
	tran[1].tx_buf = (uint64_t)&offset;
	tran[1].len = 1;
	tran[1].cs_change = false;

	// Читаем статус (выкидываем его вернее)
	uint8_t _status = 0xFF;
	tran[2].tx_buf = (uint64_t)_status;
	tran[2].len = data_size;
	tran[2].cs_change = false;

	// Читаем данные
	memset(data, 0xFF, data_size);
	tran[3].tx_buf = (uint64_t)data;
	tran[3].rx_buf = (uint64_t)data;
	tran[3].len = data_size;
	tran[3].cs_change = true;

	int rc = ioctl(brd->spidev_fd, SPI_IOC_MESSAGE(sizeof(tran)/sizeof(*tran)), tran);
	if (rc < 0)
		return -1;

	return 0;
}
