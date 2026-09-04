idf_component_register(SRCS "esp_miser.cpp"
  INCLUDE_DIRS "."
  REQUIRES nvs_flash
  PRIV_REQUIRES http_client wifi
)
