/*
 * its-time.c
 *
 *  Created on: 28 мар. 2020 г.
 *      Author: sereshotes
 */


#include "../Inc/its-time.h"
#include "sys/time.h"
#include "main.h"


typedef struct {
    RTC_TypeDef *RTCx;
    its_time_t globalTime;
    uint16_t usec_shift;
    uint32_t div_load;
    uint8_t time_base;
    // Количество раз сколько мы пытались синхронизовать время
    uint16_t _time_sync_attempts;
    // Количество раз сколько мы синхронизовали время
    uint16_t _time_syncs_count;
    // Время последней коррекции (по шкале HAL_GetTick())
    uint32_t _last_sync_ts;
    // Величина последней коррекции
    int64_t _last_sync_delta_s;
    int64_t _last_sync_delta_us;

} its_time_handler_t;

typedef struct {
	uint16_t resets_count;
	uint16_t reset_cause;
} its_reset_t;

//-------------------------------------------------------------------------------------
//PRIVATE

static its_time_handler_t its_time_h = {0};
static its_reset_t its_reset_h = {0};

static void _read_DIV_CNT_timesafe(RTC_TypeDef *RTCx, uint32_t *div, uint32_t *cnt);
static uint16_t _div_tick_to_usec(uint32_t tick, uint32_t max);
//-------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------
//PUBLIC

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
static uint16_t _fetch_reset_cause(void);
void its_collect_ark_stats(mavlink_ark_stats_t * msg);

struct Time gettime(void);


//-------------------------------------------------------------------------------------


/*
 * Initializes DWT timer for microsecond clock and
 * sets RTC prescaler value
 */
int its_time_init(void)
{
    if (1) {
        //DWT->LAR = 0xC5ACCE55;
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    its_time_h.RTCx = RTC;

    uint32_t prescaler = ITS_TIME_PRESCALER;
    if (ITS_TIME_AUTO_PRESCALER) {
        prescaler = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_RTC);

        /* Check that RTC clock is enabled*/
        if (prescaler == 0U) {
            /* Should not happen. Frequency is not available*/
            return HAL_ERROR;
        } else {
            /* RTC period = RTCCLK/(RTC_PR + 1) */
            prescaler = prescaler - 1U;
        }
    }
    its_time_h.div_load = prescaler;

    // Грузим из бэкап регистров количество рестартов, которое с нами случилось
    its_reset_h.resets_count = LL_RTC_BKP_GetRegister(BKP, LL_RTC_BKP_DR1);
    its_reset_h.resets_count += 1;
	LL_RTC_BKP_SetRegister(BKP, LL_RTC_BKP_DR1, its_reset_h.resets_count);

	// Смотрим причину последнего резета
	its_reset_h.reset_cause = _fetch_reset_cause();

    LL_RTC_DisableWriteProtection(its_time_h.RTCx);
    LL_RTC_SetAsynchPrescaler(its_time_h.RTCx, prescaler);
    LL_RTC_EnableWriteProtection(its_time_h.RTCx);

    return 0;
}

/*
 * Reads time from the RTC.
 * RTC resets prescaler counter after setting CNT. So
 * we have to save microseconds in variable and add everytime
 * when we want to get time
 */
void __attribute__((weak)) its_gettimeofday(its_time_t *time) {
    uint32_t cnt, div;
    _read_DIV_CNT_timesafe(its_time_h.RTCx, &div, &cnt);
    uint16_t usec = _div_tick_to_usec(div + 1, its_time_h.div_load + 1);
    usec += its_time_h.usec_shift;
    time->usec = usec % 1000;
    time->sec  = cnt + usec / 1000;
}

void __attribute__((weak)) its_gettimeofday_with_timebase(its_time_t *time, uint8_t * time_base)
{
	uint32_t cnt, div;
	_read_DIV_CNT_timesafe(its_time_h.RTCx, &div, &cnt);
	uint16_t usec = _div_tick_to_usec(div + 1, its_time_h.div_load + 1);
	usec += its_time_h.usec_shift;
	time->usec = usec % 1000;
	time->sec  = cnt + usec / 1000;

	*time_base = its_time_h.time_base;
}
/*
 * Sets time in the RTC
 * We have to save microseconds because of reasons
 * explained in comments for its_gettimeofday
 */
void __attribute__((weak)) its_settimeofday(its_time_t *time) {
    its_time_h.usec_shift = time->usec;
    LL_RTC_DisableWriteProtection(its_time_h.RTCx);
    LL_RTC_TIME_Set(its_time_h.RTCx, time->sec);
    LL_RTC_EnableWriteProtection(its_time_h.RTCx);
}


void __attribute__((weak)) its_settimeofday_with_timebase(its_time_t *time, uint8_t time_base) {
	its_time_h.time_base = time_base;
	its_time_h.usec_shift = time->usec;
    LL_RTC_DisableWriteProtection(its_time_h.RTCx);
    LL_RTC_TIME_Set(its_time_h.RTCx, time->sec);
    LL_RTC_EnableWriteProtection(its_time_h.RTCx);
}


