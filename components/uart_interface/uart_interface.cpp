#include "uart_interface.hpp"
#include "board_config.h"

#include <esp_log.h>

static const char*  TAG = "uart_interface";

UartInterface::UartInterface(uart_config_t* uart_cfg) 
{
  if (uart_cfg)
    uart_cfg_ = uart_cfg;
  else
    uart_cfg_ = &uart_cfg_default;
}
UartInterface::~UartInterface() {}

esp_err_t UartInterface::uart_init(void)
{
  esp_err_t res;

  res = uart_set_pin(
    UART_PORT,
    UART_TX_PIN,        // TX GPIO PIN
    UART_RX_PIN,        // RX GPIO PIN
    UART_PIN_NO_CHANGE, // RTS (not in use)
    UART_PIN_NO_CHANGE  // CTS (not in use)
  );
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "uart_set_pin %i", res);
    return res;
  }

  res = uart_driver_install(
    UART_PORT,
    buf_size_ * 2,  // RX buffer
    0,              // TX buffer (none, blocking)
    0,              // Event queue size
    NULL,           // Event queue handle
    0               // Interrupt flags
  );
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "uart_driver_install %i", res);
    return res;
  }

  ESP_LOGI(TAG, "UART drivers initialized");

  return ESP_OK;
}
