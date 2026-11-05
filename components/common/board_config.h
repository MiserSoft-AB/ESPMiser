#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/* Board-specific configurations for Waveshare LCD 7b
 * Defines physical pins and other limitiations  */

#ifdef __cplusplus
extern "C" {
#endif


/*------------------- UART Config -------------------*/
#define UART_PORT UART_NUM_1
#define UART_TX_PIN 43
#define UART_RX_PIN 44

/*------------------- I2C Config --------------------*/
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_PIN 8
#define I2C_MASTER_SCL_PIN 9
#define I2C_MASTER_FREQ_HZ 400000 // increase?
#define I2C_ADDR_BME280    0x77

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BOARD_CONFIG_H__
