#include "board_config.h"
#include "esp_err.h"


/*** I2C ***/

// Legacy i2c_bus, worked awfully for display, refactor legacy libs instead
#ifdef I2C_LEGACY_BUS_ENABLED

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

#else

static bool g_i2c_ready = false;
static i2c_master_bus_handle_t g_i2c_bus = nullptr;

esp_err_t board_i2c_master_init()
{
  if (g_i2c_ready) 
    return ESP_OK;
  
  esp_err_t ret;

  // const i2c_config_t i2c_conf = {
  //   .mode          = I2C_MODE_MASTER,
  //   .sda_io_num    = I2C_MASTER_SDA_PIN,
  //   .scl_io_num    = I2C_MASTER_SCL_PIN,
  //   .sda_pullup_en = GPIO_PULLUP_ENABLE,
  //   .scl_pullup_en = GPIO_PULLUP_ENABLE,
  //   .master = { 
  //     .clk_speed = I2C_MASTER_FREQ_HZ,
  //   },
  // };
  //
  // esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
  // if (ret != ESP_OK) 
  //   return ret;
  //
  // ret = i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0);
  // if (ret != ESP_OK || ret != ESP_ERR_INVALID_STATE) // Invalid state? 
  //   return ret;

  i2c_master_bus_config_t bus_cfg = {
    .i2c_port   = I2C_MASTER_NUM,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .clk_source = I2C_CLK_SRC_DEFAULT, // Correct?
    .glitch_ignore_cnt = 7,
    .flags = { 
      .enable_internal_pullup = true, 
    },
  };

  ret = i2c_new_master_bus(&bus_cfg, & g_i2c_bus);

  if (ret == ESP_OK)
    g_i2c_ready = true;

  return ret;
}

bool board_i2c_master_is_ready() { return g_i2c_ready; }

i2c_master_bus_handle_t board_i2c_master_bus_get()
{
  if (g_i2c_bus)
    return g_i2c_bus;
  else
    return nullptr;
}

/*** PSRAM ***/

// static bool g_psram_enabled = false;
//
// esp_err_t board_psram_enable() { return g_i2c_ready; }
//
// esp_err_t board_psram_init() 
// { 
//   ESP_ERROR_CHECK(esp_psram_init());
//   if (!esp_psram_is_initialized()) 
//     ESP_LOGW(TAG, "PSRAM not found.\n");
//
//   g_psram_enabled = true;
//
//   return err;
// }

#endif // I2C_LEGACY_BUS_ENABLED

