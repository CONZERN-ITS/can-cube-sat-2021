#include <stddef.h>
#include <string.h>

#include "sx126x_api.h"
#include "sx126x_drv.h"


#define SX126X_TCXO_STARTUP_TIMEOUT_MS (10)
#define SX126X_WAIT_BUSY_DEFAULT_TIMEOUT (1000)

#define SX126X_RETURN_IF_NONZERO(rc) if (0 != rc) return rc
#define SX126X_BREAK_IF_NONZERO(rc) if (0 != rc) break


//! Тип перехода в стендбай
typedef enum sx126x_drv_standby_switch_t
{
	//! Переходим в standby на внешнем генераторе xosc
	SX126X_DRV_STANDBY_SWITCH_XOSC,
	//! Переходим в standby на rc цепочке
	SX126X_DRV_STANDBY_SWITCH_RC,
	//! Переходим в стендбай, который по-умолчанию
	SX126X_DRV_STANDBY_SWITCH_DEFAULT
} sx126x_drv_standby_switch_t;


//! Дефолтный эвент хендлер, чтобы не делать проверок на NULL
static void _default_evt_handler(
		sx126x_drv_t * drv, void * user_arg, sx126x_evt_kind_t evt_kind, const sx126x_evt_arg_t * evt_arg
)
{
	return;
}


//! Удобный шорткат для ожидания busy состояния чипа
inline static int _wait_busy(sx126x_drv_t * drv)
{
	return sx126x_brd_wait_on_busy(drv->api.board, SX126X_WAIT_BUSY_DEFAULT_TIMEOUT);
}


//! Удобный шорткат для установки состояния антенны
inline static int _set_antenna(sx126x_drv_t * drv, sx126x_antenna_mode_t mode)
{
	return sx126x_brd_antenna_mode(drv->api.board, mode);
}


/*
//! Дельта времени с учетом переворота через 0
static uint32_t _time_diff(uint32_t start, uint32_t stop)
{
	// Если случился переворот через 0
	if (stop < start)
		return (UINT32_MAX - start) + stop;
	else
		return stop - start;
}
*/


//! Ищет в списке коэффициентов PA тот набор, который дает мощность не меньше чем указанную
/*! Коэффициенты должна быть отсортированы по убыванию итоговой мощности */
static size_t _best_pa_coeff_idx(int8_t power, const sx126x_pa_coeffs_t * coeffs, size_t coeffs_count)
{
	size_t best_idx = 0;

	// Ищем ближайшее число к указанному в массиве данные коэффициентов
	for (size_t i = 1; i < coeffs_count; i++)
	{
		if (power >= coeffs[i].power)
		{
			best_idx = i;
			break;
		}
	}

	return best_idx;
}


//! Заполнение структуры коэффициентов настройки PA для указанного типа чипов и мощности
static int _get_pa_coeffs(sx126x_chip_type_t chip_type, int8_t power, sx126x_pa_coeffs_t * coeffs)
{
	switch (chip_type)
	{
	case SX126X_CHIPTYPE_SX1261:
		{
			const sx126x_pa_coeffs_t defs[] = {
					SX126X_PA_COEFFS_1261
			};
			size_t idx = _best_pa_coeff_idx(power, defs, sizeof(defs)/sizeof(defs[0]));
			*coeffs = defs[idx];
		}
		break;

	case SX126X_CHIPTYPE_SX1262:
		{
			const sx126x_pa_coeffs_t defs[] = {
					SX126X_PA_COEFFS_1262
			};
			size_t idx = _best_pa_coeff_idx(power, defs, sizeof(defs)/sizeof(defs[0]));
			*coeffs = defs[idx];
		}
		break;

	case SX126X_CHIPTYPE_SX1268:
		{
			const sx126x_pa_coeffs_t defs[] = {
					SX126X_PA_COEFFS_1268
			};
			size_t idx = _best_pa_coeff_idx(power, defs, sizeof(defs)/sizeof(defs[0]));
			*coeffs = defs[idx];
		}
		break;

	default:
		return SX126X_ERROR_INVALID_VALUE;
	};

	return 0;
}


