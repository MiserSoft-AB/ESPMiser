#ifndef __UART_INTERFACE_HPP__
#define __UART_INTERFACE_HPP__

#include "ui_queue.hpp"

#include <esp_err.h>
#include <driver/uart.h>
#include <cstring>

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

  // Pass nullptr to use default config
  UartInterface(uart_config_t* uart_cfg);
  ~UartInterface();

  // No copy, no move
  UartInterface(const UartInterface&) = delete;
  UartInterface& operator=(const UartInterface&) = delete;
  UartInterface(UartInterface&&) noexcept = delete;
  UartInterface& operator=(UartInterface&&) noexcept = delete;

  // Must be run succesfully before task start
  esp_err_t uart_init(void);

  // Task function (needs class instance passed as param)
  static void uart_interface_task(void* param);

  esp_err_t update_data();

  /*** Command definitions ***/
  enum Command
  {
    CMD_HELP, CMD_STATUS, CMD_LEOP, CMD_SENSOR, CMD_CONFIG, CMD_WIFI, CMD_DIAG, 
    CMD_UNDEFINED = -1
  };

  // Returns string equivalent of Command enum choice
  static const char* cmd_enum_to_str(Command cmd)
  {
    switch (cmd)
    {
      case CMD_HELP:   return "HELP";
      case CMD_STATUS: return "STATUS";
      case CMD_LEOP:   return "LEOP";
      case CMD_SENSOR: return "SENSOR";
      case CMD_CONFIG: return "CONFIG";
      case CMD_WIFI:   return "WIFI";
      case CMD_DIAG:   return "DIAG";
      default:         return NULL;
    }
  };

  // Returns Command enum equivalent of string choice
  static Command cmd_str_to_enum(const char* input)
  {
    UartInterface::Command Cmd = UartInterface::CMD_UNDEFINED;
    
    if (strcmp(input, cmd_enum_to_str(CMD_HELP)) == 0)
      Cmd = CMD_HELP;
    else if (strcmp(input, cmd_enum_to_str(CMD_STATUS)) == 0)
      Cmd = CMD_STATUS;
    else if (strcmp(input, cmd_enum_to_str(CMD_LEOP)) == 0)
      Cmd = CMD_LEOP;
    else if (strcmp(input, cmd_enum_to_str(CMD_SENSOR)) == 0)
      Cmd = CMD_SENSOR;
    else if (strcmp(input, cmd_enum_to_str(CMD_CONFIG)) == 0)
      Cmd = CMD_CONFIG;
    else if (strcmp(input, cmd_enum_to_str(CMD_WIFI)) == 0)
      Cmd = CMD_WIFI;
    else if (strcmp(input, cmd_enum_to_str(CMD_DIAG)) == 0)
      Cmd = CMD_DIAG;

    return Cmd;
  }

  /* Executes the given command function */
  void cmd_palette(Command Cmd);
  

private:
  const uart_config_t* uart_cfg_;

  // Buffer members
  uint8_t*       read_buf_ = nullptr;
  const uint16_t buf_size_ = 1024;

  Share_Data  ui_data_;

};

#endif // __UART_INTERFACE_HPP__
