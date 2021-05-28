
#ifndef ITS_I2C_LINK_CONF_H_
#define ITS_I2C_LINK_CONF_H_

#include <stm32f4xx_hal.h>

//! Размер пакета передаваемого/принимаемого за одну транзакцию
#define I2C_LINK_PACKET_SIZE	(279)

//! Количество приёмных буферов (каждый по I2C_LINK_PACKET_SIZE байт)
#define I2C_LINK_RXBUF_COUNT	(5)

//! Количество отправных буферов (каждый по I2C_LINK_PACKET_SIZE байт)
#define I2C_LINK_TXBUF_COUNT (5)

//! LL i2c-handle (I2C_Typedef)
#define I2C_LINK_BUS_HANDLE (I2C1)
#define I2C_LINK_DMA_HANDLE (DMA1)
#define I2C_LINK_DMA_STREAM_TX (LL_DMA_STREAM_6)
#define I2C_LINK_DMA_STREAM_RX (LL_DMA_STREAM_0)

//! релиз резет для нашего i2c-handle
#define I2C_LINK_BUS_RELEASE_RESET (__HAL_RCC_I2C1_RELEASE_RESET())

//! форс резет для нашего i2c-handle
#define I2C_LINK_BUS_FORCE_RESET (__HAL_RCC_I2C1_FORCE_RESET())


#endif /* ITS_I2C_LINK_CONF_H_ */
