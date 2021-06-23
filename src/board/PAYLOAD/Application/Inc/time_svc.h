/*
 * time_svc.h
 *
 *  Created on: Jun 12, 2020
 *      Author: snork
 */

#ifndef INC_TIME_SVC_H_
#define INC_TIME_SVC_H_

#include <sys/time.h>

#include <stm32f4xx_hal.h>

#include "mavlink_main.h"

extern TIM_HandleTypeDef htim4;
// Халовский хендл таймера на котором мы работаем
#define TIMESVC_TIM_HANDLE (&htim4)

typedef struct time_svt_stats_t
{
	//! Количество попыток (или вернее сказать возможностей синхронизации)
	uint16_t syncs_attempted;
	//! Количество реально проведенных синхронизаций
	uint16_t syncs_performed;
	//! Метка времени последней проведенной коррекции
	//! по монотонной шкале
	uint32_t last_sync_time_steady;
	//! Величина последней проведенной коррекции, целая часть
	int64_t last_sync_delta_s;
	//! Величина последней проведенной коррекции, дробная часть (мкс)
	int64_t last_sync_delta_us;
} time_svc_stats_t;


//! Инициализация службы времени
/*! таймер, указанный TIMESVC_TIM_HANDLE должен быть настроен так, чтобы частота
 *  у него была 2000 кГц а период 2000. То есть один тик весит пол
 *  миллисекунды а переполняется таймер раз в секунду */
void time_svc_init(void);

//! Получение базы времени
uint8_t time_svc_get_time_base(void);

//! получение статистики модуля
void time_svc_get_stats(time_svc_stats_t * stats);

//! Получение текущего времени
void time_svc_gettimeofday(struct timeval * tmv);

//! Установка текущего времени
void time_svc_settimeofday(const struct timeval * tmv);


//! Эту функцию нужно вызывать в обработчике прерывания целевого таймера
void time_svc_on_tim_interrupt(void);

//! Эту функцию нужно вызывать в обработчике прерывания по получению временного синхросигнала
void time_svc_on_sync_interrupt(void);

//! Эту функцию нужно вызывать при получении mavlink сообщения от ведущего
void time_svc_on_mav_message(const mavlink_message_t * msg);

#endif /* INC_TIME_SVC_H_ */
