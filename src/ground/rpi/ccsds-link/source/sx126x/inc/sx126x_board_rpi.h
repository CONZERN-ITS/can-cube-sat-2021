#ifndef SX126X_BOARD_RPI_H_
#define SX126X_BOARD_RPI_H_


//! Параметры конструктора для "платы" малины
typedef struct sx126x_board_rpi_ctor_arg_t
{
	//! Путь до spidev устройства, на котором мы работаем
	const char * dev_path;
} sx126x_board_rpi_ctor_arg_t;


#endif /* SX126X_BOARD_RPI_H_ */
