#include <sx126x_board.h>


int sx126x_brd_ctor(sx126x_board_t ** brd, void * user_arg)
{

	return 0;
}


void sx126x_brd_dtor(sx126x_board_t * brd)
{

}


uint32_t sx126x_brd_get_time(sx126x_board_t * brd)
{
	return 0;
}


int sx126x_brd_get_chip_type(sx126x_board_t * brd, sx126x_chip_type_t * chip_type)
{
	return 0;
}


int sx126x_brd_reset(sx126x_board_t * brd)
{
	return 0;
}


int sx126x_brd_wait_on_busy(sx126x_board_t * brd)
{
	return 0;
}


void sx126x_brd_enable_irq(sx126x_board_t * brd)
{
	
}


void sx126x_brd_disable_irq(sx126x_board_t * brd)
{
	
}


int sx126x_brd_antenna_mode(sx126x_board_t * brd, sx126x_antenna_mode_t mode)
{
	return 0;
}


int sx126x_brd_cmd_write(sx126x_board_t * brd, uint8_t cmd_code, const uint8_t * args, uint16_t args_size)
{
	return 0;
}


int sx126x_brd_cmd_read(sx126x_board_t * brd, uint8_t cmd_code, uint8_t * status, uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_reg_write(sx126x_board_t * brd, uint16_t addr, const uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_reg_read(sx126x_board_t * brd, uint16_t addr, uint8_t * data, uint16_t data_size)
{
	return 0;
}


int sx126x_brd_buf_write(sx126x_board_t * brd, uint8_t offset, const uint8_t * data, uint8_t data_size)
{
	return 0;
}


int sx126x_brd_buf_read(sx126x_board_t * brd, uint8_t offset, uint8_t * data, uint8_t data_size)
{
	return 0;
}
