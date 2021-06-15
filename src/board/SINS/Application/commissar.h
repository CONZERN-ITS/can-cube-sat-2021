#ifndef INC_COMMISSAR_H_
#define INC_COMMISSAR_H_

#include <its/mavlink.h>

typedef enum commissar_subordinate_t
{
	COMMISSAR_SUB_I2C_LINK,

	COMMISSAR__SUBS_COUNT
} commissar_subordinate_t;


void commissar_init(void);

void commissar_accept_report(commissar_subordinate_t who, int error_code);

void commissar_provide_report(uint8_t * component_id, mavlink_commissar_report_t * report);

void commissar_work(void);


#endif /* INC_COMMISSAR_H_ */
