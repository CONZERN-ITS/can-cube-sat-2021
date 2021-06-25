#ifndef INC_COMMISSAR_H_
#define INC_COMMISSAR_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
// Тут генерируется множество вот таких сообщений
// warning: taking address of packed member of 'struct __mavlink_vision_speed_estimate_t' may result in an unaligned pointer value [-Waddress-of-packed-member]
// Мы доверяем мавлинку в том, что он не сгенерит ничего невыровненого, поэтому давим эти варнинги
#include <its/mavlink.h>
#pragma GCC diagnostic pop


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
