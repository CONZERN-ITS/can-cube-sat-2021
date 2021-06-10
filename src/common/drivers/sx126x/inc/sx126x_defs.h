#ifndef RADIO_SX126X_DEFS_H_
#define RADIO_SX126X_DEFS_H_


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef enum sx126x_reg_addr_t
{
	SX126X_REGADDR_RX_GAIN					= 0x08AC,
	SX126X_REGADDR_XTA_TRIM					= 0x0911,
	SX126X_REGADDR_OCP						= 0x08E7,
	SX126X_REGADDR_LR_CRCSEEDBASEADDR		= 0x06BC,
	SX126X_REGADDR_LR_CRCPOLYBASEADDR		= 0x06BE,
	SX126X_REGADDR_LR_WHITSEEDBASEADDR_MSB	= 0x06B8,
	SX126X_REGADDR_LR_WHITSEEDBASEADDR_LSB	= 0x06B9,
	SX126X_REGADDR_LR_PACKETPARAMS			= 0x0704,
	SX126X_REGADDR_LR_PAYLOADLENGTH			= 0x0702,
	SX126X_REGADDR_LR_SYNCWORDBASEADDRESS	= 0x06C0,
	SX126X_REGADDR_LR_SYNCWORD				= 0x0740,

	SX126X_REGADDR_LR_BW500_WORKAROUND		= 0x0889,
	SX126X_REGADDR_TX_CLAMP_CONFIG			= 0x08D8,
	SX126X_REGADDR_RTC_CONTROL1				= 0x0902,
	SX126X_REGADDR_RTC_CONTROL2				= 0x0944,
	SX126X_REGADDR_LR_INVIQ_WORKAROUND		= 0x0736,
} sx126x_reg_addr_t;


typedef enum sx126x_rx_gain_t
{
	SX126X_RXGAIN_POWERSAVE		= 0x94,
	SX126X_RXGAIN_BOOSTED		= 0x96,
} sx126x_rx_gain_t;


typedef enum sx126x_cmd_t
{
	SX126X_CMD_GET_STATUS					= 0xC0,
	SX126X_CMD_WRITE_REGISTER				= 0x0D,
	SX126X_CMD_READ_REGISTER				= 0x1D,
	SX126X_CMD_WRITE_BUFFER					= 0x0E,
	SX126X_CMD_READ_BUFFER					= 0x1E,
	SX126X_CMD_SET_SLEEP					= 0x84,
	SX126X_CMD_SET_STANDBY					= 0x80,
	SX126X_CMD_SET_FS						= 0xC1,
	SX126X_CMD_SET_TX						= 0x83,
	SX126X_CMD_SET_RX						= 0x82,
	SX126X_CMD_SET_RXDUTYCYCLE				= 0x94,
	SX126X_CMD_SET_CAD						= 0xC5,
	SX126X_CMD_SET_TXCONTINUOUSWAVE			= 0xD1,
	SX126X_CMD_SET_TXCONTINUOUSPREAMBLE		= 0xD2,
	SX126X_CMD_SET_PACKETTYPE				= 0x8A,
	SX126X_CMD_GET_PACKETTYPE				= 0x11,
	SX126X_CMD_SET_RFFREQUENCY				= 0x86,
	SX126X_CMD_SET_TXPARAMS					= 0x8E,
	SX126X_CMD_SET_PACONFIG					= 0x95,
	SX126X_CMD_SET_CADPARAMS				= 0x88,
	SX126X_CMD_SET_BUFFERBASEADDRES			= 0x8F,
	SX126X_CMD_SET_MODULATIONPARAMS			= 0x8B,
	SX126X_CMD_SET_PACKETPARAMS				= 0x8C,
	SX126X_CMD_GET_RXBUFFERSTATUS			= 0x13,
	SX126X_CMD_GET_PACKETSTATUS				= 0x14,
	SX126X_CMD_GET_RSSIINST					= 0x15,
	SX126X_CMD_GET_STATS					= 0x10,
	SX126X_CMD_RESET_STATS					= 0x00,
	SX126X_CMD_CFG_DIOIRQ					= 0x08,
	SX126X_CMD_GET_IRQSTATUS				= 0x12,
	SX126X_CMD_CLR_IRQSTATUS				= 0x02,
	SX126X_CMD_CALIBRATE					= 0x89,
	SX126X_CMD_CALIBRATEIMAGE				= 0x98,
	SX126X_CMD_SET_REGULATORMODE			= 0x96,
	SX126X_CMD_GET_ERROR					= 0x17,
	SX126X_CMD_CLR_ERROR					= 0x07,
	SX126X_CMD_SET_TCXOMODE					= 0x97,
	SX126X_CMD_SET_RXTXFALLBACKMODE			= 0x93,
	SX126X_CMD_SET_RFSWITCHMODE				= 0x9D,
	SX126X_CMD_SET_STOPRXTIMERONPREAMBLE	= 0x9F,
	SX126X_CMD_SET_LORASYMBTIMEOUT			= 0xA0,
} sx126x_cmd_t;


