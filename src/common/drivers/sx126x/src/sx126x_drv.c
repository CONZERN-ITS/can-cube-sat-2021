#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sx126x_api.h"
#include "sx126x_drv.h"

#define SX126X_TCXO_STARTUP_TIMEOUT_MS (10)

#define SX126X_RETURN_IF_NONZERO(rc) if (0 != rc) return rc
#define SX126X_BREAK_IF_NONZERO(rc) if (0 != rc) break


//! Причина, по которой меняется состояние модуля
typedef enum sx126x_state_switch_flag_t
{
	//! Переход из IDLE состояния
	SX126X_SSF_FROM_IDLE		= 0x00,

	//! Случился hardware timeout
	SX126X_SSF_HW_TIMEOUT		= 0x01,
	//! Случился software timeout
	SX126X_SSF_SW_TIMEOUT		= 0x02,

	//! Мы занимались получением пакета
	SX126X_SSF_RX				= 0x10,
	//! Мы занимались передачей пакета
	SX126X_SSF_TX				= 0x20,

} sx126x_state_switch_flag_t;


static void _default_evt_handler(
		sx126x_drv_t * drv, void * user_arg, sx126x_evt_kind_t evt_kind, const sx126x_evt_arg_t * evt_arg
)
{
	return;
}



inline static int _wait_busy(sx126x_drv_t * drv)
{
	return sx126x_brd_wait_on_busy(drv->api.board);
}


inline static int _set_antenna(sx126x_drv_t * drv, sx126x_antenna_mode_t mode)
{
	return sx126x_brd_antenna_mode(drv->api.board, mode);
}


//! Дельта времени с учетом переворота через 0
static uint32_t _time_diff(uint32_t start, uint32_t stop)
{
	// Если случился переворот через 0
	if (stop < start)
		return (UINT32_MAX - start) + stop;
	else
		return stop - start;
}


//! Костыль для правки бага с качеством модуляции на 500кгц лоры
static int _workaround_1_lora_bw500(sx126x_drv_t * drv)
{
	int rc;

	// Этот хак должен работать только для лоры на 500кГц
	if (drv->_modem_type != SX126X_PACKET_TYPE_LORA || drv->_modem_state.lora.bw != SX126X_LORA_BW_500)
		return 0;

	// Делаем какую-то магию, которую велено делать в даташите перед каждой TX операцией
	uint8_t value;
	rc = sx126x_brd_reg_read(drv->api.board, SX126X_REGADDR_LR_BW500_WORKAROUND, &value, sizeof(value));
	SX126X_RETURN_IF_NONZERO(rc);

	value &= 0xFB;

	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_LR_BW500_WORKAROUND, &value, sizeof(value));
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
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

//! Костыль 3 для парирования залипания RX тайматов
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
	sx126x_chip_type_t pa_type;

	rc = sx126x_brd_get_chip_type(drv->api.board, &pa_type);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ищем ближайшую к определенной мощности
	const sx126x_pa_coeffs_t * coeffs = sx126x_defs_get_pa_coeffs(pa_power, pa_type);

	// Теперь, когда все параметры определены - загоняем их как есть
	// начинаем с set pa config
	rc = sx126x_api_set_pa_config(
			&drv->api, coeffs->duty_cycle, coeffs->hp_max, coeffs->device_sel, coeffs->pa_lut
	);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь делаем set_tx_params
	rc = sx126x_api_set_tx_params(&drv->api, coeffs->tx_params_power, ramp_time);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь проставляем оверкюрент
	rc = sx126x_brd_reg_write(drv->api.board, SX126X_REGADDR_OCP, &coeffs->ocp_value, sizeof(coeffs->ocp_value));
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


//! Применение конфига для TX
static int _apply_lora_tx_cfg(sx126x_drv_t * drv)
{
	int rc;
	const sx126x_drv_lora_tx_cfg_t * const config = &drv->_modem_state.lora.tx_cfg;

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

	return 0;
}


