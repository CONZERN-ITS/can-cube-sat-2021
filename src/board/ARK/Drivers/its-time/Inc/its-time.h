/*
 * its-time.c
 *
 *  Created on: 28 мар. 2020 г.
 *      Author: sereshotes
 */

#ifndef ITS_TIME_INC_ITS_TIME_C_
#define ITS_TIME_INC_ITS_TIME_C_

#include <stm32f1xx_hal.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include "mavlink.h"
#pragma GCC diagnostic pop


/*
 * May be 0 - 0x00FFFFFF
 */
#define ITS_TIME_PRESCALER 39740
#define ITS_TIME_AUTO_PRESCALER 0

typedef struct {
    uint64_t sec;
    uint16_t msec;
} its_time_t;



int its_time_init(void);
void its_gettimeofday(its_time_t *time);
void its_gettimeofday_with_timebase(its_time_t *time, uint8_t * time_base);
void its_settimeofday(its_time_t *time);
void its_settimeofday_with_timebase(its_time_t *time, uint8_t time_base);
void its_get_time_base(uint8_t * time_base);
void its_set_time_base(uint8_t time_base);
void its_count__time_sync_attempts(void);
void ist_count__time_syncs_count(void);
void its_set_correction_delta_time(uint32_t sync_ts, int64_t sync_delta_s, int64_t sync_delta_us);
uint32_t its_time_gettick(void);
void its_collect_ark_stats(mavlink_ark_stats_t * msg);


void its_sync_time(its_time_t *from_bcs);


/*
 * Delay in microseconds (not milliseconds)
 * us - count of microseconds
 */
inline __attribute__((always_inline)) void its_delay_us(uint32_t us)
{
    volatile uint32_t startTick = DWT->CYCCNT;
    volatile uint32_t delayTicks = us * (SystemCoreClock/1000000);

    while (DWT->CYCCNT - startTick < delayTicks);
}

#endif /* ITS_TIME_INC_ITS_TIME_C_ */
