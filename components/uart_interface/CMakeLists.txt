idf_component_register(SRCS "uart_interface.cpp" 
  INCLUDE_DIRS "." 
  REQUIRES esp_driver_uart
  PRIV_REQUIRES common
)
