idf_component_register(SRCS "sysmon_task.cpp" 
  INCLUDE_DIRS "." 
  REQUIRES freertos esp_driver_tsens 
)
