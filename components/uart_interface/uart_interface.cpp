#include "uart_interface.hpp"
#include "board_config.h"
#include "data.h"

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

    if (This->update_data() != ESP_OK)
      ESP_LOGW(TAG, "Failed to update data");

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
      i = 0; // Reset chunk len
      // Echo feedback
      // uart_write_bytes(UART_PORT, (char *)This->read_buf_, read_len);

      // Loop through each char and feed line_buf until newline or overflow
      for (; i < read_len; i++)
      {
        c = This->read_buf_[i];

        // Echo feedback
        uart_write_bytes(UART_PORT, &c, 1);

        if (c == '\r' || c == '\n')
        { // Finished command
          
          read_line_buf[cmd_len] = '\0';

          // Send line_buf to cmd parser
          Command Cmd = This->cmd_str_to_enum(read_line_buf);
          This->cmd_palette(Cmd);

          i = 0, cmd_len = 0; // reset
          read_line_buf[0] = '\0';
        }
        else if (i < READ_LINE_BUF_LEN - 1)
        { // Inputted character
          read_line_buf[cmd_len++] = c;
        }
        else
        { // Buffer overflow
          ESP_LOGW(TAG, "Read line buffer overflowed");
          i = 0, cmd_len = 0; // reset
          read_line_buf[0] = '\0';
        }
      }
    }
  }
}

esp_err_t UartInterface::update_data()
{
  if (UiQueue::receive(ui_data_, pdMS_TO_TICKS(100)) == 0) 
  {
    switch (ui_data_.type) 
    {
      case DATA_NONE:
        ESP_LOGI(TAG, "Data recieved with type none...");
        break;
      case DATA_LEOP:
        break;
      case DATA_SENSOR:
        break;
      case DATA_ALL:
      default:
        ESP_LOGW(TAG, "Unknown data recieved");
        break;
    }
  }
  else return ESP_FAIL;

  return ESP_OK;
}

void UartInterface::cmd_palette(Command Cmd)
{
  // Clear terminal
  // uart_write_bytes(UART_PORT, 0, READ_LINE_BUF_LEN);

  /*
  const char* help_text = \
                          "These are the available commands:"
                          " HELP: Display this message"
                          " STATUS: Get system status"
                          " LEOP: Get leop data"
                          " SENSOR: Get sensor readings"
                          " CONFIG: Set config (N/A)"
                          " WIFI: Get WiFi status";
  uart_write_bytes(UART_PORT, help_text, strlen(help_text));
*/
  switch (Cmd)
  {
    case CMD_HELP:
    {
      const char* help_text = \
                    "These are the available commands:\r\n"
                    " HELP: Display this message\r\n"
                    " STATUS: Get system status\r\n"
                    " LEOP: Get leop data\r\n"
                    " SENSOR: Get sensor readings\r\n"
                    " CONFIG: Set config (N/A)\r\n"
                    " WIFI: Get WiFi status\r\n";
      uart_write_bytes(UART_PORT, help_text, strlen(help_text));
      
    } break;
    case CMD_STATUS: 
    {

    } break;
    case CMD_LEOP:   
    {


    } break;
    case CMD_SENSOR: 
    {

    } break;
    case CMD_CONFIG: 
    {

    } break;
    case CMD_WIFI:
    {

    } break;
    default:         
      break;
  }
}