typedef enum sx126x_standby_mode_t
{
	SX126X_STANDBY_RC	= 0x00,
	SX126X_STANDBY_XOSC	= 0x01,
} sx126x_standby_mode_t;


typedef enum sx126x_packet_type_t
{
	SX126X_PACKET_TYPE_GFSK	= 0x00,
	SX126X_PACKET_TYPE_LORA	= 0x01,
} sx126x_packet_type_t;


typedef enum sx126x_regulator_t
{
	SX126X_REGULATOR_LDO	= 0x00,
	SX126X_REGULATOR_DCDC	= 0x01
} sx126x_regulator_t;


typedef enum sx126x_fallback_mode_t
{
	SX126X_FALLBACK_FS		= 0x40,
	SX126X_FALLBACK_XOSC	= 0x30,
	SX126X_FALLBACK_RC		= 0x20,
} sx126x_fallback_mode_t;


typedef enum sx126x_irq_t
{
	SX126X_IRQ_TX_DONE			= (1 << 0),
	SX126X_IRQ_RX_DONE			= (1 << 1),
	SX126X_IRQ_PREABLE_DETECTED	= (1 << 2),
	SX126X_IRQ_SYNCWORD_VALID	= (1 << 3),
	SX126X_IRQ_HEADER_VALID		= (1 << 4),
	SX126X_IRQ_HEADER_ERROR		= (1 << 5),
	SX126X_IRQ_CRC_ERROR		= (1 << 6),
	SX126X_IRQ_CAD_DONE			= (1 << 7),
	SX126X_IRQ_CAD_DETECTED		= (1 << 8),
	SX126X_IRQ_TIMEOUT			= (1 << 9)
} sx126x_irq_t;


typedef enum sx126x_device_error_t
{
	SX126X_DEVICE_ERROR_RC64K_CALIB		= (1 << 0),
	SX126X_DEVICE_ERROR_RC13M_CALIB		= (1 << 1),
	SX126X_DEVICE_ERROR_PLL_CALIB		= (1 << 2),
	SX126X_DEVICE_ERROR_ADC_CALIB		= (1 << 3),
	SX126X_DEVICE_ERROR_IMG_CALIB		= (1 << 4),
	SX126X_DEVICE_ERROR_XOSC_START		= (1 << 5),
	SX126X_DEVICE_ERROR_PLL_LOCK		= (1 << 6),
									//	= (1 << 7), reserved
	SX126X_DEVICE_ERROR_PA_RAMP			= (1 << 8)
									//	= (1 << 9..15), reserved
} sx126x_device_error_t;


typedef enum sx126x_tcxo_v_t
{
	SX126X_TCXO_CTRL_1_6V	= 0x00,
	SX126X_TCXO_CTRL_1_7V	= 0x01,
	SX126X_TCXO_CTRL_1_8V	= 0x02,
	SX126X_TCXO_CTRL_2_2V	= 0x03,
	SX126X_TCXO_CTRL_2_4V	= 0x04,
	SX126X_TCXO_CTRL_2_7V	= 0x05,
	SX126X_TCXO_CTRL_3_0V	= 0x06,
	SX126X_TCXO_CTRL_3_3V	= 0x07,
} sx126x_tcxo_v_t;


typedef enum sx126x_ramp_time_t
{
	SX126X_PA_RAMP_10_US	= 0x00,
	SX126X_PA_RAMP_20_US	= 0x01,
	SX126X_PA_RAMP_40_US	= 0x02,
	SX126X_PA_RAMP_80_US	= 0x03,
	SX126X_PA_RAMP_200_US	= 0x04,
	SX126X_PA_RAMP_800_US	= 0x05,
	SX126X_PA_RAMP_1700_US	= 0x06,
	SX126X_PA_RAMP_3400_US	= 0x07,
} sx126x_pa_ramp_time_t;


