idf_component_register(SRCS "leop_fetcher.cpp" 
  INCLUDE_DIRS "." 
  REQUIRES esp_driver_gptimer
  PRIV_REQUIRES http common
)
