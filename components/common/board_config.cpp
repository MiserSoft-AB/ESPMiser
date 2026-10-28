#include "board_config.h"

static i2c_bus_handle_t g_i2c_bus = nullptr;

esp_err_t board_i2c_bus_init(const i2c_config_t* i2c_conf)
{
  if (!i2c_conf)
  {
    i2c_config_t i2c_conf_default = {
      .mode          = I2C_MODE_MASTER,
      .sda_io_num    = I2C_MASTER_SDA_PIN,
      .scl_io_num    = I2C_MASTER_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master        = { 
        .clk_speed = I2C_MASTER_FREQ_HZ, 
      },
    };

    g_i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &i2c_conf_default);
  }
  else
    g_i2c_bus = i2c_bus_create(I2C_MASTER_NUM, i2c_conf);

  if (g_i2c_bus)
    return ESP_OK;
  else
    return ESP_FAIL;
}

i2c_bus_handle_t board_i2c_bus_get() { return g_i2c_bus; }
