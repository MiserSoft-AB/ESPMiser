idf_component_register(
  SRCS "board_config.cpp" "data_queue.cpp"
  INCLUDE_DIRS "."
  REQUIRES i2c_bus
)
