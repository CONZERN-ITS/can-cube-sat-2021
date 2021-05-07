#ifndef INC_MICS6814_H_
#define INC_MICS6814_H_

#include "main.h"

#include "mavlink_main.h"


void mics6814_power_on();
void mics6814_power_off();

//! Настройка балансирующих резисторов в основном
/*! АЦП должно быть уже включено */
void mics6814_init(void);

//! Собираем все данные с mics6814
int mics6814_read(mavlink_pld_mics_6814_data_t * msg);


#endif /* INC_MICS6814_H_ */
