idf_component_register(SRCS "esp_miser.cpp" "tasks/network_task.cpp" "tasks/sensor_task.cpp"
  INCLUDE_DIRS "."
  REQUIRES nvs_flash
  PRIV_REQUIRES http wifi
)
