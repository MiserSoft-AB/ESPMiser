#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/* Board-specific configurations for Waveshare ESP32-S3 LCD-7B
 * Defines physical IO pins and other board specifications  */

#ifdef __cplusplus
extern "C" {
#endif

#include <driver/i2c_master.h>

/*------------------- UART Config -------------------*/

#define UART_PORT UART_NUM_1
#define UART_TX_PIN 43
#define UART_RX_PIN 44

/*------------------- I2C Config --------------------*/

#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_PIN    8
#define I2C_MASTER_SCL_PIN    9
#define I2C_MASTER_SDA_IO     GPIO_NUM_8
#define I2C_MASTER_SCL_IO     GPIO_NUM_9
#define I2C_MASTER_FREQ_HZ    400000
#define I2C_MASTER_TIMEOUT_MS 1000



#ifdef I2C_LEGACY_BUS_ENABLED
// Global i2c bus handle creation
// Pass NULL for default conf
esp_err_t board_i2c_bus_init(const i2c_config_t* i2c_conf);

// Get the global i2c bus handle if inited
// returns NULL if not inited properly
i2c_bus_handle_t board_i2c_bus_get();
#endif // I2C_LEGACY_BUS_ENABLED

esp_err_t board_i2c_master_init();
i2c_master_bus_handle_t board_i2c_master_bus_get();
bool board_i2c_master_is_ready();

// Device addresses
#define I2C_ADDR_BME280          0x77 // sensors
#define I2C_ADDR_GT911_PRIMARY   0x5D // touch panel
#define I2C_ADDR_GT911_SECONDARY 0x14 // touch panel
#define I2C_ADDR_CH422G          0x24 // backlight LED config

// #define I2C_ADDR_CH422G          0x38 // backlight LED main driver
#define CH422G_OUT 0x03
#define CH422G_IN  0x04
#define CH422G_PWM 0x05

/*------------------- LCD Config --------------------*/

// Resolution (pixels)
#define LCD_H_RES 1024
#define LCD_V_RES 600 

// Blue data signals
#define LCD_RGB_IO_DATA0  GPIO_NUM_14 // B3
#define LCD_RGB_IO_DATA1  GPIO_NUM_38 // B4
#define LCD_RGB_IO_DATA2  GPIO_NUM_18 // B5
#define LCD_RGB_IO_DATA3  GPIO_NUM_17 // B6
#define LCD_RGB_IO_DATA4  GPIO_NUM_10 // B7

// Green data signals
#define LCD_RGB_IO_DATA5  GPIO_NUM_39 // G2
#define LCD_RGB_IO_DATA6  GPIO_NUM_0  // G3
#define LCD_RGB_IO_DATA7  GPIO_NUM_45 // G4
#define LCD_RGB_IO_DATA8  GPIO_NUM_48 // G5
#define LCD_RGB_IO_DATA9  GPIO_NUM_47 // G6
#define LCD_RGB_IO_DATA10 GPIO_NUM_21 // G7

// Red data signals
#define LCD_RGB_IO_DATA11 GPIO_NUM_1  // R3
#define LCD_RGB_IO_DATA12 GPIO_NUM_2  // R4
#define LCD_RGB_IO_DATA13 GPIO_NUM_42 // R5
#define LCD_RGB_IO_DATA14 GPIO_NUM_41 // R6
#define LCD_RGB_IO_DATA15 GPIO_NUM_40 // R7

// Misc Signals
#define LCD_IO_RGB_DISP   -1
#define LCD_IO_RGB_VSYNC  GPIO_NUM_3
#define LCD_IO_RGB_HSYNC  GPIO_NUM_46
#define LCD_IO_RGB_DE     GPIO_NUM_5
#define LCD_IO_RGB_PCLK   GPIO_NUM_7

// Timings, sizes, etc.
#define LCD_PIXEL_CLOCK_HZ          16 * 1000 * 1000
#define LCD_RGB_BIT_PER_PIXEL       16
#define LCD_RGB_DATA_WIDTH          16
#define LCD_RGB_BOUNCE_LINES        10
#define LCD_RGB_BOUNCE_BUFFER_SIZE  LCD_H_RES * \
                                    LCD_RGB_BOUNCE_LINES * \
                                    LCD_RGB_BIT_PER_PIXEL \
                                    / 8

// GT911 Touch 
#define LCD_TOUCH_RESET_IO GPIO_NUM_4
#define LCD_TOUCH_PIN_NUM_RST -1
#define LCD_TOUCH_PIN_NUM_INT -1

#define LCD_HSYNC_PULSE_WIDTH 162
#define LCD_HSYNC_BACK_PORCH 152
#define LCD_HSYNC_FRONT_PORCH 48
#define LCD_VSYNC_PULSE_WIDTH 45
#define LCD_VSYNC_BACK_PORCH 13
#define LCD_VSYNC_FRONT_PORCH 3

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BOARD_CONFIG_H__
