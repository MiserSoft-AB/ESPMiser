#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/* Board-specific configurations for Waveshare ESP32-S3 LCD-7B
 * Defines physical IO pins and other board specifications  */

#ifdef __cplusplus
extern "C" {
#endif

#include <i2c_bus.h>

/*------------------- UART Config -------------------*/

#define UART_PORT UART_NUM_1
#define UART_TX_PIN 43
#define UART_RX_PIN 44

/*------------------- I2C Config --------------------*/

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_PIN 8
#define I2C_MASTER_SCL_PIN 9
#define I2C_MASTER_FREQ_HZ 400000 // increase?

// Global i2c bus handle creation
// Pass NULL for default conf
esp_err_t board_i2c_bus_init(const i2c_config_t* i2c_conf);

// Get the global i2c bus handle if inited
// returns NULL if not inited properly
i2c_bus_handle_t board_i2c_bus_get();

// Device addresses
#define I2C_ADDR_BME280 0x77          // sensors
#define I2C_ADDR_GT911_PRIMARY 0x5D   // touch panel
#define I2C_ADDR_GT911_SECONDARY 0x14 // touch panel

/*------------------- LCD Config --------------------*/

// Resolution (pixels)
#define LCD_H_RES 1024
#define LCD_V_RES 600 

// Blue data signals
#define LCD_RGB_IO_0  GPIO_NUM_14 // B3
#define LCD_RGB_IO_1  GPIO_NUM_38 // B4
#define LCD_RGB_IO_2  GPIO_NUM_18 // B5
#define LCD_RGB_IO_3  GPIO_NUM_17 // B6
#define LCD_RGB_IO_4  GPIO_NUM_10 // B7

// Green data signals
#define LCD_RGB_IO_5  GPIO_NUM_39 // G2
#define LCD_RGB_IO_6  GPIO_NUM_0  // G3
#define LCD_RGB_IO_7  GPIO_NUM_45 // G4
#define LCD_RGB_IO_8  GPIO_NUM_48 // G5
#define LCD_RGB_IO_9  GPIO_NUM_47 // G6
#define LCD_RGB_IO_10 GPIO_NUM_21 // G7

// Red data signals
#define LCD_RGB_IO_11 GPIO_NUM_1  // R3
#define LCD_RGB_IO_12 GPIO_NUM_2  // R4
#define LCD_RGB_IO_13 GPIO_NUM_42 // R5
#define LCD_RGB_IO_14 GPIO_NUM_41 // R6
#define LCD_RGB_IO_15 GPIO_NUM_40 // R7


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BOARD_CONFIG_H__
