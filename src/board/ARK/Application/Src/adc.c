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

static int convert_count = 0;
uint16_t temp_int = 0;
uint16_t vref = 0;
#define tV_25   1.43f      // Напряжение (в вольтах) на датчике при температуре 25 °C.
#define tSlope  0.0043f    // Изменение напряжения (в вольтах) при изменении температуры на градус.
extern ADC_HandleTypeDef hadc1;

typedef enum {
    CONV_TEMP,
    CONV_VCC,
    CONV_COUNT,
} conversion_t;
static conversion_t current_conversion;
static float t_avg = 0;
static float v_avg = 0;
#define BUF_SIZE 100
static uint16_t buf[BUF_SIZE];

// Напряжение со встроенного термистра при 25 градусах (в милливольта)
#define INTERNAL_TEMP_V25 (1430.0f)
// Коэффициент k внутренного термистра (мВ/C
#define INTERNAL_TEMP_AVG_SLOPE (4.3f)

void adc_task_init(void *arg) {
    //HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buf, BUF_SIZE);
}

void adc_task_update(void *arg) {

    static uint32_t prev = 0;
    uint32_t now = HAL_GetTick();
    if (now - prev > 500) {
        float t_avg = 0;
        for (int i = 0; i < BUF_SIZE; i+=2) {
            t_avg += buf[i];
        }
        float v_avg = 0;
        for (int i = 1; i < BUF_SIZE; i+=2) {
            v_avg += buf[i];
        }
        v_avg /= (float)BUF_SIZE;
        float v_res = VREFINT_CAL_VREF * (*VREFINT_CAL_ADDR) / (vrefint);
        t_avg /= (float)BUF_SIZE;
        float t_res = t_avg;
    }

    if (current_conversion == CONV_COUNT && convert_count < ADC_COUNT_IN_ROW) {
        convert_count++;

        float t0 = temp_int * 3.3 / (float)0x0FFF;
        float t = (tV_25 - t0) / tSlope + 25;
        t_avg += t;
        float v0 =  vref * 3.3 / (float)0x0FFF;
        v_avg += v0;

        //printf("-----temp: %d\n", (int)(100 * t));

        current_conversion = 0;
        printf("ADC: t = %d.%d, v = %d.%d\n", (int)t, (int)(t * 100) % 100, (int)v0, (int)(v0 * 100) % 100);
        HAL_ADC_Start_IT(&hadc1);
    }
    static uint32_t time = 0;
    static mavlink_message_t msg;
    static int is_sending = 0;
    if (convert_count >= ADC_COUNT_IN_ROW && HAL_GetTick() - time > ADC_PERIOD && !is_sending) {
        time = HAL_GetTick();
        convert_count = 0;
        t_avg /= ADC_COUNT_IN_ROW;
        v_avg /= ADC_COUNT_IN_ROW;

        its_time_t here = {0};
        its_gettimeofday(&here);

        mavlink_own_temp_t mot = {0};
        mot.deg = t_avg;
        mot.time_s = here.sec;
        mot.time_us = here.usec;
        mot.vdda = v_avg;

        mavlink_msg_own_temp_encode(mavlink_system, COMP_ANY_0, &msg, &mot);

        is_sending = 1;
        t_avg = 0;
        v_avg = 0;
    }
    if (is_sending) {
        if (uplink_packet(&msg) >= 0) {
            is_sending = 0;
        }
    }
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if(hadc->Instance == ADC1) {
        if (current_conversion == CONV_TEMP) {
            temp_int = HAL_ADC_GetValue(hadc);
            current_conversion++;
        }
        if (current_conversion == CONV_VCC) {
            vref = HAL_ADC_GetValue(hadc);
            current_conversion++;
        }
    }
}
