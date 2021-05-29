/*
 * MIT License
 *
 * Copyright (c) 2017 David Antliff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "sensors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "pinout_cfg.h"
#include "router.h"
#include "mavlink_help2.h"

#include "ds18b20.h"
#include "ina219.h"

#define ITS_ESP_DEBUG

static void sensors_ina_task(void *arg);
static void sensors_task(void *arg);
#define SAMPLE_PERIOD 1000
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
static char *TAG = "SENSORS";

esp_err_t sensors_init(void) {
	xTaskCreatePinnedToCore(sensors_task, "Sensors task", configMINIMAL_STACK_SIZE + 3000, 0, 1, 0, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(sensors_ina_task, "Sensors ina task", configMINIMAL_STACK_SIZE + 3000, 0, 1, 0, tskNO_AFFINITY);
	return 0;
}

static OneWireBus_ROMCode device_rom_codes[ITS_OWB_MAX_DEVICES];
//#define USE_GPIO
static void sensors_task(void *arg) {
	OneWireBus * owb = 0;
#ifdef USE_GPIO
	owb_gpio_driver_info gpio_driver_info = {0};
	vTaskDelay(2000/portTICK_PERIOD_MS);
	owb = owb_gpio_initialize(&gpio_driver_info, ITS_PIN_OWB);
	vTaskDelay(2000/portTICK_PERIOD_MS);
#else
	owb_rmt_driver_info rmt_driver_info = {0};
	owb = owb_rmt_initialize(&rmt_driver_info, ITS_PIN_OWB, RMT_CHANNEL_1, RMT_CHANNEL_0);
#endif
	owb_use_crc(owb, true);  // enable CRC check for ROM code

#ifndef ITS_ESP_DEBUG
	// Если это не стенд

	const OneWireBus_ROMCode device_rom_codes[ITS_OWB_MAX_DEVICES] = {
			{ .bytes = { 0x28, 0x52, 0x95, 0xE6, 0x0B, 0x00, 0x00, 0xBA } }, // БСК1
			{ .bytes = { 0x28, 0x6E, 0x55, 0xE6, 0x0B, 0x00, 0x00, 0x08 } }, // БСК 2-2
			{ .bytes = { 0x28, 0xD6, 0x96, 0xE6, 0x0B, 0x00, 0x00, 0xC2 } }, // БСК 3-2
			{ .bytes = { 0x28, 0xE6, 0x89, 0xE6, 0x0B, 0x00, 0x00, 0x3C } }, // БСК 4-2
			{ .bytes = { 0x28, 0x4F, 0x19, 0xAC, 0x0A, 0x00, 0x00, 0x11 } }  // БСК 5-2


	};

	const int num_devices = 5;
#else // if defined ITS_ESP_DEBUG
	// Если это стенд
	// Find all connected devices
	ESP_LOGD(TAG, "Find devices:");

	int num_devices = 0;

	OneWireBus_SearchState search_state = {0};
	bool found = false;
	owb_search_first(owb, &search_state, &found);
	while (found)
	{
		char rom_code_s[17];
		owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
		ESP_LOGD(TAG, "  %d : %s", num_devices, rom_code_s);
		device_rom_codes[num_devices] = search_state.rom_code;
		++num_devices;
		owb_search_next(owb, &search_state, &found);
	}
	ESP_LOGD(TAG, "Found %d device%s", num_devices, num_devices == 1 ? "" : "s");

#endif

	for (int i = 0; i < num_devices; i++)
	{
		const OneWireBus_ROMCode * rom_code = &device_rom_codes[i];
		char rom_code_s[OWB_ROM_CODE_STRING_LENGTH] = {0};
		owb_string_from_rom_code(*rom_code, rom_code_s, sizeof(rom_code_s));

		ESP_LOGD(TAG, "checking ds18b20 number %d", i);

		bool is_present;
		owb_status search_status = owb_verify_rom(owb, *rom_code, &is_present);
		if (search_status == OWB_STATUS_OK)
		{
			ESP_LOGI(TAG, "Device %s is %s", rom_code_s, is_present ? "present" : "not present");
		}
		else
		{
			ESP_LOGE(TAG, "An error occurred searching for known device: %d", search_status);
		}
	}

	ESP_LOGV(TAG, "Creating devices: ");
	// Create DS18B20 devices on the 1-Wire bus
	DS18B20_Info * devices[ITS_OWB_MAX_DEVICES] = {0};
	for (int i = 0; i < num_devices; ++i)
	{
		DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
		ESP_LOGV(TAG, "%d, %d", i, ds18b20_info != 0);
		devices[i] = ds18b20_info;
		ESP_LOGV(TAG, "c1");

		ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
		ESP_LOGV(TAG, "c2");
		ds18b20_use_crc(ds18b20_info, true);           // enable CRC check on all reads
		ESP_LOGV(TAG, "c3");
		ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
		ESP_LOGV(TAG, "c4");
	}

	// Check for parasitic-powered devices
	bool parasitic_power = false;
	ds18b20_check_for_parasite_power(owb, &parasitic_power);
	if (parasitic_power) {
		ESP_LOGD(TAG, "Parasitic-powered devices detected");
	}

	// In parasitic-power mode, devices cannot indicate when conversions are complete,
	// so waiting for a temperature conversion must be done by waiting a prescribed duration
	owb_use_parasitic_power(owb, 0);

#ifdef CONFIG_ENABLE_STRONG_PULLUP_GPIO
	// An external pull-up circuit is used to supply extra current to OneWireBus devices
	// during temperature conversions.
	owb_use_strong_pullup_gpio(owb, CONFIG_STRONG_PULLUP_GPIO);
#endif

	// Read temperatures more efficiently by starting conversions on all devices at the same time
	//int errors_count[ITS_OWB_MAX_DEVICES] = {0};

	ESP_LOGD(TAG, "Start cycle");

	ESP_LOGV(TAG, "Main cycle");
	if (num_devices > 0)
	{
		TickType_t last_wake_time = xTaskGetTickCount();

		while (1)
		{
			ESP_LOGV(TAG, "step");
			last_wake_time = xTaskGetTickCount();

			ds18b20_convert_all(owb);

			// In this application all devices use the same resolution,
			// so use the first device to determine the delay
			ds18b20_wait_for_conversion(devices[0]);
			struct timeval tv = {0};
			gettimeofday(&tv, 0);

			// Read the results immediately after conversion otherwise it may fail
			// (using printf before reading may take too long)
			float readings[ITS_OWB_MAX_DEVICES] = { 0 };
			DS18B20_ERROR errors[ITS_OWB_MAX_DEVICES] = { 0 };

			for (int i = 0; i < num_devices; ++i)
			{
				errors[i] = ds18b20_read_temp(devices[i], &readings[i]);
			}

			for (int i = 0; i < num_devices; ++i) {
				mavlink_thermal_state_t mts = {0};
				mavlink_message_t msg = {0};
				mts.time_s = tv.tv_sec;
				mts.time_us = tv.tv_usec;
				mts.temperature = errors[i] == DS18B20_OK ? readings[i] : NAN;
				mavlink_msg_thermal_state_encode(mavlink_system, i, &msg, &mts);
				its_rt_sender_ctx_t ctx = {0};

				ctx.from_isr = 0;
				its_rt_route(&ctx, &msg, 0);
			}

			vTaskDelayUntil(&last_wake_time, SAMPLE_PERIOD / portTICK_PERIOD_MS);
		}
	}
	else
	{
		ESP_LOGD(TAG, "No DS18B20 devices detected!");
	}

	// clean up dynamically allocated data
	for (int i = 0; i < num_devices; ++i)
	{
		ds18b20_free(&devices[i]);
	}
	owb_uninitialize(owb);

	vTaskDelete(0);
}

static void sensors_ina_task(void *arg) {
#define INA_MAX 6
	ina219_t ina[INA_MAX];
	ina219_init_desc(&ina[0], INA219_ADDR_GND_GND, ITS_I2CTM_PORT);
	ina219_init_desc(&ina[1], INA219_ADDR_GND_SCL, ITS_I2CTM_PORT);
	ina219_init_desc(&ina[2], INA219_ADDR_GND_SDA, ITS_I2CTM_PORT);
	ina219_init_desc(&ina[3], INA219_ADDR_GND_VS, ITS_I2CTM_PORT);
	ina219_init_desc(&ina[4], INA219_ADDR_SCL_GND, ITS_I2CTM_PORT);
	ina219_init_desc(&ina[5], INA219_ADDR_SDA_SDA, ITS_I2CTM_PORT);

	for (int i = 0; i < 6; i++) {
		ina219_configure(&ina[i], INA219_BUS_RANGE_16V, INA219_GAIN_1,
				INA219_RES_12BIT_128S, INA219_RES_12BIT_128S, INA219_MODE_CONT_SHUNT_BUS);
	}
	while (1) {
		struct timeval tp = {0};
		gettimeofday(&tp, 0);
		mavlink_electrical_state_t mes[INA_MAX];
		for (int i = 0; i < 6; i++) {
			if (ina219_get_bus_voltage(&ina[i], &mes[i].voltage) != ESP_OK) {
				mes[i].voltage = NAN;
			}
			if (ina219_get_current(&ina[i], &mes[i].current) != ESP_OK) {
				mes[i].current = NAN;
			}
		}
		for (int i = 0; i < 6; i++) {
			uint8_t buf[MAVLINK_MAX_PACKET_LEN];
			mavlink_message_t msg

			mavlink_msg_electrical_state_decode(&msg, &mes[i]);

			its_rt_sender_ctx_t ctx = {0};
			ctx.from_isr = 0;
			its_rt_route(&ctx, buf, 0);
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
