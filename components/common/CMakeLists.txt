idf_component_register(
  SRCS "ui_queue.cpp" "board_i2c.cpp"
  INCLUDE_DIRS "."
  REQUIRES esp_driver_i2c
)
