#include "uart_interface.hpp"
#include "board_config.h"
#include "data.h"

#include <esp_log.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

static const char* TAG = "uart_interface";
static const int READ_LINE_BUF_LEN = 256; // For reading input commands

// Helper to format and send a response over UART
static void uart_send(const char* fmt, ...)
{
  char buf[512];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (len > 0)
    uart_write_bytes(UART_PORT, buf, len);
}

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
  if (DataQueue::get(&data_, pdMS_TO_TICKS(100)) < 0) 
  {
    ESP_LOGE(TAG, "Failed getting data from queue");
    return ESP_FAIL;
  }

  return ESP_OK;
}

void UartInterface::cmd_palette(Command Cmd)
{
  switch (Cmd)
  {
    case CMD_HELP:
    {
       uart_send("These are the available commands:\r\n");
       uart_send(" HELP: Display this message\r\n");
       uart_send(" STATUS: Get system status\r\n");
       uart_send(" LEOP: Get leop data\r\n");
       uart_send(" SENSOR: Get sensor readings\r\n");
       uart_send(" CONFIG: Set config (N/A)\r\n");
       uart_send(" WIFI: Get WiFi status\r\n");
    } break;
    case CMD_STATUS: 
    {
      uart_send("=== SYSTEM STATUS ===\r\n");
      uart_send("Error Flag : %s\r\n", data_.system.sys_status.error_flag ? "SET" : "CLEAR");
      uart_send("WDT Enabled: %s\r\n", data_.system.sys_status.wdt_enabled ? "YES" : "NO");
      uart_send("WiFi Enable: %s\r\n", data_.system.sys_status.wifi_enabled ? "YES" : "NO");
      uart_send("LEOP Conn  : %s\r\n", data_.system.sys_status.leop_conn ? "CONNECTED" : "DISCONNECTED");
      uart_send("Sensor Conn: %s\r\n", data_.system.sys_status.sensor_conn ? "CONNECTED" : "DISCONNECTED");
      uart_send("CPU Temp   : %.2f C\r\n", (double)data_.system.temp_cpu);
#ifdef TASK_DIAG_ENABLED
      uart_send("--- Tasks (%d) ---\r\n", (int)data_.system.tasks_n);
      for (size_t i = 0; i < data_.system.tasks_n && i < SYSTEM_DATA_MAX_TASKS; i++)
      {
        const Task_Data* t = &data_.system.tasks[i];
        uart_send(" [%d] %-16s stack=%u words state=%d cpu=%u%%\r\n",
                  (int)i,
                  t->name ? t->name : "(null)",
                  (unsigned int)t->stack_words_remaining,
                  (int)t->state,
                  (unsigned int)t->cpu_usage_pct);
      }
#endif // TASK_DIAG_ENABLED
    } break;
    case CMD_LEOP:   
    {
      uart_send("=== LEOP DATA ===\r\n");
      uart_send("Price: %.2f\r\n", (double)data_.leop.price);
    } break;
    case CMD_SENSOR: 
    {
      uart_send("=== SENSOR DATA ===\r\n");
      uart_send("Temperature: %.2f C\r\n", (double)data_.sensor.temperature);
      uart_send("Humidity   : %.2f %%\r\n", (double)data_.sensor.humidity);
      uart_send("Pressure   : %.2f hPa\r\n", (double)data_.sensor.pressure);
    } break;
    case CMD_CONFIG: 
    {
      uart_send("CONFIG: Not yet implemented\r\n");
    } break;
    case CMD_WIFI:
    {
      uart_send("=== WIFI DATA ===\r\n");
      uart_send("SSID    : %s\r\n", data_.wifi.ap_ssid ? data_.wifi.ap_ssid : "(none)");
      uart_send("Connected: %s\r\n", data_.wifi.ap_connected ? "YES" : "NO");
    } break;
    default:         
       uart_send("These are the available commands:\r\n");
       uart_send(" HELP: Display this message\r\n");
       uart_send(" STATUS: Get system status\r\n");
       uart_send(" LEOP: Get leop data\r\n");
       uart_send(" SENSOR: Get sensor readings\r\n");
       uart_send(" CONFIG: Set config (N/A)\r\n");
       uart_send(" WIFI: Get WiFi status\r\n");
      break;
  }
}


