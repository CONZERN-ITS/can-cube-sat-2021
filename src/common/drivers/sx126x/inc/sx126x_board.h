#ifndef RADIO_SX126X_BOARD_H_
#define RADIO_SX126X_BOARD_H_

#include "sx126x_defs.h"


#define SX126X_BRD_NOP (0x00)


struct sx126x_board_t;
typedef struct sx126x_board_t sx126x_board_t;


int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg);
void sx126x_brd_dtor(sx126x_board_t * brd);

int sx126x_brd_get_time(sx126x_board_t * brd, uint32_t * value);

int sx126x_brd_get_chip_type(sx126x_board_t * brd, sx126x_chip_type_t * chip_type);

int sx126x_brd_reset(sx126x_board_t * brd);

int sx126x_brd_wait_on_busy(sx126x_board_t * brd, uint32_t timeout);

int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode);

int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size);
int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size);

int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size);
int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size);

int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size);
int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size);



#endif /* RADIO_SX126X_BOARD_H_ */
