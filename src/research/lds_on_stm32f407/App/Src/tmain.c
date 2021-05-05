/*
 * tmain.c
 *
 *  Created on: Mar 13, 2021
 *      Author: sereshotes
 */

#include <math.h>
#include <stdio.h>

#include "lds.h"
#include "matrix.h"
#include "matrix_static_alloc.h"
#include "tmain.h"


#define ADC_TIMEOUT 100
#define STDIO_UART_TIMEOUT 100

int _read(int file, char *ptr, int len)
{
    if (HAL_UART_Receive(&huart1, (uint8_t *)ptr, len, STDIO_UART_TIMEOUT) == HAL_OK) {
        return len;
    } else {
        return 0;
    }
}

int _write(int file, char *ptr, int len)
{
    if (HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, STDIO_UART_TIMEOUT) == HAL_OK) {
        return len;
    } else {
        return 0;
    }
}



int tmain() {
    float x[3];
    float b[LDS_COUNT];
    int step = 0;
    while (1) {
        for (int i = 0; i < LDS_COUNT; i++) {
            hadc1.Instance->SQR3 = i;
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT);
            uint32_t value = HAL_ADC_GetValue(&hadc1);
            float v = ((float)value / (1 << 12)) * 3.3;

            b[i] = v;
        }

        float sph[3];
        lds_find(x, b);
        dekart_to_euler(x, sph);

        for (int i = 0; i < LDS_COUNT; i++) {
            printf("a%d: %f0.3 ", i, b[i]);
        }
        printf("\n");

        printf("%4d: mag: %4.4f theta: %.4f phi: %.4f\n", step, sph[0], degrees(sph[1]), degrees(sph[2]));
        HAL_Delay(1000);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        step++;
    }

    return 0;
}
