/*
 * task_ina.c
 *
 *  Created on: Apr 25, 2020
 *      Author: sereshotes
 */

#include "task_ina.h"


static tina_value_t ina_value[TINA_COUNT];
static int _is_valid[TINA_COUNT];


static ina219_t hina[2];

static void (*ina_callback_arr[TINA_CALLBACK_COUNT])(void);
static int ina_callback_count;

void tina219_add_ina_callback(void (*f)(void)) {
    assert(ina_callback_count < TINA_CALLBACK_COUNT);
    ina_callback_arr[ina_callback_count++] = f;
}

int tina219_get_value(tina_value_t **arr, int **is_valid) {
    *arr = ina_value;
    *is_valid = _is_valid;
    return TINA_COUNT;
}

void task_ina_init(void *arg) {

#   if defined CUBE_1 && !defined CUBE_2
	ina219_init_default(&hina[0], INA_BUS_HANDLE, INA219_I2CADDR_A1_GND_A0_GND << 1, INA_TIMEOUT);
	ina219_init_default(&hina[1], INA_BUS_HANDLE, INA219_I2CADDR_A1_GND_A0_VSP << 1, INA_TIMEOUT);
#   elif defined CUBE_2 && !defined CUBE_1
    // не делаем ничего, так как ин тут нет
#   else
#       error "invalid cube definition"
    // устаревший вариант для двух инн
    //ina219_init_default(&hina[0], &hi2c2, INA219_I2CADDR_A1_GND_A0_GND << 1, INA_TIMEOUT);
    //ina219_init_default(&hina[1], &hi2c2, INA219_I2CADDR_A1_GND_A0_VSP << 1, INA_TIMEOUT);

#   endif
}

void task_ina_update(void *arg) {
    static uint32_t prev = 0;
    if (HAL_GetTick() - prev < TINA_READ_PERIOD) {
        return;
    }
    prev += TINA_READ_PERIOD;
    ina219_secondary_data_t sd;
    ina219_primary_data_t pd;

    for (int i = 0; i < TINA_COUNT; i++) {
         if (!ina219_read_all(&hina[i], &pd, &sd)) {
             _is_valid[i] = 1;
             ina_value[i].current = ina219_current_convert(&hina[i], sd.current);
             ina_value[i].voltage = ina219_bus_voltage_convert(&hina[i], pd.busv);
         } else {
             _is_valid[i] = 0;

             // Грузим шину, если получам невалидные данные
             INA_I2C_FORCE_RESET();
             INA_I2C_RELEASE_RESET();

             HAL_I2C_MspDeInit(INA_BUS_HANDLE);
             HAL_Delay(10);
             __HAL_I2C_RESET_HANDLE_STATE(INA_BUS_HANDLE);
             HAL_I2C_MspInit(INA_BUS_HANDLE);

             //ужасный копипаст
             ina219_init_default(&hina[0], INA_BUS_HANDLE, INA219_I2CADDR_A1_GND_A0_GND << 1, INA_TIMEOUT);
             ina219_init_default(&hina[1], INA_BUS_HANDLE, INA219_I2CADDR_A1_GND_A0_VSP << 1, INA_TIMEOUT);

         }
    }

    for (int i = 0; i < ina_callback_count; i++) {
        (*ina_callback_arr[i])();
    }
}
