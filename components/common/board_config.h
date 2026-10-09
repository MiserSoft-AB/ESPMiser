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


#ifdef __cplusplus
} // extern "C"
#endif

#endif // __BOARD_CONFIG_H__
