idf_component_register(SRCS "esp_miser.cpp"
  INCLUDE_DIRS "."
  REQUIRES  esp_psram nvs_flash
  PRIV_REQUIRES http wifi sysmon ui display leop common sensors
)

target_compile_options(${COMPONENT_LIB} PRIVATE
-Wall # Standardvarningar
# -Wextra # Ytterligare varningar
# -Wpedantic # Strikt standardefterlevnad
# -Wshadow # Variabelmaskering
# -Wconversion # Implicita konverteringar
# -Werror # Behandla varningar som fel
)
