idf_component_register(
  SRCS "display.cpp" "display_adapter.cpp" "display_update.cpp"
  INCLUDE_DIRS "."
  REQUIRES freertos log driver esp_lcd esp_lcd_touch_gt911 lvgl
  PRIV_REQUIRES common
)