typedef enum sx126x_preamble_detector_t
{
	SX126X_PREAMBLE_DETECTOR_OFF		= 0x00,
	SX126X_PREAMBLE_DETECTOR_08_BITS	= 0x04,
	SX126X_PREAMBLE_DETECTOR_16_BITS	= 0x05,
	SX126X_PREAMBLE_DETECTOR_24_BITS	= 0x06,
	SX126X_PREAMBLE_DETECTOR_32_BITS	= 0x07
} sx126x_preamble_detector_t;


typedef enum sx126x_address_comp_t
{
	SX126X_ADDRESSCOMP_FILT_OFF			= 0x00,
	SX126X_ADDRESSCOMP_FILT_NODE		= 0x01,
	SX126X_ADDRESSCOMP_FILT_NODE_BROAD	= 0x02,
} sx126x_address_comp_t;


typedef enum sx126x_packet_length_mode_t
{
	SX126X_PACKET_FIXED_LENGTH		= 0x00,
	SX126X_PACKET_VARIABLE_LENGTH	= 0x01,
} sx126x_packet_length_mode_t;


typedef enum sx126x_crc_mode_t
{
	SX126X_CRC_LEN_OFF			= 0x01,
	SX126X_CRC_LEN_1_BYTE		= 0x00,
	SX126X_CRC_LEN_2_BYTE		= 0x02,
	SX126X_CRC_LEN_1_BYTE_INV	= 0x04,
	SX126X_CRC_LEN_2_BYTE_INV	= 0x06,
} sx126x_crc_mode_t;


typedef enum sx126x_cad_length_t
{
	SX126X_LORA_CAD_01_SYMBOL	= 0x00,
	SX126X_LORA_CAD_02_SYMBOL	= 0x01,
	SX126X_LORA_CAD_04_SYMBOL	= 0x02,
	SX126X_LORA_CAD_08_SYMBOL	= 0x03,
	SX126X_LORA_CAD_16_SYMBOLS	= 0x04,
} sx126x_cad_length_t;


typedef enum sx126x_cad_exit_mode_t
{
	SX126X_LORA_CAD_ONLY	= 0x00,
	SX126X_LORA_CAD_RX		= 0x01,
	SX126X_LORA_CAD_LBT		= 0x10,
} sx126x_cad_exit_mode_t;


typedef enum sx126x_pulse_shape_t
{
	MOD_SHAPING_OFF		= 0x00,
	MOD_SHAPING_G_BT_03	= 0x08,
	MOD_SHAPING_G_BT_05	= 0x09,
	MOD_SHAPING_G_BT_07	= 0x0A,
	MOD_SHAPING_G_BT_1	= 0x0B,
} sx126x_pulse_shape_t;


typedef enum sx126x_rx_bw_t
{
	SX126X_RX_BW_4800	= 0x1F,
	SX126X_RX_BW_5800	= 0x17,
	SX126X_RX_BW_7300	= 0x0F,
	SX126X_RX_BW_9700	= 0x1E,
	SX126X_RX_BW_11700	= 0x16,
	SX126X_RX_BW_14600	= 0x0E,
	SX126X_RX_BW_19500	= 0x1D,
	SX126X_RX_BW_23400	= 0x15,
	SX126X_RX_BW_29300	= 0x0D,
	SX126X_RX_BW_39000	= 0x1C,
	SX126X_RX_BW_46900	= 0x14,
	SX126X_RX_BW_58600	= 0x0C,
	SX126X_RX_BW_78200	= 0x1B,
	SX126X_RX_BW_93800	= 0x13,
	SX126X_RX_BW_117300	= 0x0B,
	SX126X_RX_BW_156200	= 0x1A,
	SX126X_RX_BW_187200	= 0x12,
	SX126X_RX_BW_234300	= 0x0A,
	SX126X_RX_BW_312000	= 0x19,
	SX126X_RX_BW_373600	= 0x11,
	SX126X_RX_BW_467000	= 0x09,
} sx126x_rx_bw_t;


