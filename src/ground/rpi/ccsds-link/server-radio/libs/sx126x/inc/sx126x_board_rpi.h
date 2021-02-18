#ifndef SX126X_INCLUDE_SX126X_BOARD_RPI_H_
#define SX126X_INCLUDE_SX126X_BOARD_RPI_H_

#include <stdint.h>

#include <sx126x_board.h>

// Расширение API платы - позволяет получить дескриптор события
// для обработки прерываний
int sx126x_brd_rpi_get_event_fd(sx126x_board_t * brd);


#endif /* SX126X_INCLUDE_SX126X_BOARD_RPI_H_ */
