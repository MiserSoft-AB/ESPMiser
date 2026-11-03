idf_component_register(
  SRCS "display.cpp"
  INCLUDE_DIRS "."
  REQUIRES freertos log i2c_bus esp_lcd esp_lcd_touch_gt911 lvgl
  PRIV_REQUIRES common
)
