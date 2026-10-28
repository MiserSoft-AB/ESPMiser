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
#define I2C_MASTER_FREQ_HZ 400000 // increase? // INCREASE!!! to what tho? <R
#define I2C_ADDR_BME280    0x77

#define BOARD_I2C_SDA_PIN I2C_MASTER_SDA_PIN
#define BOARD_I2C_SCL_PIN I2C_MASTER_SCL_PIN

/*------------------- LCD Config --------------------*/
#define LCD_H_RES 1024
#define LCD_V_RES 600

#define LCD_HSYNC_PIN 46
#define LCD_VSYNC_PIN 3
#define LCD_DE_PIN 5
#define LCD_PCLK_PIN 7

/* RGB565 data pins, ordered from D0 to D15 */
#define LCD_DATA_PINS { \
    14, 38, 18, 17, 10, \
    39, 0, 45, 48, 47, 21, \
    1, 2, 42, 41, 40 \
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BOARD_CONFIG_H__
