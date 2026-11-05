idf_component_register(SRCS "sensors_task.cpp" "bme280.cpp" "bme280_port/bme280.c"
  INCLUDE_DIRS "." 
  REQUIRES driver
  PRIV_REQUIRES common
)