//! Костыль для правки бага с качеством модуляции на 500кгц лоры
static int _workaround_1_lora_bw500(sx126x_drv_t * drv)
{
	// Делаем какую-то магию, которую велено делать в даташите перед каждой TX операцией
	int rc;

	uint8_t value;
	rc = sx126x_brd_reg_read(drv->api.board, SX126X_REGADDR_LR_BW500_WORKAROUND, &value, sizeof(value));
	SX126X_RETURN_IF_NONZERO(rc);

	if (SX126X_DRV_MODEM_TYPE_LORA == drv->_modem_type && SX126X_LORA_BW_500 == drv->_modem_state.lora.bw)
	{
		// Для лоры на 500кГц особое значение
		value = value & 0xFB;
	}
	else
	{
		// Для всего остального - тоже особое, но другое
		value = value | 0x04;
	}
	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_LR_BW500_WORKAROUND, &value, sizeof(value));
	SX126X_RETURN_IF_NONZERO(rc);
	_wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Костыль для правки бага с ограничением TX мощности
static int _workaround_2_tx_power(sx126x_drv_t * drv)
{
	int rc;

	// Должен работать только для sx1262/sx1268
	sx126x_chip_type_t chip_type;
	rc = sx126x_brd_get_chip_type(drv->api.board, &chip_type);
	SX126X_RETURN_IF_NONZERO(rc);

	switch (chip_type)
	{
	case SX126X_CHIPTYPE_SX1262:
	case SX126X_CHIPTYPE_SX1268:
		break;

	default:
		return 0;
	}

	uint8_t reg_value;
	rc = sx126x_brd_reg_read(drv->api.board, SX126X_REGADDR_TX_CLAMP_CONFIG, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);

	reg_value |= 0x1E;

	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_TX_CLAMP_CONFIG, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Костыль 3 для парирования залипания RX таймаутов
static int _workaround_3_rx_timeout(sx126x_drv_t * drv)
{
	int rc;
	// работает всегда...
	uint8_t reg_value = 0x00;

	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_RTC_CONTROL1, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_brd_reg_read(drv->api.board, SX126X_REGADDR_RTC_CONTROL2, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);

	reg_value |= 0x02;

	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_RTC_CONTROL2, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Костыль для лечения багов возникающих при инверсии IQ
static int _workaround_4_iq_polarity(sx126x_drv_t * drv, bool iq_inversion_used)
{
	int rc;
	// работает всегда, когда вызывают (только при настройке лоры)

	uint8_t reg_value = 0x00;

	rc = sx126x_brd_reg_read(drv->api.board, SX126X_REGADDR_LR_INVIQ_WORKAROUND, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);

	if (iq_inversion_used)
		reg_value |= 0x04;
	else
		reg_value &= 0xF7;

	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_LR_INVIQ_WORKAROUND, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Настройка PA усилителя. Довольно громоздкая конструкция, поэтому вынесена отдельно
static int _configure_pa(sx126x_drv_t * drv, int8_t pa_power, const sx126x_pa_ramp_time_t ramp_time)
{
	int rc;
	sx126x_chip_type_t chip_type;

	rc = sx126x_brd_get_chip_type(drv->api.board, &chip_type);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ищем ближайшую к определенной мощности
	sx126x_pa_coeffs_t coeffs;
	rc = _get_pa_coeffs(chip_type, pa_power, &coeffs);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь, когда все параметры определены - загоняем их как есть
	// начинаем с set pa config
	rc = sx126x_api_set_pa_config(
			&drv->api, coeffs.duty_cycle, coeffs.hp_max, coeffs.device_sel, coeffs.pa_lut
	);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь делаем set_tx_params
	rc = sx126x_api_set_tx_params(&drv->api, coeffs.tx_params_power, ramp_time);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь проставляем оверкюрент
	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_OCP, &coeffs.ocp_value, sizeof(coeffs.ocp_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Установка опорной частоты
static int _set_rf_frequency(sx126x_drv_t * drv, uint32_t frequency)
{
	int rc = 0;
	uint8_t args[2];

	// Вобще говоря тут должна быть несколько разная стратегия для разных конкретных sx126x
	// но разница не сильно большая и конкретные цифры взятые из примеров - не сильно очевидны
	// поэтому попробуем так
	if( frequency > 900*1000*1000)
	{
		args[0] = 0xE1;
		args[1] = 0xE9;
	}
	else if(frequency > 850*1000*1000)
	{
		args[0] = 0xD7;
		args[1] = 0xD8;
	}
	else if(frequency > 770*1000*1000)
	{
		args[0] = 0xC1;
		args[1] = 0xC5;
	}
	else if(frequency > 460*1000*1000)
	{
		args[0] = 0x75;
		args[1] = 0x81;
	}
	else if(frequency > 425*1000*1000)
	{
		args[0] = 0x6B;
		args[1] = 0x6F;
	}
	else
	{
		// Так же, нам нельзя опускаться ниже 400, так как
		// это запрещено нашими конфигурациями PA усилителя
		rc = SX126X_ERROR_INVALID_FREQUENCY;
	}
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_calibrate_image(&drv->api, args);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_set_rf_frequency(&drv->api, frequency);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Настройка синхрослова лоры для закрытых и открытых лора-сетей
/*! Не совсем понятно что это значит */
static int _set_lora_syncword(sx126x_drv_t * drv, sx126x_lora_syncword_t syncword_value)
{
	int rc;

	const uint8_t syncword_bytes[2] = {
			(syncword_value >> 8) & 0xFF,
			(syncword_value     ) & 0xFF
	};
	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_LR_SYNCWORD, syncword_bytes, sizeof(syncword_bytes));
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Получение и очиства IRQ флагов с устройства
static int _fetch_clear_irq(sx126x_drv_t * drv, uint16_t * irq_status)
{
	int rc;

	*irq_status = 0;

	// Сходим к устройству и спросим как у него дела
	rc = sx126x_api_get_irq_status(&drv->api, irq_status);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Сбросим все флаги прерывания, которые успели увидеть
	// не сбрасываем по 0xFFFF на случай, если что-то случилось пока мы не видели
	if (*irq_status)
	{
		rc = sx126x_api_clear_irq_status(&drv->api, *irq_status);
		SX126X_RETURN_IF_NONZERO(rc);
		rc = _wait_busy(drv);
		SX126X_RETURN_IF_NONZERO(rc);
	}

	rc = sx126x_brd_cleanup_irq(drv->api.board);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Смена состояния
static int _switch_state_to_standby(sx126x_drv_t * drv, sx126x_drv_standby_switch_t standby_kind)
{
	int rc;

	// Проверяем в какой именно стендбай мы пойдем
	sx126x_drv_state_t new_state;
	switch (standby_kind)
	{
	case SX126X_DRV_STANDBY_SWITCH_RC:
		new_state = SX126X_DRV_STATE_STANDBY_RC;
		break;

	case SX126X_DRV_STANDBY_SWITCH_XOSC:
		new_state = SX126X_DRV_STATE_STANDBY_XOSC;
		break;

	case SX126X_DRV_STANDBY_SWITCH_DEFAULT:
		switch (drv->_default_standby)
		{
		case SX126X_STANDBY_RC:
			new_state = SX126X_DRV_STATE_STANDBY_RC;
			break;

		case SX126X_STANDBY_XOSC:
			new_state = SX126X_DRV_STATE_STANDBY_XOSC;
			break;

		default:
			return SX126X_ERROR_BAD_STATE;
		}
		break;

	default:
		return SX126X_ERROR_INVALID_VALUE;
		break;
	};

	// Проверяем, находимся мы в этом состоянии реально или нет
	sx126x_status_t status;
	rc = sx126x_api_get_status(&drv->api, &status);
	SX126X_RETURN_IF_NONZERO(rc);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
// warning: this statement may fall through. Так задумано
	switch (status.bits.chip_mode)
	{
	case SX126X_STATUS_CHIPMODE_STDBY_RC:
		if (SX126X_DRV_STATE_STANDBY_RC == new_state)
			break;

	case SX126X_DRV_STATE_STANDBY_XOSC:
		if (SX126X_DRV_STATE_STANDBY_XOSC == new_state)
			break;

	default:
		return SX126X_ERROR_BAD_CHIPMODE;
	}
#pragma GCC diagnostic pop
	// Если мы выходим из RX - нужно делать воркэраунд на таймер
	if (drv->_state == SX126X_DRV_STATE_RX)
	{
		rc = _workaround_3_rx_timeout(drv);
		SX126X_RETURN_IF_NONZERO(rc);
	}

	// Выключаем антенну
	rc = _set_antenna(drv, SX126X_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->_state = new_state;
	return 0;
}


//! Переинициализация дескриптора параметрами после reset-а модуля
static void _load_defaults(sx126x_drv_t * drv)
{
	// Без настроек это работает именно так
	drv->_default_standby = SX126X_STANDBY_RC;
	// Модем еще не настроен никак
	drv->_modem_type = SX126X_DRV_MODEM_TYPE_UNCONFIGURED;

	// Просто это нужно выставить в какое-то корректное значение
	// чтобы не стрелял воркэраунд, если кто-то вдруг решит попередавать без настроек
	// Хотя ничего хорошего из этого не выйдет и так и так...
	drv->_modem_state.lora.bw = SX126X_LORA_BW_007;

	// Это значение по-умолчанию
	drv->_modem_state.lora.cad_exit_mode = SX126X_LORA_CAD_ONLY;

	// Это, наверное, тоже значения по-умолчанию?
	// Аналогично, нам нужно иметь тут какие-то определенные значения
	drv->_modem_state.lora.explicit_header = false;
	drv->_modem_state.lora.payload_length = 0;
}

//! Проверка на то, находится ли модуль в standby режиме
/*! Вынесена отдельно, больно часто уж используется */
static bool _is_in_standby(sx126x_drv_t * drv)
{
	const bool retval = (
			drv->_state == SX126X_DRV_STATE_STANDBY_RC
		||	drv->_state == SX126X_DRV_STATE_STANDBY_XOSC
	);

	return retval;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


int sx126x_drv_ctor(sx126x_drv_t * drv, void * board_ctor_arg)
{
	int rc;
	memset(drv, 0, sizeof(*drv));

	rc = sx126x_api_ctor(&drv->api, board_ctor_arg);
	SX126X_RETURN_IF_NONZERO(rc);

	_load_defaults(drv);

	// Пока что мы ничего не настраивали
	drv->_state = SX126X_DRV_STATE_STARTUP;
	drv->_evt_handler = _default_evt_handler;

	return 0;
}


void sx126x_drv_dtor(sx126x_drv_t * drv)
{
	sx126x_api_dtor(&drv->api);
}


int sx126x_drv_set_event_handler(sx126x_drv_t * drv, sx126x_evt_handler_t handler, void * cb_user_arg)
{
	drv->_evt_handler = handler;
	drv->_evt_handler_user_arg = cb_user_arg;

	return 0;
}


sx126x_drv_state_t sx126x_drv_state(sx126x_drv_t * drv)
{
	return drv->_state;
}


int sx126x_drv_reset(sx126x_drv_t * drv)
{
	int rc;

	rc = sx126x_brd_reset(drv->api.board);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = _set_antenna(drv, SX126X_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	_load_defaults(drv);
	drv->_state = SX126X_DRV_STATE_STARTUP;
	return 0;
}


int sx126x_drv_configure_basic(sx126x_drv_t * drv, const sx126x_drv_basic_cfg_t * config)
{
	// Такую настройку можно делать только в startup состоянии
	if (drv->_state != SX126X_DRV_STATE_STARTUP)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Убежаемся что мы в правильном режиме
	rc = sx126x_api_set_standby(&drv->api, SX126X_STANDBY_RC);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Начали настраивать
	// В первую очередь определяемся с регулятором
	sx126x_regulator_t regulator_mode = config->allow_dcdc ? SX126X_REGULATOR_DCDC : SX126X_REGULATOR_LDO;
	rc = sx126x_api_set_regulator_mode(&drv->api, regulator_mode);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь включаем TCXO, если нужно
	if (config->use_dio3_for_tcxo)
	{
		rc = sx126x_api_set_dio3_as_tcxo_ctl(&drv->api, config->tcxo_v, SX126X_TCXO_STARTUP_TIMEOUT_MS);
		SX126X_RETURN_IF_NONZERO(rc);
		rc = _wait_busy(drv);
		SX126X_RETURN_IF_NONZERO(rc);
	}

	// Затем включаем rf переключатель, если вдруг он нужен
	rc = sx126x_api_set_dio2_as_rf_switch(&drv->api, config->use_dio2_for_rf_switch);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Ставим режим фолбека rx/tx
	sx126x_fallback_mode_t fallback_mode;
	switch (config->standby_mode)
	{
	case SX126X_STANDBY_RC:
		fallback_mode = SX126X_FALLBACK_RC;
		break;

	case SX126X_STANDBY_XOSC:
		fallback_mode = SX126X_FALLBACK_XOSC;
		break;

	default:
		return SX126X_ERROR_INVALID_VALUE;
	}
	rc = sx126x_api_set_rxtx_fallback_mode(&drv->api, fallback_mode);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Делаем хак2
	rc = _workaround_2_tx_power(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем прерывания
	uint16_t irq_mask = SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR
			| SX126X_IRQ_CAD_DETECTED | SX126X_IRQ_CAD_DONE
	;
	rc = sx126x_api_set_dio_irq_params(&drv->api, irq_mask, irq_mask, 0x00, 0x00);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->_default_standby = config->standby_mode;

	// Переходим в стендбай RC режим
	drv->_state = SX126X_DRV_STATE_STANDBY_RC;
	return 0;
}


int sx126x_drv_mode_standby_rc(sx126x_drv_t * drv)
{
	// Сперва нужно сконфигурироваться
	if (drv->_state < SX126X_DRV_STATE_STANDBY_RC)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	rc = sx126x_api_set_standby(&drv->api, SX126X_STANDBY_RC);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = _switch_state_to_standby(drv, SX126X_DRV_STATE_STANDBY_RC);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_drv_mode_standby(sx126x_drv_t * drv)
{
	// Сперва нужно сконфигурироваться
	if (drv->_state < SX126X_DRV_STATE_STANDBY_RC)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	rc = sx126x_api_set_standby(&drv->api, drv->_default_standby);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Все ок, меняем свое состояние
	rc = _switch_state_to_standby(drv, SX126X_DRV_STANDBY_SWITCH_DEFAULT);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_drv_configure_lora_modem(sx126x_drv_t * drv, const sx126x_drv_lora_modem_cfg_t * config)
{
	// Такое можно делать только в стендбае RC
	if (drv->_state != SX126X_DRV_STATE_STANDBY_RC)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Перво-наперво, как велит даташит - делаем set_packet_type
	rc = sx126x_api_set_packet_type(&drv->api, SX126X_PACKET_TYPE_LORA);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь выставляем частоту
	rc = _set_rf_frequency(drv, config->frequency);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь настраиваем мощность передачтика
	rc = _configure_pa(drv, config->pa_power, config->pa_ramp_time);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ставим параметры модуляции
	const sx126x_lora_modulation_params_t mod_params = {
			.spreading_factor = config->spreading_factor,
			.bandwidth = config->bandwidth,
			.coding_rate = config->coding_rate,
			.ldr_optimizations = config->ldr_optimizations
	};
	rc = sx126x_api_set_lora_modulation_params(&drv->api, &mod_params);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь настроим буст LNA
	uint8_t reg_value = config->lna_boost ? SX126X_RXGAIN_BOOSTED : SX126X_RXGAIN_POWERSAVE;
	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_RX_GAIN, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->_modem_type = SX126X_DRV_MODEM_TYPE_LORA;
	drv->_modem_state.lora.bw = config->bandwidth;
	return 0;
}


int sx126x_drv_configure_lora_packet(sx126x_drv_t * drv, const sx126x_drv_lora_packet_cfg_t * config)
{
	// Такое можно делать только в стендбае
	if (!_is_in_standby(drv))
		return SX126X_ERROR_BAD_STATE;

	// И еще только, если выбран лора модем
	if (SX126X_DRV_MODEM_TYPE_LORA != drv->_modem_type)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Ставим параметры пакетизации
	const sx126x_lora_packet_params_t packet_params = {
			.preamble_length = config->preamble_length,
			.explicit_header = config->explicit_header,
			.payload_length = config->payload_length,
			.use_crc = config->use_crc,
			.invert_iq = config->invert_iq
	};
	rc = sx126x_api_set_lora_packet_params(&drv->api, &packet_params);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Хак инвертированного IQ
	rc = _workaround_4_iq_polarity(drv, config->invert_iq);
	SX126X_RETURN_IF_NONZERO(rc);

	// Настраиваем синхрослово лоры
	rc = _set_lora_syncword(drv, config->syncword);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->_modem_state.lora.explicit_header = config->explicit_header;
	drv->_modem_state.lora.payload_length = config->payload_length;
	return 0;
}


int sx126x_drv_configure_lora_cad(sx126x_drv_t * drv, const sx126x_drv_cad_cfg_t * config)
{
	// Такое можно делать только в стендбае
	if (!_is_in_standby(drv))
		return SX126X_ERROR_BAD_STATE;

	int rc;
	rc = sx126x_api_set_cad_params(
			&drv->api,
			config->cad_len,
			config->cad_min,
			config->cad_peak,
			config->exit_mode,
			config->cad_timeout_ms
	);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->_modem_state.lora.cad_exit_mode = config->exit_mode;
	return 0;
}


int sx126x_drv_configure_lora_rx_timeout(sx126x_drv_t * drv, const sx126x_drv_lora_rx_timeout_cfg_t * config)
{
	// Такое можно делать только в стендбае
	if(!_is_in_standby(drv))
		return SX126X_ERROR_BAD_STATE;

	int rc;
	rc = sx126x_api_stop_rx_timer_on_preamble(&drv->api, config->stop_timer_on_preable);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_set_lora_symb_num_timeout(&drv->api, config->lora_symb_timeout);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}



int sx126x_drv_payload_write(sx126x_drv_t * drv, const uint8_t * data, uint8_t data_size)
{
	int rc;
	// Загоняем пакет в радио
	rc = sx126x_brd_buf_write(drv->api.board, 0, data, data_size);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_drv_payload_rx_size(sx126x_drv_t * drv, uint8_t * data_size)
{
	int rc;
	uint8_t payload_size;
	uint8_t payload_offset;

	if (SX126X_DRV_MODEM_TYPE_LORA == drv->_modem_type && drv->_modem_state.lora.explicit_header)
	{
		// Если используется лора с неявным заголовком, то размер пакета мы и так знаем
		payload_size = drv->_modem_state.lora.payload_length;
	}
	else
	{
		// Если нет - придется лезть в чип и спрашивать
		rc = sx126x_api_get_rx_buffer_status(&drv->api, &payload_size, &payload_offset);
		SX126X_RETURN_IF_NONZERO(rc);
		// Не ждем busy
	}

	*data_size = payload_size;
	return 0;
}


int sx126x_drv_payload_read(sx126x_drv_t * drv, uint8_t * buffer, uint8_t buffer_size)
{
	// Такое можно делать только в стендбае
	if(!_is_in_standby(drv))
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Выгребаем
	rc = sx126x_brd_buf_read(drv->api.board, 0, buffer, buffer_size);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_drv_mode_rx(sx126x_drv_t * drv, uint32_t timeout_ms)
{
	// Сперва нужно сконфигурироваться
	if (drv->_state < SX126X_DRV_STATE_STANDBY_RC)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Выставляем указатели буфера
	rc = sx126x_api_buffer_base_address(&drv->api, 0, 0);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем антенну
	rc = _set_antenna(drv, SX126X_ANTENNA_RX);
	SX126X_RETURN_IF_NONZERO(rc);

	// Запускаем rx операцию
	rc = sx126x_api_set_rx(&drv->api, timeout_ms);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Проверяем что реально начали
	sx126x_status_t status;
	rc = sx126x_api_get_status(&drv->api, &status);
	SX126X_RETURN_IF_NONZERO(rc);
	if (status.bits.chip_mode != SX126X_STATUS_CHIPMODE_RX)
		return SX126X_ERROR_BAD_CHIPMODE;

	// Переходим в RX
	drv->_state = SX126X_DRV_STATE_RX;

	if (0xFFFF == timeout_ms)
		drv->_infinite_rx = true;

	return 0;
}


int sx126x_drv_mode_tx(sx126x_drv_t * drv, uint32_t timeout_ms)
{
	// Сперва нужно сконфигурироваться
	if (drv->_state < SX126X_DRV_STATE_STANDBY_RC)
		return SX126X_ERROR_BAD_STATE;

	int rc;
	// Выставляем указатели буфера
	rc = sx126x_api_buffer_base_address(&drv->api, 0, 0);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// делаем workaround1 из даташита
	rc = _workaround_1_lora_bw500(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем антенну
	rc = _set_antenna(drv, SX126X_ANTENNA_TX);
	SX126X_RETURN_IF_NONZERO(rc);

	// Запускаем передачу пакета
	rc = sx126x_api_set_tx(&drv->api, timeout_ms);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Проверяем что реально начали
	sx126x_status_t status;
	rc = sx126x_api_get_status(&drv->api, &status);
	SX126X_RETURN_IF_NONZERO(rc);
	if (status.bits.chip_mode != SX126X_STATUS_CHIPMODE_TX)
		return SX126X_ERROR_BAD_CHIPMODE;

	drv->_state = SX126X_DRV_STATE_TX;
	return 0;
}


int sx126x_drv_poll_irq(sx126x_drv_t * drv)
{
	int rc;
	sx126x_evt_arg_t evt_arg;
	uint16_t irq_status;

	// Получим irq флаги
	rc = _fetch_clear_irq(drv, &irq_status);
	SX126X_RETURN_IF_NONZERO(rc);

	switch (drv->_state)
	{
	case SX126X_DRV_STATE_RX:
		if (irq_status & (SX126X_IRQ_RX_DONE))
		{
			if (!drv->_infinite_rx)
			{
				rc = _switch_state_to_standby(drv, SX126X_DRV_STANDBY_SWITCH_DEFAULT);
				SX126X_RETURN_IF_NONZERO(rc);
			}

			evt_arg.rx_done.timed_out = false;
			evt_arg.rx_done.crc_valid = (irq_status & SX126X_IRQ_CRC_ERROR) ? false : true;
			// Сбегаем к чипу и достанем статистику пакета
			if (SX126X_DRV_MODEM_TYPE_LORA == drv->_modem_type)
			{
				rc = sx126x_api_get_lora_packet_status(&drv->api, &evt_arg.rx_done.packet_status.lora);
				SX126X_RETURN_IF_NONZERO(rc);
			}

			drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVTKIND_RX_DONE, &evt_arg);
		}
		else if (irq_status & SX126X_IRQ_TIMEOUT)
		{
			rc = _switch_state_to_standby(drv, SX126X_DRV_STANDBY_SWITCH_DEFAULT);
			SX126X_RETURN_IF_NONZERO(rc);

			evt_arg.rx_done.timed_out = true;
			drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVTKIND_RX_DONE, &evt_arg);
		}
		break;

	case SX126X_DRV_STATE_TX:
		if (irq_status & (SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT))
		{
			rc = _switch_state_to_standby(drv, SX126X_DRV_STANDBY_SWITCH_DEFAULT);
			SX126X_RETURN_IF_NONZERO(rc);

			evt_arg.tx_done.timed_out = (irq_status & SX126X_IRQ_TIMEOUT) != 0;
			drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVTKIND_TX_DONE, &evt_arg);
		}
		break;

	default:
		break;
	};

	return 0;
}


int sx126x_drv_rssi_inst(sx126x_drv_t * drv, int8_t * value)
{
	// Такое можно делать только в RX (или CAD)
	if (drv->_state != SX126X_DRV_STATE_RX)
		return SX126X_ERROR_BAD_STATE;

	int rc = sx126x_api_get_rssi_inst(&drv->api, value);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}

