/*
 * my_i2c.c
 *
 *  Created on: 27 мар. 2020 г.
 *      Author: sereshotes
 */

#include "my_i2c.h"
#include <esp_err.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include "esp_log.h"

const static char *TAG = "my_i2c";
typedef struct i2c_dev_t {
	SemaphoreHandle_t mutex;
	StaticSemaphore_t xMutexBuffer;
} i2c_dev_t;

static i2c_dev_t i2c_dev_arr[I2C_NUM_MAX];

void i2c_master_mutex_init(i2c_port_t i2c_port) {
	if (i2c_dev_arr[i2c_port].mutex == 0) {
		i2c_dev_arr[i2c_port].mutex = xSemaphoreCreateMutexStatic(&i2c_dev_arr[i2c_port].xMutexBuffer);
		configASSERT(i2c_dev_arr[i2c_port].mutex);
	}
}

BaseType_t i2c_master_prestart(i2c_port_t i2c_port, int timeout) {
	ESP_LOGV(TAG, "prestart");
	return xSemaphoreTake(i2c_dev_arr[i2c_port].mutex, timeout);
}
BaseType_t i2c_master_postend(i2c_port_t i2c_port) {
	ESP_LOGV(TAG, "postend");
	return xSemaphoreGive(i2c_dev_arr[i2c_port].mutex);
}



int my_i2c_send(i2c_port_t i2c_port, uint8_t address, uint8_t *data, int size, int timeout) {
	if (size <= 0) {
		printf("Error size: size <= 0");
		return -1;
	}
	esp_err_t err;
	i2c_cmd_handle_t cmd =  i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address << 1 | I2C_MASTER_WRITE, 1);
	i2c_master_write(cmd, data, size, 1);
	i2c_master_stop(cmd);

	err = i2c_master_cmd_begin(i2c_port, cmd, timeout);

	i2c_cmd_link_delete(cmd);
	return err;
}
int my_i2c_recieve(i2c_port_t i2c_port, uint8_t address, uint8_t *data, int size, int timeout) {
	esp_err_t err;
	i2c_cmd_handle_t cmd =  i2c_cmd_link_create();
	if (size <= 0) {
		return -1;
	}
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address << 1 | I2C_MASTER_READ, 1);
	i2c_master_read(cmd, data, size, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);

	err = i2c_master_cmd_begin(i2c_port, cmd, timeout);
	i2c_cmd_link_delete(cmd);

	return err;
}
