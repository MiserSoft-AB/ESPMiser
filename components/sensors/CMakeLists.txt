idf_component_register(SRCS "sensors_task.cpp" "bme280.cpp"
  INCLUDE_DIRS "." 
  REQUIRES bme280 driver
  PRIV_REQUIRES common
)
