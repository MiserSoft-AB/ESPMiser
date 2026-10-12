#ifndef __UART_INTERFACE_HPP__
#define __UART_INTERFACE_HPP__

#include <esp_err.h>
#include <driver/uart.h>

static const uart_config_t uart_cfg_default = {
  .baud_rate  = 115200,
  .data_bits  = UART_DATA_8_BITS,
  .parity     = UART_PARITY_DISABLE,
  .stop_bits  = UART_STOP_BITS_1,
  .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
  .source_clk = UART_SCLK_DEFAULT,
};

class UartInterface
{
public:

  enum Command
  {
    CMD_HELP, CMD_STATUS, CMD_LEOP, CMD_SENSOR, CMD_CONFIG, CMD_WIFI, CMD_DIAG, 
  };

  // Returns string equivalent of Command enum choice
  // Not sure why I made this, it looks neat I guess
  static const char* cmd_enum_to_string(Command cmd)
  {
    switch (cmd)
    {
      case CMD_HELP:   return "HELP";
      case CMD_STATUS: return "STATUS";
      case CMD_LEOP:   return "LEOP";
      case CMD_SENSOR: return "SENSOR";
      case CMD_CONFIG: return "CONFIG";
      case CMD_WIFI:   return "WIFI";
      default:         return NULL;
    }
  };

  // Pass nullptr to use default config
  UartInterface(uart_config_t* uart_cfg);
  ~UartInterface();

  // No copy, no move
  UartInterface(const UartInterface&) = delete;
  UartInterface& operator=(const UartInterface&) = delete;
  UartInterface(UartInterface&&) noexcept = delete;
  UartInterface& operator=(UartInterface&&) noexcept = delete;

  esp_err_t uart_init(void);

  // Task function (needs class instance passed as param)
  static void uart_interface_task(void* param);

private:
  const uart_config_t* uart_cfg_;

  // Buffer members
  uint8_t*       read_buf_ = nullptr;
  const uint16_t buf_size_ = 1024;
};

#endif // __UART_INTERFACE_HPP__
