#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <stm32f1xx_hal.h>


//! Размер пакета передаваемого/принимаемого за одну транзакцию
#define I2C_LINK_PACKET_SIZE	(279)


typedef enum imi_cmd_t
{
	IMI_CMD_NONE = 0x00,
	IMI_CMD_GET_SIZE = 0x01,
	IMI_CMD_GET_PACKET = 0x02,
	IMI_CMD_SET_PACKET = 0x04,
} imi_cmd_t;


typedef I2C_HandleTypeDef * imi_port_t;


typedef struct {
	imi_port_t i2c_port;
	uint8_t address;
	int timeout;
} imi_t;


extern I2C_HandleTypeDef hi2c1;
typedef I2C_HandleTypeDef * imi_bus_handle_t;


static int imi_send_cmd(imi_t * imi, imi_cmd_t cmd) {
	HAL_StatusTypeDef rc;
	uint8_t cmd_buf = cmd;
	rc = HAL_I2C_Master_Transmit(
			imi->i2c_port, imi->address << 1, &cmd_buf, sizeof(uint8_t), imi->timeout
	);

	if (rc != HAL_OK)
		printf("Error send cmd: %d\n", (int)rc);

	return rc;
}


static int imi_msg_send(imi_t * imi, uint8_t *data, uint16_t size) {
	HAL_StatusTypeDef rc;

	if (size > I2C_LINK_PACKET_SIZE)
	{
		printf("packet is too large!\n");
		return HAL_ERROR;
	}

	rc = imi_send_cmd(imi, IMI_CMD_SET_PACKET);
	if (rc)
		return rc;

	uint8_t buffer[I2C_LINK_PACKET_SIZE + sizeof(size)] = {0};
	int buffer_size = size + sizeof(size);

	memcpy(buffer, &size, sizeof(size));
	memcpy(buffer + sizeof(size), data, size);

	rc = HAL_I2C_Master_Transmit(imi->i2c_port, imi->address << 1, buffer, buffer_size, imi->timeout);
	if (rc != HAL_OK) {
		printf("Error: msg send: %d\n", (int)rc);
		return rc;
	}

	return HAL_OK;
}


static int imi_get_packet_size(imi_t * imi, uint16_t *size) {

	HAL_StatusTypeDef rc;
	rc = imi_send_cmd(imi, IMI_CMD_GET_SIZE);
	if (rc) {
		printf("Error: get packet size: %d\n", (int)rc);
		return rc;
	}

	uint16_t rsize = 0;
	rc = HAL_I2C_Master_Receive(
			imi->i2c_port, imi->address << 1, (uint8_t*)&rsize, sizeof(size), imi->timeout
	);
	if (rc != HAL_OK) {
		printf("Error: get packet size 2: %d\n", (int)rc);
		return rc;
	}
	*size = rsize;
	return HAL_OK;
}


static int imi_get_packet(imi_t * imi, uint8_t *data, uint16_t size) {
	HAL_StatusTypeDef rc;

	rc = imi_send_cmd(imi, IMI_CMD_GET_PACKET);
	if (rc != HAL_OK) {
		printf("Error: imi_get_packet: %d\n", (int)rc);
		return rc;
	}

	rc = HAL_I2C_Master_Receive(
			imi->i2c_port, imi->address << 1, data, size, imi->timeout
	);

	if (rc != HAL_OK) {
		printf("Error: imi_get_packet 2: %d\n", (int)rc);
		return rc;
	}

	return HAL_OK;
}



int app_main()
{
	I2C_HandleTypeDef * hi2c = &hi2c1;

	__HAL_I2C_ENABLE(hi2c);
	imi_t sins;
	sins.address = 0xAA;
	sins.i2c_port = hi2c;
	sins.timeout = HAL_MAX_DELAY;

	uint8_t pbuffer[I2C_LINK_PACKET_SIZE] = {0};
	for(uint8_t i = 0; true; i++)
	{
		int rc;
		HAL_Delay(1000);

		uint16_t psize;
		rc = imi_get_packet_size(&sins, &psize);
		if (HAL_OK != rc)
			continue;

		memset(pbuffer, 0x00, sizeof(pbuffer));
		rc = imi_get_packet(&sins, pbuffer, psize);
		if (HAL_OK != rc)
			continue;

		printf("got packet: ");
		for (int j = 0; j < psize; j++)
			printf("0x%02X", (int)pbuffer[j]);

		printf("\n");

		HAL_Delay(1000);

		memset(pbuffer, i, sizeof(pbuffer));
		rc = imi_msg_send(&sins, pbuffer, sizeof(pbuffer));
		if (HAL_OK != rc)
			continue;

		printf("sent packet: ");
		for (int j = 0; j < sizeof(pbuffer); j++)
			printf("0x%02X", pbuffer[j]);

		printf("\n");
	}

	return 0;
}
