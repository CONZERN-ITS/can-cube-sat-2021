#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sx126x_api.h"
#include "sx126x_drv.h"

#define SX126X_TCXO_STARTUP_TIMEOUT_MS (10)

#define SX126X_RETURN_IF_NONZERO(rc) if (0 != rc) return rc
#define SX126X_BREAK_IF_NONZERO(rc) if (0 != rc) break


//! Структура параметров, при помощи которых задается выходная мощность передатчика
typedef struct sx126x_pa_coeffs_t
{
	// Значение мощности, которые получится на выходе
	int8_t power;

	//! Параметр set_pa_config
	uint8_t duty_cycle;
	//! Параметр set_pa_config
	uint8_t hp_max;
	//! Параметр set_pa_config
	uint8_t device_sel;
	//! Параметр set_pa_config
	uint8_t pa_lut;
	//! Параметр для передачи в set_tx_params
	int8_t tx_params_power;
	//! Параметр для over current protection регистра
	uint8_t ocp_value;
} sx126x_pa_coeffs_t;


// Коэффициенты для установки мощности для sx1261
// Не должны быть использованы для частоты ниже 400мГц
static const sx126x_pa_coeffs_t _coeffs_sx1261[] = {
		// For SX1261 at synthesis frequency above 400 MHz, paDutyCycle should not be higher than 0x07.
		// For SX1261 at synthesis frequency below 400 MHz, paDutyCycle should not be higher than 0x04.
		{ 10, 0x01, 0x00, 0x01, 0x01, 13, 0x18 },
		{ 14, 0x04, 0x00, 0x01, 0x01, 14, 0x18 },
		{ 15, 0x06, 0x00, 0x01, 0x01, 14, 0x18 },
};

// Коэффициенты для установки мощности для sx1262
// Не должны быть использованы для частоты ниже 400мГц
static const sx126x_pa_coeffs_t _coeffs_sx1262[] = {
		// For SX1262, paDutyCycle should not be higher than 0x04.
		{ 14, 0x02, 0x02, 0x00, 0x01, 22, 0x38 },
		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 },
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 },
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 },
};

// Коэффициенты для установки мощности для sx1268
// Не должны быть использованы для частоты ниже 400мГц
static const sx126x_pa_coeffs_t _coeffs_sx1268[] = {
		// Эти два режима оптимальны для питания чипа от DC-DC ?
		{ 10, 0x00, 0x03, 0x00, 0x01, 15, 0x18 },
		{ 14, 0x04, 0x06, 0x00, 0x01, 15, 0x18 },

		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 },
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 },
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 },
};


//! Ищет структуру параметров мощности, целевая мощность которой (power) ближе всего к указанному значению
static const sx126x_pa_coeffs_t * _get_pa_coeffs(int8_t power, sx126x_chip_type_t pa_type)
{
	const sx126x_pa_coeffs_t * coeffs = NULL;
	size_t coeffs_count = 0;

	switch (pa_type)
	{
	case SX126X_CHIPTYPE_SX1261:
		coeffs = _coeffs_sx1261;
		coeffs_count = sizeof(_coeffs_sx1261)/sizeof(_coeffs_sx1261[0]);
		break;

	case SX126X_CHIPTYPE_SX1262:
		coeffs = _coeffs_sx1262;
		coeffs_count = sizeof(_coeffs_sx1262)/sizeof(_coeffs_sx1262[0]);
		break;

	case SX126X_CHIPTYPE_SX1268:
		coeffs = _coeffs_sx1268;
		coeffs_count = sizeof(_coeffs_sx1268)/sizeof(_coeffs_sx1268[0]);
		break;
	};

	if (NULL == coeffs)
	{
		// Нам дали какой-то не тот тип PA
		return NULL;
	}

	// Ищем ближайшее число к указанному в массиве данные коэффициентов
	size_t best_idx = 0;
	int16_t best_diff = abs(power - coeffs[0].power);

	for (size_t i = 1; i < coeffs_count; i++)
	{
		uint16_t diff = abs(power - coeffs[i].power);
		if (diff < best_diff)
		{
			best_diff = diff;
			best_idx = i;
		}
	}

	return coeffs + best_idx;
}


