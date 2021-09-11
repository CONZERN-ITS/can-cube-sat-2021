/*
 * adc.c
 *
 *  Created on: Jul 16, 2020
 *      Author: sereshotes
 */


#include "adc.h"
#include <stdio.h>
#include "mavlink_help2.h"
#include "uplink.h"
#include "its-time.h"

#define ADC_PERIOD 1000 //ms
#define ADC_COUNT_IN_ROW 10

uint16_t temp_int = 0;
uint16_t vref = 0;
#define tV_25   1.43f      // Напряжение (в вольтах) на датчике при температуре 25 °C.
#define tSlope  0.0043f    // Изменение напряжения (в вольтах) при изменении температуры на градус.
extern ADC_HandleTypeDef hadc1;

#define BUF_SIZE 200
#define MEASURE_PERIOD_MS 1000
#define TEMP_SENSOR_V25 1430 // (1340 .. 1520) mv
#define TEMP_SENSOR_SLOPE 4.3 // (4.0 .. 4.6)
static uint16_t adc_buffer[BUF_SIZE];

// Напряжение со встроенного термистра при 25 градусах (в милливольта)
#define INTERNAL_TEMP_V25 (1430.0f)
// Коэффициент k внутренного термистра (мВ/C
#define INTERNAL_TEMP_AVG_SLOPE (4.3f)

void adc_task_init(void *arg) {
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, sizeof(adc_buffer)/sizeof(adc_buffer[0]));
}

void adc_task_update(void *arg) {

	static uint32_t prev = 0;

	const uint32_t now = HAL_GetTick();
	if (now - prev < MEASURE_PERIOD_MS)
		return;

	prev = now;

	int64_t temp_raw_accum = 0;
	int64_t vref_raw_accum = 0;
	for (size_t i = 0; i < sizeof(adc_buffer)/sizeof(adc_buffer[0]); i++)
	{
		if (i % 2 == 0)
			temp_raw_accum += adc_buffer[i];
		else
			vref_raw_accum += adc_buffer[i];
	}

	float temp_raw_avg = (float)temp_raw_accum / (sizeof(adc_buffer) / sizeof(adc_buffer[0]) / 2);
	float vref_raw_avg = (float)vref_raw_accum / (sizeof(adc_buffer) / sizeof(adc_buffer[0]) / 2);
	float vdda = 1200.f / vref_raw_avg  * 0xfff;
	float vtemp = vdda / 0xfff * temp_raw_avg;
	float temp_c = (TEMP_SENSOR_V25 - vtemp) / TEMP_SENSOR_SLOPE + 25;
	printf("ADC: vdda: %d, temp_uc %d\n", (int)(vdda), (int)(temp_c*1000));

	static mavlink_message_t msg;
	its_time_t here = {0};
	its_gettimeofday(&here);

	mavlink_own_temp_t mot = {0};
	mot.time_s = here.sec;
	mot.time_us = here.usec;
	mot.vdda = vdda/1000;
	mot.deg = temp_c;

	mavlink_msg_own_temp_encode(mavlink_system, COMP_ANY_0, &msg, &mot);
	uplink_packet(&msg);
}
