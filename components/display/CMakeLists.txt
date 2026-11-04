idf_component_register(
  SRCS "display.cpp"
  INCLUDE_DIRS "."
  REQUIRES freertos log driver esp_lcd esp_lcd_touch_gt911 lvgl esp_lvgl_adapter
  PRIV_REQUIRES common
)
