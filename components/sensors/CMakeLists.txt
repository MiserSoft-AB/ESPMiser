idf_component_register(SRCS "sensors_task.cpp" "sensors_update.cpp" "bme280.cpp" "bme280_adapter.cpp" "bme280_port/bme280.c"
  INCLUDE_DIRS "." 
  REQUIRES driver
  PRIV_REQUIRES common
)
