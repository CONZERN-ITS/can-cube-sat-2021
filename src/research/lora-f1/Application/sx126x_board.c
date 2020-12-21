#include <string.h>

#include <stm32f1xx_hal.h>

#include "main.h"
#include "sx126x_board.h"


extern SPI_HandleTypeDef hspi1;
#define hspi (&hspi1)


static void _cs_down(void)
{
	HAL_GPIO_WritePin(SX126X_CS_GPIO_Port, SX126X_CS_Pin, GPIO_PIN_RESET);
}


static void _cs_up(void)
{
	HAL_GPIO_WritePin(SX126X_CS_GPIO_Port, SX126X_CS_Pin, GPIO_PIN_SET);
}


int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg)
{
	(void)user_arg;
	*brd = NULL;

	HAL_GPIO_WritePin(SX126X_NRST_GPIO_Port, SX126X_NRST_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(SX126X_TXE_GPIO_Port, SX126X_TXE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SX126X_RXE_GPIO_Port, SX126X_RXE_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(SX126X_CS_GPIO_Port, SX126X_CS_Pin, GPIO_PIN_SET);

	return 0;
}

void sx126x_brd_dtor(sx126x_board_t * brd)
{
	(void)brd;
}


uint32_t sx126x_brd_get_time(sx126x_board_t * brd)
{
	return HAL_GetTick();
}


int sx126x_brd_get_chip_type(sx126x_board_t * brd, sx126x_chip_type_t * chip_type)
{
	*chip_type = SX126X_CHIPTYPE_SX1268;
	return 0;
}


int sx126x_brd_reset(sx126x_board_t * brd)
{
	HAL_GPIO_WritePin(SX126X_NRST_GPIO_Port, SX126X_NRST_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(SX126X_NRST_GPIO_Port, SX126X_NRST_Pin, GPIO_PIN_SET);
	return 0;
}


int sx126x_brd_wait_on_busy(sx126x_board_t * brd)
{
	while(1)
	{
		GPIO_PinState state = HAL_GPIO_ReadPin(SX126X_BUSY_GPIO_Port, SX126X_BUSY_Pin);
		if (state != GPIO_PIN_RESET)
			continue;
		else
			break;
	}

	return 0;
}


void sx126x_brd_enable_irq(sx126x_board_t * brd)
{
	return;
}


void sx126x_brd_disable_irq(sx126x_board_t * brd)
{
	return;
}


int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode)
{
	int rc = 0;

	HAL_GPIO_WritePin(SX126X_TXE_GPIO_Port, SX126X_TXE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SX126X_RXE_GPIO_Port, SX126X_RXE_Pin, GPIO_PIN_RESET);

	switch (mode)
	{
	case SX126X_ANTENNA_OFF:
		break;

	case SX126X_ANTENNA_TX:
		HAL_GPIO_WritePin(SX126X_TXE_GPIO_Port, SX126X_TXE_Pin, GPIO_PIN_SET);
		break;

	case SX126X_ANTENNA_RX:
		HAL_GPIO_WritePin(SX126X_RXE_GPIO_Port, SX126X_RXE_Pin, GPIO_PIN_SET);
		break;

	default:
		rc = -1;
	}

	return rc;
}


int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size)
{
	_cs_down();

	HAL_SPI_Transmit(hspi, &cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)args, args_size, HAL_MAX_DELAY);

	_cs_up();
	return 0;
}


int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size)
{
	uint8_t _status = 0xFF;
	memset(data, 0xFF, data_size);

	_cs_down();

	HAL_SPI_Transmit(hspi, &cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, &_status, sizeof(_status), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, data, data_size, HAL_MAX_DELAY);

	if (status) *status = _status;

	_cs_up();
	return 0;
}


int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_WRITE_REGISTER;

	const uint8_t addr_bytes[2] = {
			(addr >> 8) & 0xFF,
			(addr >> 0) & 0xFF,
	};

	_cs_down();

	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)addr_bytes, sizeof(addr_bytes), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)data, data_size, HAL_MAX_DELAY);

	_cs_up();

	return 0;
}


int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_READ_REGISTER;

	const uint8_t addr_bytes[2] = {
			(addr >> 8) & 0xFF,
			(addr >> 0) & 0xFF,
	};

	uint8_t _status = 0xFF;
	memset(data, 0xFF, data_size);

	_cs_down();

	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)addr_bytes, sizeof(addr_bytes), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, &_status, sizeof(_status), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, data, data_size, HAL_MAX_DELAY);

	_cs_up();

	return 0;
}


int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_WRITE_BUFFER;

	_cs_down();

	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)&offset, sizeof(offset), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)data, data_size, HAL_MAX_DELAY);

	_cs_up();

	return 0;
}


int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size)
{
	const uint8_t cmd_code = SX126X_CMD_READ_BUFFER;

	uint8_t _status = 0xFF;
	memset(data, 0xFF, data_size);

	_cs_down();

	HAL_SPI_Transmit(hspi, (uint8_t*)&cmd_code, sizeof(cmd_code), HAL_MAX_DELAY);
	HAL_SPI_Transmit(hspi, (uint8_t*)&offset, sizeof(offset), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, (uint8_t*)&_status, sizeof(_status), HAL_MAX_DELAY);
	HAL_SPI_Receive(hspi, (uint8_t*)data, data_size, HAL_MAX_DELAY);

	_cs_up();

	return 0;
}