//! Применение лоровоского конфига для RX
static int _apply_lora_rx_cfg(sx126x_drv_t * drv)
{
	int rc;
	const sx126x_drv_lora_rx_cfg_t * const config = &drv->_modem_state.lora.rx_cfg;
	const bool explicit_header = (0 == config->payload_length);

	// Ставим параметры пакетизации
	const sx126x_lora_packet_params_t packet_params = {
			.preamble_length = config->preamble_length,
			.explicit_header = explicit_header,
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

	// Активируем symbnum таймаут
	rc = sx126x_api_set_lora_symb_num_timeout(&drv->api, config->rx_timeout_symbs);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Разрешаем остановку RX таймера по преамбуле
	rc = sx126x_api_stop_rx_timer_on_preamble(&drv->api, config->rx_timeout_stop_on_preamble);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
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

	return 0;
}



// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


int sx126x_drv_ctor(sx126x_drv_t * drv, void * board_ctor_arg)
{
	int rc;
	memset(drv, 0, sizeof(*drv));

	rc = sx126x_api_ctor(&drv->api, board_ctor_arg);
	SX126X_RETURN_IF_NONZERO(rc);

	// Пока что мы ничего не настраивали и находимся в состоянии ошибки
	drv->state = SX126X_DRVSTATE_ERROR;
	drv->_default_standby = SX126X_STANDBY_RC;
	drv->_evt_handler = _default_evt_handler;

	// Просто это нужно выставить в какое-то корректное значение
	drv->_modem_state.lora.bw = SX126X_LORA_BW_007;
	// Это значение по-умолчанию
	drv->_modem_state.lora.cad_exit_mode = SX126X_LORA_CAD_ONLY;
	return 0;
}


void sx126x_drv_dtor(sx126x_drv_t * drv)
{
	sx126x_api_dtor(&drv->api);
}


int sx126x_drv_register_event_handler(sx126x_drv_t * drv, sx126x_evt_handler_t handler, void * cb_user_arg)
{
	drv->_evt_handler = handler;
	drv->_evt_handler_user_arg = cb_user_arg;

	return 0;
}


int sx126x_drv_reset(sx126x_drv_t * drv)
{
	int rc;

	sx126x_brd_disable_irq(drv->api.board);

	rc = sx126x_brd_reset(drv->api.board);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	sx126x_brd_enable_irq(drv->api.board);

	rc = _set_antenna(drv, SX126X_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->state = SX126X_DRVSTATE_STANDBY_RC;
	return 0;
}


int sx126x_drv_mode_standby_rc(sx126x_drv_t * drv)
{
	int rc;

	rc = sx126x_api_set_standby(&drv->api, SX126X_STANDBY_RC);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = _set_antenna(drv, SX126X_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->state = SX126X_DRVSTATE_STANDBY_RC;
	return 0;
}


int sx126x_drv_mode_standby(sx126x_drv_t * drv)
{
	int rc;

	rc = sx126x_api_set_standby(&drv->api, drv->_default_standby);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = _set_antenna(drv, SX126X_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->state = SX126X_DRVSTATE_STANDBY_DEFAULT;
	return 0;
}


int sx126x_drv_mode_rx(sx126x_drv_t * drv)
{
	int rc;
	// Выставляем указатели буфера
	rc = sx126x_api_buffer_base_address(&drv->api, 0, 0);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Вкатываем соответствующие настройки
	rc = _apply_lora_rx_cfg(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем антенну
	rc = _set_antenna(drv, SX126X_ANTENNA_RX);
	SX126X_RETURN_IF_NONZERO(rc);

	// Запускаем rx операцию
	const uint32_t timeout = drv->_modem_state.lora.rx_cfg.rx_timeout_ms;

	rc = sx126x_api_set_rx(&drv->api, timeout);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->state = SX126X_DRVSTATE_RX;
	return 0;
}


int sx126x_drv_mode_tx(sx126x_drv_t * drv)
{
	int rc;
	// Вкатываем настройки
	rc = _apply_lora_tx_cfg(drv);
	SX126X_RETURN_IF_NONZERO(rc);

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
	const uint32_t timeout = drv->_modem_state.lora.tx_cfg.tx_timeout_ms;
	rc = sx126x_api_set_tx(&drv->api, timeout);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	drv->state = SX126X_DRVSTATE_TX;
	return 0;
}


int sx126x_drv_mode_cad(sx126x_drv_t * drv)
{
	int rc;

	rc = sx126x_api_set_cad(&drv->api);
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

	if (SX126X_PACKET_TYPE_LORA  == drv->_modem_type && !drv->_modem_state.lora.rx_cfg.payload_length != 0)
	{
		// Если используется лора с неявным заголовком, то размер пакета мы и так знаем
		payload_size = drv->_modem_state.lora.rx_cfg.payload_length;
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
	int rc;
	// Выгребаем
	rc = sx126x_brd_buf_read(drv->api.board, 0, buffer, buffer_size);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


int sx126x_drv_configure_basic(sx126x_drv_t * drv, const sx126x_drv_basic_cfg_t * config)
{
	int rc;
	// TODO: Проверка на то, что мы сейчас можем давать такие команды

	// В первую очередь определяемся с регулятором
	sx126x_regulator_t regulator_mode = config->allow_dcdc ? SX126X_REGULATOR_DCDC : SX126X_REGULATOR_LDO;
	rc = sx126x_api_set_regulator_mode(&drv->api, regulator_mode);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = _wait_busy(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь включаем TCXO
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
	rc = sx126x_api_set_rxtx_fallback_mode(&drv->api, config->standby_mode);
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
	return 0;
}


int sx126x_drv_configure_lora_modem(sx126x_drv_t * drv, const sx126x_drv_lora_modem_cfg_t * config)
{
	int rc;
	// TODO: Проверка на то, что такие команды мы можем здесь выдавать

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

	drv->_modem_state.lora.bw = config->bandwidth;
	return 0;
}


int sx126x_drv_configure_lora_cad(sx126x_drv_t * drv, const sx126x_drv_cad_cfg_t * config)
{
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

	drv->_modem_state.lora.cad_exit_mode = config->exit_mode;
	return 0;
}


int sx126x_drv_set_lora_tx_config(sx126x_drv_t * drv, const sx126x_drv_lora_tx_cfg_t * config)
{
	drv->_modem_state.lora.tx_cfg = *config;
	return 0;
}


int sx126x_drv_set_lora_rx_config(sx126x_drv_t * drv, const sx126x_drv_lora_rx_cfg_t * config)
{
	drv->_modem_state.lora.rx_cfg = *config;
	return 0;
}


static int _consider_next_state(sx126x_drv_t * drv, int state_switch_flags)
{
	int rc;
	if ((state_switch_flags & SX126X_SSF_TX) == 0 && drv->tx_packet_size)
	{
		// Если пакет ожидающий отправки должен быть отправлен только в ответ на полученный
		if (drv->tx_buffer_flags & SX126X_DRV_TXCB_FLAGS_SEND_AS_PONG)
		{
			// Проверяем, что у нас не было таймаутов и только тогда отправляем
			if (0 == (state_switch_flags & (SX126X_SSF_HW_TIMEOUT | SX126X_SSF_SW_TIMEOUT)))
				rc = _start_tx(drv);
		}
		else
		{
			// В противном случае просто отправляем
			rc = _start_tx(drv);
		}
	}
	else
	{
		rc = _start_rx(drv);
	}

	return rc;
}


int sx126x_drv_poll(sx126x_drv_t * drv)
{
	int rc;
	sx126x_evt_arg_t evt_arg;
	uint16_t irq_status;

	// Получим irq флаги
	rc = _fetch_clear_irq(drv, &irq_status);
	SX126X_RETURN_IF_NONZERO(rc);

	if (irq_status & SX126X_IRQ_CAD_DONE)
	{
		evt_arg.cad_done.cad_detected = (irq_status & SX126X_IRQ_CAD_DETECTED) ? true : false;
		drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVTKIND_CAD_DONE, &evt_arg);

		if (drv->_modem_state.lora.cad_exit_mode == SX126X_LORA_CAD_RX)
			drv->state = SX126X_DRVSTATE_RX;
		else
			drv->state = SX126X_DRVSTATE_STANDBY_DEFAULT;
	}

	if (irq_status & SX126X_IRQ_RX_DONE)
	{
		evt_arg.rx_done.packet_valid = (irq_status & SX126X_IRQ_CRC_ERROR) ? false : true;
		// Сбегаем к чипу и достанем статистику пакета
		if (SX126X_PACKET_TYPE_LORA == drv->_modem_type)
		{
			rc = sx126x_api_get_lora_packet_status(&drv->api, &evt_arg.rx_done.packet_status.lora);
			SX126X_RETURN_IF_NONZERO(rc);
		}
		else if (SX126X_PACKET_TYPE_GFSK == drv->_modem_type)
		{
			rc = sx126x_api_get_lora_packet_status(&drv->api, &evt_arg.rx_done.packet_status.gfsk);
			SX126X_RETURN_IF_NONZERO(rc);
		}

		rc = _workaround_3_rx_timeout(drv);
		SX126X_RETURN_IF_NONZERO(rc);

		drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVT_KIND_RX_DONE, &evt_arg);
		drv->state = SX126X_DRVSTATE_STANDBY_DEFAULT;
	}

	if (irq_status & SX126X_IRQ_TX_DONE)
	{
		drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVT_KIND_TX_DONE, &evt_arg);
		drv->state = SX126X_DRVSTATE_STANDBY_DEFAULT;
	}

	if (irq_status & SX126X_IRQ_TIMEOUT)
	{
		drv->_evt_handler(drv, drv->_evt_handler_user_arg, SX126X_EVTKIND_TIMEOUT, &evt_arg);
		drv->state = SX126X_DRVSTATE_STANDBY_DEFAULT;
	}

	switch (drv->state)
	{
	default: // В таком состоянии мы быть вообще не должны
		rc = SX126X_ERROR_BAD_STATE;
		break;

	case SX126X_DRVSTATE_ERROR: // Мы в состоянии ошибки..
		break;

	case SX126X_DRVSTATE_TX: // Мы отправляем пакет
		// Закончили отправку?
		if (irq_status & SX126X_IRQ_TX_DONE)
		{
			_ack_tx(drv, 0);
			SX126X_BREAK_IF_NONZERO(rc);

			rc = _consider_next_state(drv, SX126X_SSF_TX);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		else if (irq_status & SX126X_IRQ_TIMEOUT)
		{
			rc = _consider_next_state(drv, SX126X_SSF_TX | SX126X_SSF_HW_TIMEOUT);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		else if (_sw_timeout_time_spent(drv) > drv->tx_timeout_soft)
		{
			rc = _consider_next_state(drv, SX126X_SSF_TX | SX126X_SSF_SW_TIMEOUT);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		break;

	case SX126X_DRVSTATE_LBT_RX:
		// Мы сейчас заняты приёмом. Он закончился?
		if (irq_status & SX126X_IRQ_CAD_DONE)
		{
			// LBT этап закончился
			if (0 == (irq_status & SX126X_IRQ_CAD_DETECTED))
			{
				// Канал свободен, можно кричать!
				rc = _consider_next_state(drv, SX126X_SSF_RX | SX126X_SSF_HW_TIMEOUT);
				SX126X_BREAK_IF_NONZERO(rc);
			}
			else
			{
				// Значит сейчас идет приём пакета. Подождем
			}
		}
		if (irq_status & (SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERROR))
		{
			// Ухты, мы получили пакет!
			rc = _workaround_3_rx_timeout(drv);
			SX126X_BREAK_IF_NONZERO(rc);

			rc = _fetch_rx(drv, irq_status & SX126X_IRQ_CRC_ERROR ? SX126X_DRV_RXCB_FLAGS_BAD_CRC : 0);
			SX126X_BREAK_IF_NONZERO(rc);

			rc = _consider_next_state(drv, SX126X_SSF_RX);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		else if (irq_status & SX126X_IRQ_TIMEOUT)
		{
			// Нет, случился аппаратный таймаут
			rc = _workaround_3_rx_timeout(drv);
			SX126X_BREAK_IF_NONZERO(rc);

			rc = _consider_next_state(drv, SX126X_SSF_RX | SX126X_SSF_HW_TIMEOUT);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		else if (_sw_timeout_time_spent(drv) > drv->rx_timeout_soft)
		{
			// Нет, случился программный таймаут
			rc = _workaround_3_rx_timeout(drv);
			SX126X_BREAK_IF_NONZERO(rc);

			rc = _consider_next_state(drv, SX126X_SSF_RX | SX126X_SSF_SW_TIMEOUT);
			SX126X_BREAK_IF_NONZERO(rc);
		}
		else
		{
			int8_t rssi;
			rc = sx126x_api_get_rssi_inst(&drv->plt, &rssi);
			//printf("rssi = %d\n", (int)rssi);
		}
		break;
	}; // switch

	if (0 != rc)
	{
		// Проваливаемся в состояние ошибки
		drv->state = SX126X_DRVSTATE_ERROR;
		return rc;
	}

	// Работаем с колбеками
	rc = _dispatch_callbacks(drv);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


