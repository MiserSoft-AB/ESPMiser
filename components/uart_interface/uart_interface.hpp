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

  // Pass nullptr to use default config
  UartInterface(uart_config_t* uart_cfg);   
  ~UartInterface();

  // No copy, no move
  UartInterface(const UartInterface&) = delete;
  UartInterface& operator=(const UartInterface&) = delete;
  UartInterface(UartInterface&&) noexcept = delete;
  UartInterface& operator=(UartInterface&&) noexcept = delete;

  esp_err_t uart_init(void);

private:
  const uart_config_t* uart_cfg_;
  const uint16_t       buf_size_ = 1024;

};

#endif // __UART_INTERFACE_HPP__
