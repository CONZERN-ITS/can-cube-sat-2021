#include <sx126x_board.h>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include <string.h>

#include <sx126x_board_rpi.h>


#define SX126X_MAX_SPI_SPEED (10*1000*1000)


struct sx126x_board_t
{
	//! Дескриптор SPI шины
	int spidev_fd;
	//! Время начала нашей работы (для get_time в монотонных миллисекундах)
	struct timespec start_time;
};


static sx126x_board_t _dev;


static int _spi_init(const char * filename, int * fd_)
{
	int fd = open(filename, O_RDWR);
	if (fd < 0)
		return errno;

	// Выставляем максимальную скорость
	const uint32_t speed = SX126X_MAX_SPI_SPEED;
	int rc = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (rc < 0)
		return errno;

	rc = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (rc < 0)
		return errno;

	// Кажется, все готово
	return 0;
}



int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg_)
{
	const sx126x_board_rpi_ctor_arg_t * arg = (sx126x_board_rpi_ctor_arg_t *)user_arg_;
	memset(&_dev, 0x00, sizeof(_dev));

	// Настраиваем SPI
	int rc = _spi_init(arg->dev_path, &_dev.spidev_fd);
	if (rc != 0)
		return rc;

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

	// закрывем spi
	if (brd->spidev_fd)
	{
		close(brd->spidev_fd);
		brd->spidev_fd = 0;
	}

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
	return 0;
}


int sx126x_brd_wait_on_busy(sx126x_board_t * brd)
{
	return 0;
}


void sx126x_brd_enable_irq(sx126x_board_t * brd)
{
	
}


void sx126x_brd_disable_irq(sx126x_board_t * brd)
{
	
}


int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode)
{
	return 0;
}


int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size)
{
	return 0;
}


int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size)
{
	return 0;
}


int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size)
{
	return 0;
}