typedef enum sx126x_lora_sf_t
{
	SX126X_LORA_SF_5		= 0x05,
	SX126X_LORA_SF_6		= 0x06,
	SX126X_LORA_SF_7		= 0x07,
	SX126X_LORA_SF_8		= 0x08,
	SX126X_LORA_SF_9		= 0x09,
	SX126X_LORA_SF_10		= 0x0A,
	SX126X_LORA_SF_11		= 0x0B,
	SX126X_LORA_SF_12		= 0x0C,
} sx126x_lora_sf_t;


typedef enum sx126x_lora_bw_t
{
	SX126X_LORA_BW_500	= 0x06,
	SX126X_LORA_BW_250	= 0x05,
	SX126X_LORA_BW_125	= 0x04,
	SX126X_LORA_BW_062	= 0x03,
	SX126X_LORA_BW_041	= 0x0A,
	SX126X_LORA_BW_031	= 0x02,
	SX126X_LORA_BW_020	= 0x09,
	SX126X_LORA_BW_015	= 0x01,
	SX126X_LORA_BW_010	= 0x08,
	SX126X_LORA_BW_007	= 0x00,
} sx126x_lora_bw_t;


typedef enum sx126x_lora_cr_t
{
	SX126X_LORA_CR_4_5	= 0x01,
	SX126X_LORA_CR_4_6	= 0x02,
	SX126X_LORA_CR_4_7	= 0x03,
	SX126X_LORA_CR_4_8	= 0x04,
} sx126x_lora_cr_t;


typedef enum sx126x_lora_syncword_t
{
	SX126X_LORASYNCWORD_PRIVATE	= 0x1424,
	SX126X_LORASYNCWORD_PUBLIC	= 0x3444,
} sx126x_lora_syncword_t;


typedef enum sx126x_sleep_flags_t
{
	SX126x_SLEEP_FLAG_RTC_TIMEOUT	= (1 << 0),
								//	= (1 << 1) reserved
	SX126X_SLEEP_FLAG_WARM_START	= (1 << 2)
								//	= (1 << 3..7) reserved
} sx126x_sleep_flags_t;


typedef enum sx126x_calib_flags_t
{
	SX126X_CALIB_FLAG_RC64K	= (1 << 0),
	SX126X_CALIB_FLAG_RC13M	= (1 << 1),
	SX126X_CALIB_FLAG_PLL	= (1 << 2),
	SX126X_CALIB_FLAG_ADCP	= (1 << 3),
	SX126X_CALIB_FLAG_ADCBN	= (1 << 4),
	SX126X_CALIB_FLAG_ADCBP	= (1 << 6),
	SX126X_CALIB_FLAG_IMAGE	= (1 << 7),
						//	= (1 << 8) reserved
} sx126x_calib_flags_t;


typedef enum sx126x_antenna_mode_t
{
	SX126X_ANTENNA_OFF	= 0x00,
	SX126X_ANTENNA_RX,
	SX126X_ANTENNA_TX
} sx126x_antenna_mode_t;


typedef enum sx126x_chip_type_t
{
	SX126X_CHIPTYPE_SX1261,
	SX126X_CHIPTYPE_SX1262,
	SX126X_CHIPTYPE_SX1268,
} sx126x_chip_type_t;


typedef enum sx126x_error_t
{
	SX126X_ERROR_INVALID_FREQUENCY		= 0x01,
	SX126x_ERROR_UNEXPECTED_VALUE		= 0x02,
	SX126X_ERROR_BAD_STATE				= 0x03,
	SX126X_ERROR_INVALID_VALUE			= 0x04,
	SX126X_ERROR_BOARD					= 0x05,
	SX126X_ERROR_TIMEOUT				= 0x06,
	SX126X_ERROR_INTERNAL				= 0x07,
} sx126x_error_t;


typedef enum sx126x_status_chip_mode_t
{
	SX126X_STATUS_CHIPMODE_UNUNSED		= 0x00,
	SX126X_STATUS_CHIPMODE_RFU			= 0x01,
	SX126X_STATUS_CHIPMODE_STDBY_RC		= 0x02,
	SX126X_STATUS_CHIPMODE_STDBY_XOSC	= 0x03,
	SX126X_STATUS_CHIPMODE_FS			= 0x04,
	SX126X_STATUS_CHIPMODE_RX			= 0x05,
	SX126X_STATUS_CHIPMODE_TX			= 0x06,
} sx126x_status_chip_mode_t;


