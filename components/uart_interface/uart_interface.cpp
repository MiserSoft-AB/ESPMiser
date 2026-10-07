#include "uart_interface.hpp"
#include "board_config.h"
#include "driver/uart.h"

#include <esp_log.h>

static const char* TAG = "uart_interface";
static const int READ_LINE_BUF_LEN = 256; // For reading input commands

UartInterface::UartInterface(uart_config_t* uart_cfg) 
{
  if (uart_cfg)
    uart_cfg_ = uart_cfg;
  else
    uart_cfg_ = &uart_cfg_default;

  // Allocate read buffer in PSRAM or internal heap if that fails
  read_buf_ = (uint8_t*)heap_caps_malloc(buf_size_, MALLOC_CAP_SPIRAM);
  if (!read_buf_)
  {
    ESP_LOGW(TAG, "Failed to allocate buffer to PSRAM, falling back to internal heap...");

    read_buf_ = (uint8_t*)malloc(buf_size_); // Fallback to internal heap
    if (!read_buf_) 
    {
      ESP_LOGE(TAG, "Out of heap memory entirely...");
      return;
    }
  }
  ESP_LOGI(TAG, "UartInterface has been created with read buffer size: %zu", buf_size_);

}
UartInterface::~UartInterface() {}

esp_err_t UartInterface::uart_init(void)
{
  esp_err_t res;

  // Install drivers
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

  // Set config parameters
  res = uart_param_config(UART_PORT, uart_cfg_);
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "uart_param_config %i", res);
    return res;
  }

  // Set pin layout (defined in board_config.h)
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

  ESP_LOGI(TAG, "UART initialized");

  return ESP_OK;
}

void UartInterface::uart_interface_task(void* param)
{
  UartInterface* This = static_cast<UartInterface*>(param);
  if (!This || !This->read_buf_)
  {
    ESP_LOGE(TAG, "Invalid param passed to task");
    return;
  }

  static char read_line_buf[READ_LINE_BUF_LEN];
  char c;
  int i = 0, read_len = 0, cmd_len = 0;

  while (1)
  {
    // uart_write_bytes(UART_PORT, "Hello\n", 7);
    // vTaskDelay(pdMS_TO_TICKS(500));

    // Read input
    read_len = uart_read_bytes(
      UART_PORT, 
      This->read_buf_, 
      This->buf_size_,
      pdMS_TO_TICKS(10)
    );

    // Handle input
    if (read_len > 0)
    {
      // Echo feedback
      uart_write_bytes(UART_PORT, (char *)This->read_buf_, read_len);


      // Loop through each char and feed line_buf until newline
      for (; i < read_len; i++)
      {
        cmd_len++;
        c = This->read_buf_[i];

        if (c == '\n' || c == '\r')
        { // Finished command
          read_line_buf[i] = '\0';
          // Send line_buf to cmd parser
          // TODO: Parse input, execute from command palette and build response
          // This should be a new function
          i = 0, cmd_len = 0; // reset
        }
        else if (i < READ_LINE_BUF_LEN)
        { // Inputted character
          read_line_buf[i] = c;
        }
        else
        { // Buffer overflow
          ESP_LOGW(TAG, "Read line buffer overflowed");
          i = 0, cmd_len = 0; // reset
        }
      }
    }
  }
}
