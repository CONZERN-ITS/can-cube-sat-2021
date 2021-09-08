/*
 * task_read.c
 *
 *  Created on: Apr 24, 2020
 *      Author: sereshotes
 */
#include <stdio.h>
#include <task_ds.h>
#include "main.h"

#include "assert.h"
#include "task.h"
#include "ds18b20.h"
#include "onewire.h"

//#define DS_SEARCH

static float ds_temp[TDS_TEMP_MAX_COUNT];
static int _is_valid[TDS_TEMP_MAX_COUNT];


static ds18b20_config_t hds[TDS_TEMP_MAX_COUNT] =
{
        // Из-за путаницы при сборке в первом кубе стоит вторая АКБ
        { .rom = 0xc301206bf4fb5828 }, // банка 1
        { .rom = 0xe501206c06262828 }, // банка 2
        { .rom = 0x8101206bffd48f28 }, // банка 3
        { .rom = 0x0000000b4dbaca28 }  // банка 4
};
static int ds_count = 4;


static onewire_t how;


static void (*callback_arr[TDS_CALLBACK_COUNT])(void);
static int callback_count;

void tds18b20_add_callback(void (*f)(void)) {
    assert(callback_count < TDS_CALLBACK_COUNT);
    callback_arr[callback_count++] = f;
}

int tds18b20_get_value(float **arr, int **is_valid) {
    *arr = ds_temp;
    *is_valid = _is_valid;
    return ds_count;
}

void _ds_sort(ds18b20_config_t *arr, int size) {
    int is_sorted = 0;
    for (int j = size - 1; j > 0 && !is_sorted; j--) {
        is_sorted = 1;
        for (int i = 0; i < j; i++) {
            if (arr[i].rom > arr[i + 1].rom) {
                is_sorted = 0;

                ds18b20_config_t t = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = t;
            }
        }
    }
}

void task_ds_init(void *arg) {
    onewire_Init(&how, OW_GPIO_Port, OW_Pin);
    HAL_Delay(100);

#ifdef DS_SEARCH
    int status = onewire_First(&how);
    int index = 0;
    while (status) {
        //Save ROM number from device
        onewire_GetFullROM(&how, &hds[index].rom);
        hds[index].how = &how;
        hds[index].resolution = ds18b20_Resolution_12bits;
        index++;
        //Check for new device
        status = onewire_Next(&how);

        if (index >= TDS_TEMP_MAX_COUNT) {
            break;
        }
    }
    ds_count = index;
    printf("DS_TASK ds count = %d:\n", ds_count);
    for (int i = 0; i < ds_count; i++) {
        printf("\t[i] = ");
        uint8_t *arr = (uint8_t*) &hds[i].rom;
        for (int j = 0; j < sizeof(hds[i].rom); j++) {
            printf("0x%02x, ", arr[j]);
        }
        printf("\n");
    }
    if (ds_count == 0) {
        return;
    }
    _ds_sort(hds, ds_count);
#else
    for (int i = 0; i < ds_count; i++)
    {
        hds[i].how = &how;
        hds[i].resolution = ds18b20_Resolution_12bits;
    }
#endif

    ds18b20_StartAll(&hds[0]);
}

void TM_GPIO_SetPinAsInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void task_ds_update(void *arg) {
    static uint32_t prev = 0;
    int is_done = ds18b20_AllDone(hds);
    if (!is_done) {
        return;
    }
    uint32_t now = HAL_GetTick();
    if (ds_count == 0 || now - prev < TDS_READ_PERIOD) {
        return;
    }

    prev += TDS_READ_PERIOD;
    for (int i = 0; i < ds_count; i++) {
        _is_valid[i] = ds18b20_Read(&hds[i], &ds_temp[i]);
    }
    ds18b20_StartAll(&hds[0]);
    ONEWIRE_INPUT(hds[0].how);
    for (int i = 0; i < ds_count; i++) {
        printf("DS[%d] = %d.%02d\n", i, (int)ds_temp[i], (int)(ds_temp[i] * 100) % 100);
    }
    for (int i = 0; i < callback_count; i++) {
        (*callback_arr[i])();
    }
}


