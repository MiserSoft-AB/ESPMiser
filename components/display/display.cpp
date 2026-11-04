#include "display.hpp"
#include "board_config.h"

#include <cstdint>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "portmacro.h"

namespace 
{
    /* The CH422G output command at 0x38 may not ACK. The new I2C master
     * driver reports that as ESP_ERR_INVALID_STATE / "unexpected nack",
     * but the command is still accepted by the expander. */
    constexpr esp_err_t CH422G_OUTPUT_NACK = ESP_ERR_INVALID_STATE;

    constexpr char TAG[] = "display";

} // Namespace

esp_err_t Display::backlight_init()
{
  if (!board_i2c_master_is_ready()) 
    return ESP_ERR_INVALID_STATE;
  
  if (bl_cfg_dev_ != nullptr && bl_out_dev_ != nullptr) 
    return ESP_OK;
  
  /* CH422G uses its I2C address as the command selector:
   * - 0x24: mode command
   * - 0x38: EXIO0..7 output command
   * Each transaction is one data byte; there is no register-address byte. */
  const i2c_device_config_t mode_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = I2C_ADDR_CH422G_CFG, /* 0x24 */
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };

  const i2c_device_config_t output_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = I2C_ADDR_CH422G, /* 0x38 */
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };

  ESP_LOGD(TAG, "Adding backlight cfg i2c device");
  esp_err_t ret = i2c_master_bus_add_device(
    board_i2c_master_bus_get(),
    &mode_cfg,
    &bl_cfg_dev_);

  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to add CH422G mode device: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Setting backlight output mode");
  // Set output mode
  const uint8_t mode_set[] = { 0x02, 0xFF };
  ret = i2c_master_transmit(
    bl_cfg_dev_,
    mode_set,
    sizeof(mode_set),
    I2C_MASTER_TIMEOUT_MS);

  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to set CH422G output mode: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Adding backlight output i2c device");
  ret = i2c_master_bus_add_device(
    board_i2c_master_bus_get(),
    &output_cfg,
    &bl_out_dev_);
  
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to add CH422G output device: %s", esp_err_to_name(ret));
    return ret;
  }

  return ESP_OK;
}

esp_err_t Display::backlight_set(uint8_t value)
{
  if (bl_cfg_dev_ == nullptr) 
    return ESP_ERR_INVALID_STATE;
  

  // Vendor/register-addressed CH422G form:
  // device address 0x24, then command byte + data byte.
  const uint8_t output_cmd[] = { 0x03, value };

  esp_err_t err = i2c_master_transmit(
    bl_cfg_dev_,
    output_cmd,
    sizeof(output_cmd),
    I2C_MASTER_TIMEOUT_MS);

  if (err != ESP_OK) 
    ESP_LOGE(TAG, "CH422G output write failed: %s", esp_err_to_name(err));

  return err;
}

esp_err_t Display::touch_init()
{
  if (!board_i2c_master_is_ready()) 
    return ESP_ERR_INVALID_STATE;

  // Configure GPIO
  const gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << LCD_TOUCH_RESET_IO),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };

  esp_err_t ret = gpio_config(&io_conf);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to configure touch reset GPIO %d: %s",
             (int)LCD_TOUCH_RESET_IO, esp_err_to_name(ret));
    return ret;
  }
  
  ESP_LOGD(TAG, "Initializing backlight");
  /* The backlight must be initialized and the GT911 reset sequence
   * must run before the GT911 driver is created. */
  ret = backlight_init();
  if (ret != ESP_OK) 
    return ret;

  ESP_LOGD(TAG, "Resetting touch");
  ret = touch_reset();
  if (ret != ESP_OK) 
    return ret;
  

  tp_io_cfg_ = {
    .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
    .control_phase_bytes = 1,
    .dc_bit_offset = 0,
    .lcd_cmd_bits = 16,
    .flags = {
      .disable_control_phase = 1,
    },
    /* CHANGED: the new esp_lcd I2C implementation requires an explicit SCL
     * frequency; use 100 kHz for reliable GT911 communication. */
    .scl_speed_hz = 100000,
  };

  ret = esp_lcd_new_panel_io_i2c(
    board_i2c_master_bus_get(),
    &tp_io_cfg_,
    &tp_io_handle_);

  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to create new i2c handle for touch panel: %s",
             esp_err_to_name(ret));
    return ret;
  }

  esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
    .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
  };

  const esp_lcd_touch_config_t tp_cfg = {
    .x_max = LCD_H_RES,
    .y_max = LCD_V_RES,
    .rst_gpio_num = (gpio_num_t)-1,
    .int_gpio_num = (gpio_num_t)-1,
    .levels = {
      .reset = 0,
      .interrupt = 0,
    },
    .flags = {
      .swap_xy = 0,
      .mirror_x = 0,
      .mirror_y = 0,
    },
    // .driver_data = &tp_gt911_config,
  };

  ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle_, &tp_cfg, &tp_handle_);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to create GT911 driver: %s", esp_err_to_name(ret));
    return ret;
  }

  return ESP_OK;
}

