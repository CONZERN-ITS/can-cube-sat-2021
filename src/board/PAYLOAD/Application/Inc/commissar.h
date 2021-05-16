#ifndef INC_COMMISSAR_H_
#define INC_COMMISSAR_H_


typedef enum commissar_subordinate_t
{
	COMMISSAR_SUB_BME280_INT = 0,
	COMMISSAR_SUB_BME280_EXT,
	COMMISSAR_SUB_MS5611_INT,
	COMMISSAR_SUB_MS5611_EXT,
	COMMISSAR_SUB_I2C_LINK,

	COMMISSAR__SUBS_COUNT
} commissar_subordinate_t;


void commissar_init(void);

void commissar_report(commissar_subordinate_t who, int error_code);

void commissar_work(void);


#endif /* INC_COMMISSAR_H_ */
