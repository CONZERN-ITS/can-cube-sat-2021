#ifndef RADIO_SX126X_PLATFORM_H_
#define RADIO_SX126X_PLATFORM_H_

#include <stdint.h>

#include "sx126x_defs.h"


typedef struct sx126x_plt_t
{
	void * user_arg;
} sx126x_plt_t;


int sx126x_plt_ctor(sx126x_plt_t * plt, void * user_arg);
void sx126x_plt_dtor(sx126x_plt_t * plt);

int sx126x_plt_get_pa_type(sx126x_plt_t * plt, sx126x_chip_type_t * pa_type);

int sx126x_plt_reset(sx126x_plt_t * plt);

int sx126x_plt_wait_on_busy(sx126x_plt_t * plt);

void sx126x_plt_enable_irq(sx126x_plt_t * plt);
void sx126x_plt_disable_irq(sx126x_plt_t * plt);

int sx126x_plt_antenna_mode(sx126x_plt_t * plt, sx126x_antenna_mode_t mode);

int sx126x_plt_cmd_write(sx126x_plt_t * plt, uint8_t cmd_code, const uint8_t * args, uint16_t args_size);
int sx126x_plt_cmd_read(sx126x_plt_t * plt, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size);

int sx126x_plt_reg_write(sx126x_plt_t * plt, uint16_t addr, const uint8_t * data, uint16_t data_size);
int sx126x_plt_reg_read(sx126x_plt_t * plt, uint16_t addr, uint8_t * data, uint16_t data_size);

int sx126x_plt_buf_write(sx126x_plt_t * plt, uint8_t offset, const uint8_t * data, uint8_t data_size);
int sx126x_plt_buf_read(sx126x_plt_t * plt, uint8_t offset, uint8_t * data, uint8_t data_size);


#endif /* RADIO_SX126X_PLATFORM_H_ */
