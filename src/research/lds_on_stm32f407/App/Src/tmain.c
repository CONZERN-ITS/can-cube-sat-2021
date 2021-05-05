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

#define LDS_COUNT 3
#define LDS_DIM 3

float Arr[3][3] = {
        {1.00000, 0.00000, 0.00000 },
        {-1.00000, 0.75249, 0.75249 },
        {0.00000, -1.46190, 1.46190 }
};

float degrees(float a) {
    return a * 180 / M_PI;
}

int tmain() {
    Matrixf x;
    Matrixf b;
    lds_t lds;
    Matrixf temp;


    MATRIX_SA(x, LDS_DIM, 1);
    MATRIX_SA(b, LDS_COUNT, 1);
    MATRIX_SA(temp, LDS_DIM, LDS_DIM);
    MATRIX_SA(lds.A, LDS_COUNT, LDS_DIM);
    MATRIX_SA(lds.As, LDS_DIM, LDS_COUNT);

    Matrixf arr;
    arr.arr = (float *)Arr;
    arr.height = 3;
    arr.width = 3;
    arr.reserved = 9;
    lds_init(&lds, LDS_COUNT, LDS_DIM, &temp);
    hadc1.Instance->SQR3 = 0;
    int step = 0;
    while (1) {
        for (int i = 0; i < LDS_COUNT; i++) {
            hadc1.Instance->SQR3 = i;
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT);
            uint32_t value = HAL_ADC_GetValue(&hadc1);
            float v = ((float)value / (1 << 12)) * 3.3;

            *matrix_at(&b, i, 0) = v;
        }
        matrix_multiplicate(&arr, &b, &x);
/*
        lds_search(&lds, &b, &x);
        float err = lds_get_error(&lds, &b, &x);
*/

        float mag = matrix_norm(&x);
        float phi = atan(*matrix_at(&x, 1, 0) / *matrix_at(&x, 0, 0));
        float psi = acos(*matrix_at(&x, 2, 0) / mag);
        printf("a1: %4.4f: a2: %4.4f a3: %4.4f\n", *matrix_at(&b, 0, 0), *matrix_at(&b, 1, 0), *matrix_at(&b, 2, 0));
        printf("%4d: mag: %4.4f phi: %.4f psi: %.4f\n", step, mag, degrees(phi), degrees(psi));
        HAL_Delay(1000);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        step++;
    }

    return 0;
}
