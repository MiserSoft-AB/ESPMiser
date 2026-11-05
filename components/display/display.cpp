#include "display.hpp"
#include "display_hal.hpp"
#include "display_update.hpp"
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
    constexpr uint32_t DISPLAY_REFRESH_MS = 2000;

    struct DisplayBuffer
    {
        uint32_t refresh_count;
    };

    DisplayBuffer display_buffer{};

    TaskHandle_t display_task_handle = nullptr;

    void display_timer_callback(TimerHandle_t timer)
    {
        (void)timer;

        if (display_task_handle != nullptr)
        {
            xTaskNotifyGive(display_task_handle);
        }
    }

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

  esp_err_t ret = i2c_master_bus_add_device(
    board_i2c_master_bus_get(),
    &mode_cfg,
    &bl_cfg_dev_);

  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to add CH422G mode device: %s", esp_err_to_name(ret));
    return ret;
  }

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

  touch_reset_gpio_configured_ = true;
  
  /* The backlight must be initialized and the GT911 reset sequence
   * must run before the GT911 driver is created. */
  ret = backlight_init();
  if (ret != ESP_OK) 
    return ret;

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

  esp_err_t ret = backlight_set(0x2C);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "CH422G touch-reset stage 1 failed: %s", esp_err_to_name(ret));
    return ret;
  }

  esp_rom_delay_us(100 * 1000);

  gpio_set_level(LCD_TOUCH_RESET_IO, 0);

  esp_rom_delay_us(100 * 1000);

  ret = backlight_set(0x2E);
  if (ret != ESP_OK) 
  {
    ESP_LOGE(TAG, "CH422G touch-reset stage 2 failed: %s", esp_err_to_name(ret));
    return ret;
  }

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
  if (ready_)
  {
    return ESP_OK;
  }

  if (owns_resources())
  {
    cleanup();
  
    if (owns_resources())
    {
      ESP_LOGE(TAG, "previous display resources could not be released");
      return ESP_ERR_INVALID_STATE;
    }
  }

  if (!board_i2c_master_is_ready()) 
  {
    ESP_LOGW(TAG, "Global i2c master not inited, trying to init");
    if (board_i2c_master_init() != ESP_OK) 
    {
      ESP_LOGE(TAG, "Could not init i2c master");
      return ESP_FAIL;
    }
  }

  // RGB panel first
  if (rgb_init() != ESP_OK) {
    ESP_LOGE(TAG, "RGB display panel failed to init");
    cleanup();
    return ESP_FAIL;
  }

  /* touch_init() performs CH422G initialization and the
   * complete GT911 reset sequence itself, so do not call backlight_init()
   * separately here. */
  if (touch_init() != ESP_OK) 
  {
    ESP_LOGE(TAG, "Touch panel failed to init");
    cleanup();
    return ESP_FAIL;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  // Set backlight
  if (backlight_set(0x1E) != ESP_OK) 
  {
    ESP_LOGE(TAG, "Failed to set backlight");
    cleanup();
    return ESP_FAIL;
  }

  ready_ = true;
  return ESP_OK;
}


void display_task(void* parameter)
{
    IDisplay* Disp = static_cast<IDisplay*>(parameter);
    if (!Disp || !Disp->is_ready())
    {
        ESP_LOGE(TAG, "Display is not inited or not passed as task parameter");
        return;
    }

    display_task_handle = xTaskGetCurrentTaskHandle();

    static StaticTimer_t timer_storage;

    TimerHandle_t refresh_timer = xTimerCreateStatic
    (
        "display_refresh",
        pdMS_TO_TICKS(DISPLAY_REFRESH_MS),
        pdTRUE,
        nullptr,
        display_timer_callback,
        &timer_storage
    );

    if (refresh_timer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create display timer");
        display_task_handle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    if (xTimerStart(refresh_timer, pdMS_TO_TICKS(100)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to start display timer");
        display_task_handle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(TAG, "Display timer start requested");

    while (true)
    {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000)) == 0) 
        {
            ESP_LOGW(TAG, "No display timer notification within 3 seconds");
            continue;
        }

        // Just switch backlight on/off for test
        // TODO: Replace with UI update, or move all this to UI and let that handle it
        // Or have a queue recieve from UI to update
        update_display_backlight(*Disp, display_buffer.refresh_count);
        

        display_buffer.refresh_count++;

        ESP_LOGI(TAG, "Display buffer update frame %lu", static_cast<unsigned long>(display_buffer.refresh_count));
    }
}

Display::~Display() noexcept
{
    cleanup();
}

bool Display::owns_resources() const noexcept
{
    return rgb_handle_ != nullptr || tp_handle_ != nullptr || tp_io_handle_ != nullptr || bl_cfg_dev_ != nullptr || bl_out_dev_ != nullptr || touch_reset_gpio_configured_;
}

void Display::cleanup() noexcept
{
    ready_ = false;

    if (bl_cfg_dev_ != nullptr)
    {
      backlight_set(0x00); 
    }

    const auto release = [](auto& handle, auto delete_resource, const char* name)
    {
      if (handle == nullptr) 
      {
        return;
      }
        
      const esp_err_t result = delete_resource(handle);
      if (result == ESP_OK)
      {
        handle = nullptr;
      }
      else
      {
        ESP_LOGW(TAG, "Failed to release %s: %s", name, esp_err_to_name(result));
      }
    };
  
    release(tp_handle_, esp_lcd_touch_del, "touch panel");

    if (tp_handle_ == nullptr)
    {
      release(tp_io_handle_, esp_lcd_panel_io_del, "touch panel I/O");
    }

    release(rgb_handle_, esp_lcd_panel_del, "RGB panel");

    if (touch_reset_gpio_configured_ && tp_handle_ == nullptr)
    {
      const esp_err_t result = gpio_reset_pin(LCD_TOUCH_RESET_IO);
      if (result == ESP_OK)
      {
        touch_reset_gpio_configured_ = false;
      }
      else
      {
        ESP_LOGW(TAG, "Failed to reset touch reset GPIO: %s", esp_err_to_name(result));
      }
    }

    release(bl_out_dev_, i2c_master_bus_rm_device, "backlight output");
    release(bl_cfg_dev_, i2c_master_bus_rm_device, "backlight config");
        
}