void __attribute__((weak)) its_get_time_base(uint8_t * time_base)
{
	*time_base = its_time_h.time_base;
}


void __attribute__((weak)) its_set_time_base(uint8_t time_base)
{
	its_time_h.time_base = time_base;
}


void __attribute((weak)) its_count__time_sync_attempts()
{
	its_time_h._time_sync_attempts++;
}


void __attribute((weak)) ist_count__time_syncs_count()
{
	its_time_h._time_syncs_count++;
}


void __attribute((weak)) its_set_correction_delta_time(uint32_t sync_ts, int64_t sync_delta_s, int64_t sync_delta_us)
{
	its_time_h._last_sync_ts = sync_ts;
	its_time_h._last_sync_delta_s = sync_delta_s;
	its_time_h._last_sync_delta_us = sync_delta_us;
}

/*
 * Returns count of system ticks
 * This clock ticks evey 1 / SystemCoreClock of seconds. So,
 * to get microseconds divide it by (SystemCoreClock/1000000)
 */
uint32_t its_time_gettick(void) {
    return DWT->CYCCNT;
}


/*
 * Safes us from the situation, when one of the regs is overflowed
 * in the moment between reading them. It would be bad because
 * global time bias increases every time when that happens.
 */
static void _read_DIV_CNT_timesafe(RTC_TypeDef *RTCx, uint32_t *div, uint32_t *cnt) {
    /*
    SET_BIT(RTCx->CRL, RTC_CRL_CNF);
    LL_RTC_WaitForSynchro(RTCx);
    CLEAR_BIT(RTCx->CRL, RTC_CRL_CNF);*/

    uint16_t val[2][4];
    val[0][3] = READ_REG(RTCx->CNTH & RTC_CNTH_RTC_CNT);
    val[0][2] = READ_REG(RTCx->CNTL & RTC_CNTL_RTC_CNT);
    val[0][1] = READ_REG(RTCx->DIVH & RTC_DIVH_RTC_DIV);
    val[0][0] = READ_REG(RTCx->DIVL & RTC_DIVL_RTC_DIV);

    val[1][3] = READ_REG(RTCx->CNTH & RTC_CNTH_RTC_CNT);
    val[1][2] = READ_REG(RTCx->CNTL & RTC_CNTL_RTC_CNT);
    val[1][1] = READ_REG(RTCx->DIVH & RTC_DIVH_RTC_DIV);
    val[1][0] = READ_REG(RTCx->DIVL & RTC_DIVL_RTC_DIV);


    int test = 0;
    for (int i = 1; i < 4; i++) {
        if (val[0][i] != val[1][i]) {
            test = 1;
            break;
        }
    }


    *cnt = ((uint32_t)(((uint32_t) val[test][3] << 16U) | val[test][2]));
    //DIVH: [15:4] - reserved, [3:0] - actual value
    *div = ((uint32_t)((((uint32_t) val[test][1] & 0x000F) << 16U) | val[test][0]));
}

/*
 * Transfers ticks from DIV register to milliseconds
 */
static uint16_t _div_tick_to_usec(uint32_t tick, uint32_t max) {
    return (max - tick) * 1000 / max;
}


void its_collect_ark_stats(mavlink_ark_stats_t * msg)
{
	its_time_t now;
	its_gettimeofday(&now);
	msg->time_s = now.sec;
	msg->time_us = now.usec;
	msg->time_steady = HAL_GetTick();


	if (RCC_PLLSOURCE_HSI_DIV2 == __HAL_RCC_GET_PLL_OSCSOURCE())
		msg->active_oscillator = ACTIVE_OSCILLATOR_HSI;
	else
		msg->active_oscillator = ACTIVE_OSCILLATOR_HSE;

	msg->last_time_sync_delta_s = its_time_h._last_sync_delta_s;
	msg->last_time_sync_delta_us = its_time_h._last_sync_delta_us;
	msg->last_time_sync_time_steady = its_time_h._last_sync_ts;
	msg->reset_cause = its_reset_h.reset_cause;
	msg->resets_count = its_reset_h.resets_count;
	msg->time_base = its_time_h.time_base;
	msg->time_syncs_attempted = its_time_h._time_sync_attempts;
	msg->time_syncs_performed = its_time_h._time_syncs_count;
}


static uint16_t _fetch_reset_cause(void)
{
	int rc = 0;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
		rc |= MCU_RESET_PIN;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
		rc |= MCU_RESET_POR;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
		rc |= MCU_RESET_SW;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
		rc |= MCU_RESET_WATCHDOG;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
		rc |= MCU_RESET_WATCHDOG2;

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
		rc |= MCU_RESET_LOWPOWER;

	__HAL_RCC_CLEAR_RESET_FLAGS();
	return rc;
}