typedef enum sx126x_status_cmd_status_t
{
	SX126X_STATUS_CMD_STATUS_RESERVED		= 0x00,
	SX126X_STATUS_CMD_STATUS_RFU			= 0x01,
	SX126X_STATUS_CMD_STATUS_DATA_AVAIL		= 0x02,
	SX126X_STATUS_CMD_TIMEOUT				= 0x03,
	SX126X_STATUS_CMD_PROCESSING_ERROR		= 0x04,
	SX126X_STATUS_CMD_EXECUTION_FAILURE		= 0x05,
	SX126X_STATUS_CMD_TX_DONE				= 0x06,
} sx126x_status_cmd_status_t;


typedef union sx126x_status_t
{
	struct
	{
		uint8_t
			_reserved1:	1,
			cmd_status:	3, //!< sx126x_status_chip_mode_t
			chip_mode:	3, //!< sx126x_status_cmd_status_t
			_reserved2:	1;
	} bits;
	uint8_t value;
} sx126x_status_t;


typedef struct sx126x_gfsk_packet_params_t
{
	uint16_t preamble_length;
	sx126x_preamble_detector_t preamble_detector;
	uint8_t sync_word_length;
	sx126x_address_comp_t adress_comp;
	bool explicit_header;
	uint8_t payload_length;
	sx126x_crc_mode_t crc_length;
	bool enable_whitening;
} sx126x_gfsk_packet_params_t;


typedef struct sx126x_gfsk_modulation_params_t
{
	uint32_t bit_rate;
	uint32_t f_dev;
	sx126x_pulse_shape_t pulse_shape;
	sx126x_rx_bw_t bandwidth;
} sx126x_gfsk_modulation_params_t;


typedef struct sx126x_gfsk_packet_status_t
{
	uint8_t rx_status;
	int8_t rssi_avg;
	int8_t rssi_sync;
} sx126x_gfsk_packet_status_t;


typedef struct sx126x_lora_packet_params_t
{
	uint16_t preamble_length;
	bool explicit_header;
	uint8_t payload_length;
	bool use_crc;
	bool invert_iq;
} sx126x_lora_packet_params_t;


typedef struct sx126x_lora_modulation_params_t
{
	sx126x_lora_sf_t spreading_factor;
	sx126x_lora_bw_t bandwidth;
	sx126x_lora_cr_t coding_rate;
	bool ldr_optimizations;
} sx126x_lora_modulation_params_t;


typedef struct sx126x_lora_packet_status_t
{
	int8_t rssi_pkt;
	int8_t snr_pkt;
	int8_t signal_rssi_pkt;
} sx126x_lora_packet_status_t;


typedef struct sx126x_stats_t
{
	uint16_t pkt_received;
	uint16_t crc_errors;
	uint16_t hdr_errors;
} sx126x_stats_t;


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

//! Коэффициенты для установки мощности для sx1261
/*! For SX1261 at synthesis frequency above 400 MHz, paDutyCycle should not be higher than 0x07.
	For SX1261 at synthesis frequency below 400 MHz, paDutyCycle should not be higher than 0x04. */
#define SX126X_PA_COEFFS_1261 \
		{ 15, 0x06, 0x00, 0x01, 0x01, 14, 0x18 }, \
		{ 14, 0x04, 0x00, 0x01, 0x01, 14, 0x18 }, \
		{ 10, 0x01, 0x00, 0x01, 0x01, 13, 0x18 }

//! Коэффициенты для установки мощности для sx1262
/*! For SX1262, paDutyCycle should not be higher than 0x04. */
#define SX126X_PA_COEFFS_1262 \
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 }, \
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 }, \
		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 }, \
		{ 14, 0x02, 0x02, 0x00, 0x01, 22, 0x38 }

//! Коэффициенты для установки мощности для sx1268
/*! paDutyCycle should not be higher than 0x04 */
#define SX126X_PA_COEFFS_1268 \
		{ 22, 0x04, 0x07, 0x00, 0x01, 22, 0x38 }, \
		{ 20, 0x03, 0x05, 0x00, 0x01, 22, 0x38 }, \
		{ 17, 0x02, 0x03, 0x00, 0x01, 22, 0x38 }, \
		/* Эти два режима оптимальны для питания чипа от DC-DC ? */ \
		{ 14, 0x04, 0x06, 0x00, 0x01, 15, 0x18 }, \
		{ 10, 0x00, 0x03, 0x00, 0x01, 15, 0x18 }


#endif /* RADIO_SX126X_DEFS_H_ */