//! Настройка PA усилителя. Довольно громоздкая конструкция, поэтому вынесена отдельно
static int _configure_pa(sx126x_dev_t * dev, int8_t pa_power, const sx126x_pa_ramp_time_t ramp_time)
{
	int rc;
	sx126x_chip_type_t pa_type;

	rc = sx126x_plt_get_pa_type(&dev->plt, &pa_type);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ищем ближайшую к определенной мощности
	const sx126x_pa_coeffs_t * coeffs = _get_pa_coeffs(pa_power, pa_type);

	// Теперь, когда все параметры определены - загоняем их как есть
	// начинаем с set pa config
	rc = sx126x_api_set_pa_config(
			&dev->plt, coeffs->duty_cycle, coeffs->hp_max, coeffs->device_sel, coeffs->pa_lut
	);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь делаем set_tx_params
	rc = sx126x_api_set_tx_params(&dev->plt, coeffs->tx_params_power, ramp_time);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь проставляем оверкюрент
	rc = sx126x_plt_reg_write(&dev->plt, SX126X_REGADDR_OCP, &coeffs->ocp_value, sizeof(coeffs->ocp_value));
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


//! Установка опорной частоты
static int _set_rf_frequency(sx126x_dev_t * dev, uint32_t frequency)
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

	rc = sx126x_api_calibrate_image(&dev->plt, args);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_set_rf_frequency(&dev->plt, frequency);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


static int _fetch_clear_irq(sx126x_dev_t * dev, uint16_t * irq_status)
{
	int rc;

	*irq_status = 0;

	// Сходим к устройству и спросим как у него дела
	rc = sx126x_api_get_irq_status(&dev->plt, irq_status);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Сбросим все флаги прерывания, которые успели увидеть
	// не сбрасываем по 0xFFFF на случай, если что-то случилось пока мы не видели
	if (*irq_status)
	{
		rc = sx126x_api_clear_irq_status(&dev->plt, *irq_status);
		SX126X_RETURN_IF_NONZERO(rc);
		sx126x_plt_wait_on_busy(&dev->plt);
		SX126X_RETURN_IF_NONZERO(rc);
	}

	return 0;
}


// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


int sx126x_drv_ctor(sx126x_dev_t * dev, void * plt_init_user_arg)
{
	int rc;
	memset(dev, 0, sizeof(*dev));

	rc = sx126x_plt_ctor(&dev->plt, plt_init_user_arg);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_drv_reset(dev);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


void sx126x_drv_dtor(sx126x_dev_t * dev)
{
	sx126x_plt_dtor(&dev->plt);
}


int sx126x_drv_reset(sx126x_dev_t * dev)
{
	int rc;

	rc = sx126x_plt_antenna_mode(&dev->plt, SX126X_PLT_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	sx126x_plt_disable_irq(&dev->plt);

	rc = sx126x_plt_reset(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	sx126x_plt_enable_irq(&dev->plt);

	dev->state = SX126X_DRVSTATE_STANDBY_RC;
	return 0;
}


int sx126x_drv_mode_standby_rc(sx126x_dev_t * dev)
{
	int rc;
	// TODO: Проверка на то, что мы сейчас можем давать такие команды

	rc = sx126x_plt_antenna_mode(&dev->plt, SX126X_PLT_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_set_standby(&dev->plt, SX126X_STANDBY_RC);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	dev->state = SX126X_DRVSTATE_STANDBY_RC;
	return 0;
}


int sx126x_drv_configure_chip(sx126x_dev_t * dev, const sx126x_drv_chip_cfg_t * config)
{
	int rc;
	// TODO: Проверка на то, что мы сейчас можем давать такие команды

	// В первую очередь определяемся с регулятором
	sx126x_regulator_t regulator_mode = config->allow_dcdc ? SX126X_REGULATOR_DCDC : SX126X_REGULATOR_LDO;
	rc = sx126x_api_set_regulator_mode(&dev->plt, regulator_mode);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Затем включаем rf переключатель, если вдруг он нужен
	rc = sx126x_api_set_dio2_as_rf_switch(&dev->plt, config->use_dio2_for_rf_switch);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь включаем TCXO
	if (config->use_dio3_for_tcxo)
	{
		rc = sx126x_api_set_dio3_as_tcxo_ctl(&dev->plt, config->tcxo_v, SX126X_TCXO_STARTUP_TIMEOUT_MS);
		SX126X_RETURN_IF_NONZERO(rc);
		sx126x_plt_wait_on_busy(&dev->plt);
		SX126X_RETURN_IF_NONZERO(rc);
	}

	rc = sx126x_api_set_rxtx_fallback_mode(&dev->plt, config->rxtx_fallback_mode);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь включаем IRQ
	uint16_t irq_mask = SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR;
	rc = sx126x_api_set_dio_irq_params(&dev->plt, irq_mask, irq_mask, 0x00, 0x00);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// На этом готово
	return 0;
}


int sx126x_mode_standby_xosc(sx126x_dev_t * dev)
{
	int rc;
	// TODO: Проверка на то, что мы сейчас можем давать такие команды

	rc = sx126x_plt_antenna_mode(&dev->plt, SX126X_PLT_ANTENNA_OFF);
	SX126X_RETURN_IF_NONZERO(rc);

	rc = sx126x_api_set_standby(&dev->plt, SX126X_STANDBY_XOSC);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	dev->state = SX126X_DRVSTATE_STANDBY_XOSC;
	return 0;
}



int sx126x_drv_configure_lora(sx126x_dev_t * dev, const sx126x_drv_lora_cfg_t * config)
{
	// TODO: Проверка на то, что такие команды мы можем здесь выдавать
	int rc;

	// Перво-наперво, как велит даташит - делаем set_packet_type
	rc = sx126x_api_set_packet_type(&dev->plt, SX126X_PACKET_TYPE_LORA);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ставим частоту
	rc = _set_rf_frequency(dev, config->frequency);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь настраиваем мощность передачтика
	rc = _configure_pa(dev, config->pa_power, config->pa_ramp_time);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь ставим параметры модуляции
	const sx126x_lora_modulation_params_t mod_params = {
			.spreading_factor = config->spreading_factor,
			.bandwidth = config->bandwidth,
			.coding_rate = config->coding_rate,
			.ldr_optimizations = config->ldr_optimizations
	};
	rc = sx126x_api_set_lora_modulation_params(&dev->plt, &mod_params);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Ну и теперь параметры пакетизации
	const sx126x_lora_packet_params_t packet_params = {
			.preamble_length = config->preamble_length,
			.explicit_header = config->explicit_header,
			.payload_length = config->payload_length,
			.use_crc = config->use_crc,
			.invert_iq = config->invert_iq
	};
	rc = sx126x_api_set_lora_packet_params(&dev->plt, &packet_params);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Если у нас пакет с неявным заголовком, то rx таймер будет останавливать сразу
	// по получению одной лишь преамбулы без заголовков
	rc = sx126x_api_stop_rx_timer_on_preamble(&dev->plt, !config->explicit_header);
	//rc = sx126x_api_stop_rx_timer_on_preamble(&dev->plt, true);
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Теперь настроим буст LNA
	uint8_t reg_value = config->boost_rx_lna ? SX126X_RXGAIN_BOOSTED : SX126X_RXGAIN_POWERSAVE;
	rc = sx126x_plt_reg_write(&dev->plt, SX126X_REGADDR_RX_GAIN, &reg_value, sizeof(reg_value));
	SX126X_RETURN_IF_NONZERO(rc);
	sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Настраиваем синхрослово лоры (не совсем понятно что это значит)
	const uint8_t syncword_bytes[2] = {
			(config->syncword >> 8) & 0xFF,
			(config->syncword     ) & 0xFF
	};
	rc = sx126x_plt_reg_write(&dev->plt, SX126X_REGADDR_LR_SYNCWORD, syncword_bytes, sizeof(syncword_bytes));

	// Устанавливаем таймауты на приём/передачу пакета
	//dev->rx_timeout_ms = 3000; // TODO: Сделать расчет из параметров лоры
	dev->rx_timeout_ms = 2500; // TODO: Сделать расчет из параметров лоры
	dev->tx_timeout_ms = 0; // TODO: Сделать расчет из параметров лоры

	// Так же запоминаем всякие параметры пакета - они нам приодиться потом
	dev->payload_size = config->payload_length;
	dev->packet_type = SX126X_PACKET_TYPE_LORA;
	dev->explicit_lora_header = config->explicit_header;
	return 0;
}


int sx126x_drv_configure_callbacks(sx126x_dev_t * dev, const sx126x_drv_callback_cfg_t * config)
{
	dev->cb_user_arg = config->cb_user_arg;
	dev->onrx_callback = config->onrx_callback;
	dev->ontx_callback = config->ontx_callback;
	return 0;
}


int sx126x_drv_start(sx126x_dev_t * dev)
{
	int rc;

	// Прыгаем в standby_osc, если мы вдруг еще не в нем
	rc = sx126x_mode_standby_xosc(dev);
	SX126X_RETURN_IF_NONZERO(rc);

	// Чистим все флаги прерывания чипа
	rc = sx126x_api_clear_irq_status(&dev->plt, 0xFFFF);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Выставляем соответствующий статус
	dev->state = SX126X_DRVSTATE_LBT_IDLE;
	return 0;
}


static int _try_start_tx(sx126x_dev_t * dev, bool * tx_started)
{
	int rc;
	int packet_flags;
	size_t user_size;

	// Спросим у приложения
	user_size = dev->ontx_callback(
			dev->cb_user_arg, dev->payload_buffer, dev->payload_size, &packet_flags
	);

	if (!user_size)
	{
		// Приложение не хочет ничего передавать. Ну и чудненько
		*tx_started = false;
		return 0;
	}

	// Всетаки хочет. Окей, начинаем
	*tx_started = true;

	// Выставляем адрес буфера
	rc = sx126x_api_buffer_base_address(&dev->plt, 0, 0);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// затрем нулями то, что пользователь решил не отправлять
	memset(dev->payload_buffer+user_size, 0, dev->payload_size - user_size);
	// Загоняем пакет в радио
	rc = sx126x_plt_buf_write(&dev->plt, 0, dev->payload_buffer, dev->payload_size);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем антенну
	rc = sx126x_plt_antenna_mode(&dev->plt, SX126X_PLT_ANTENNA_TX);
	SX126X_RETURN_IF_NONZERO(rc);

	// Запускаем передачу пакета
	rc = sx126x_api_set_tx(&dev->plt, dev->tx_timeout_ms);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


static int _start_rx(sx126x_dev_t * dev)
{
	int rc;
	// Выставляем указатели буфера
	rc = sx126x_api_buffer_base_address(&dev->plt, 0, 0);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	// Включаем антенну
	rc = sx126x_plt_antenna_mode(&dev->plt, SX126X_PLT_ANTENNA_RX);
	SX126X_RETURN_IF_NONZERO(rc);

	// Запускаем rx операцию
	rc = sx126x_api_set_rx(&dev->plt, dev->rx_timeout_ms);
	SX126X_RETURN_IF_NONZERO(rc);
	rc = sx126x_plt_wait_on_busy(&dev->plt);
	SX126X_RETURN_IF_NONZERO(rc);

	return 0;
}


static int _get_rx_packet_size(sx126x_dev_t * dev, uint8_t * packet_size)
{
	int rc;
	uint8_t payload_size;
	uint8_t payload_offset;

	// Если используется лора с неявным заголовком, то размер пакета мы и так знаем
	if (SX126X_PACKET_TYPE_LORA  == dev->packet_type && !dev->explicit_lora_header)
	{
		*packet_size = dev->payload_size;
		return 0;
	}

	// Если нет - придется лезть в чип и спрашивать
	rc = sx126x_api_get_rx_buffer_status(&dev->plt, &payload_size, &payload_offset);
	SX126X_RETURN_IF_NONZERO(rc);
	// Не ждем busy

	*packet_size = payload_size;
	return 0;
}


int sx126x_drv_poll(sx126x_dev_t * dev)
{
	int rc;

	// Получим irq флаги
	uint16_t irq_status;
	rc = _fetch_clear_irq(dev, &irq_status);
	SX126X_RETURN_IF_NONZERO(rc);

	// TODO: Проверку на валидность состояния
	switch (dev->state)
	{
	case SX126X_DRVSTATE_ERROR:
		// Мы в состоянии ошибки..
		break;

	case SX126X_DRVSTATE_LBT_TX: // Мы отправяем пакет
		// Закончили отправку?
		if (irq_status & SX126X_IRQ_TX_DONE)
		{
			// Чудненько. Снова начинаем слушать эфир
			rc = _start_rx(dev);
			SX126X_BREAK_IF_NONZERO(rc);

			// меняем свое состояние
			dev->state = SX126X_DRVSTATE_LBT_RX;
		}
		else if (irq_status & SX126X_IRQ_TIMEOUT)
		{
			// Случилось что-то не то
			rc = SX126X_ERROR_BAD_STATE;
		}
		else
		{
			// Значит передача еще не закончилась
		}
		break;

	case SX126X_DRVSTATE_LBT_IDLE: // Мы только начали
		// Начинаем слушать эфир
		rc = _start_rx(dev);
		SX126X_BREAK_IF_NONZERO(rc);

		// меняем свое состояние
		dev->state = SX126X_DRVSTATE_LBT_RX;
		break;

	case SX126X_DRVSTATE_LBT_RX:
		// Мы сейчас заняты приёмом. Он закончился?
		if (irq_status & SX126X_IRQ_TIMEOUT)
		{
			// Закончился и закончился таймаутом
			// Значит мы можем что-то передать, если есть чего передавать
			bool tx_started;
			rc = _try_start_tx(dev, &tx_started);
			SX126X_BREAK_IF_NONZERO(rc);

			// Если пользователю было что передать и у нас получилось это сделать -
			// переходим в TX состояние
			if (tx_started)
			{
				dev->state = SX126X_DRVSTATE_LBT_TX;
			}
			else
			{
				// Если нет - сразу же запускаем очередной цикл получения
				rc = _start_rx(dev);
				SX126X_BREAK_IF_NONZERO(rc);
				// Состояние остается таким-же
			}
		}
		else if (irq_status & (SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERROR))
		{
			// Ухты, мы получили пакет! Смотрим какого он размера
			uint8_t rx_packet_size;
			rc = _get_rx_packet_size(dev, &rx_packet_size);
			SX126X_BREAK_IF_NONZERO(rc);

			// Выгребаем
			rc = sx126x_plt_buf_read(&dev->plt, 0, dev->payload_buffer, rx_packet_size);
			SX126X_BREAK_IF_NONZERO(rc);

			// Передаем пакет пользователю
			// TODO: Флаг о плохой контрольной сумме
			dev->onrx_callback(dev->cb_user_arg, dev->payload_buffer, rx_packet_size, 0);

			// Запускаем приём следующего пакета
			rc = _start_rx(dev);
			SX126X_BREAK_IF_NONZERO(rc);
			dev->state = SX126X_DRVSTATE_LBT_RX;
		}
		else
		{
			// Приём еще не закончился. Будем ждать
		}
		break;

	default:
		// В таком состоянии мы быть вообще не должны
		rc = SX126X_ERROR_BAD_STATE;
		break;

	}; // switch

	if (0 != rc)
	{
		// Проваливаемся в состояние ошибки
		dev->state = SX126X_DRVSTATE_ERROR;
		return rc;
	}

	return 0;
}


