idf_component_register(SRCS "ui.cpp" "fonts/notosans_14p.c"
  INCLUDE_DIRS "." 
  PRIV_REQUIRES lvgl esp_lvgl_adapter
  REQUIRES common display 
)