esp_err_t Display::touch_reset()
{
  if (bl_out_dev_ == nullptr) 
    return ESP_ERR_INVALID_STATE;
  
  esp_err_t ret;

  ESP_LOGD(TAG, "Reset setting backlight");
  ret = backlight_set(0x0C);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "CH422G touch-reset stage 1 failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Rom delay");
  esp_rom_delay_us(100 * 1000);

  ESP_LOGD(TAG, "Setting gpio level");
  gpio_set_level(LCD_TOUCH_RESET_IO, 0);

  ESP_LOGD(TAG, "Rom delay");
  esp_rom_delay_us(100 * 1000);

  ESP_LOGD(TAG, "Reset setting backlight");
  ret = backlight_set(0x0E);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "CH422G touch-reset stage 2 failed: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Rom delay");
  esp_rom_delay_us(200 * 1000);

  return ESP_OK;
}

esp_err_t Display::rgb_init()
{
  const uint8_t num_fbs = 2; // Number of framebuffers to allocate

  esp_lcd_rgb_panel_config_t panel_config = {
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .timings =  {
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .h_res = LCD_H_RES,
      .v_res = LCD_V_RES,
      .hsync_pulse_width = 4,
      .hsync_back_porch  = 8,
      .hsync_front_porch = 8,
      .vsync_pulse_width = 4,
      .vsync_back_porch  = 8,
      .vsync_front_porch = 8,
      .flags = {
        .pclk_active_neg = 1,
      },
    },
    .data_width = LCD_RGB_DATA_WIDTH,
    .bits_per_pixel = LCD_RGB_BIT_PER_PIXEL,
    .num_fbs = num_fbs,
    .bounce_buffer_size_px = LCD_RGB_BOUNCE_BUFFER_SIZE,
    // .sram_trans_align = 4,
    // .psram_trans_align = 64,
    .hsync_gpio_num = LCD_IO_RGB_HSYNC,
    .vsync_gpio_num = LCD_IO_RGB_VSYNC,
    .de_gpio_num    = LCD_IO_RGB_DE,
    .pclk_gpio_num  = LCD_IO_RGB_PCLK,
    .disp_gpio_num  = LCD_IO_RGB_DISP,
    .data_gpio_nums = {
      LCD_RGB_IO_DATA0,
      LCD_RGB_IO_DATA1,
      LCD_RGB_IO_DATA2,
      LCD_RGB_IO_DATA3,
      LCD_RGB_IO_DATA4,
      LCD_RGB_IO_DATA5,
      LCD_RGB_IO_DATA6,
      LCD_RGB_IO_DATA7,
      LCD_RGB_IO_DATA8,
      LCD_RGB_IO_DATA9,
      LCD_RGB_IO_DATA10,
      LCD_RGB_IO_DATA11,
      LCD_RGB_IO_DATA12,
      LCD_RGB_IO_DATA13,
      LCD_RGB_IO_DATA14,
      LCD_RGB_IO_DATA15,
    },
    .flags = {
      .fb_in_psram = 1,
    },
  };

  if (esp_lcd_new_rgb_panel(&panel_config, &rgb_handle_) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create LCD panel handle");
    return ESP_FAIL;
  }
  if (esp_lcd_panel_init(rgb_handle_) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to init LCD panel");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t Display::init()
{
  if (!board_i2c_master_is_ready()) 
  {
    ESP_LOGW(TAG, "Global i2c master not inited, trying to init");
    if (board_i2c_master_init() != ESP_OK) 
    {
      ESP_LOGE(TAG, "Could not init i2c master");
      return ESP_FAIL;
    }
  }

  ESP_LOGD(TAG, "Initializing RGB panel");

  // RGB panel first
  if (rgb_init() != ESP_OK) {
    ESP_LOGE(TAG, "RGB display panel failed to init");
    return ESP_FAIL;
  }

  ESP_LOGD(TAG, "Initializing Touch");
  /* touch_init() performs CH422G initialization and the
   * complete GT911 reset sequence itself, so do not call backlight_init()
   * separately here. */
  if (touch_init() != ESP_OK) 
  {
    ESP_LOGE(TAG, "Touch panel failed to init");
    return ESP_FAIL;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  ESP_LOGD(TAG, "Setting initial backlight");
  // Set backlight
  if (backlight_set(0x1E) != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to set backlight");
    return ESP_FAIL;
  }

  ready_ = true;
  return ESP_OK;
}

