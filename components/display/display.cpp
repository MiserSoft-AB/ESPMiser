#include "display.hpp"
#include "board_config.h"

#include <cstdint>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define BACKLIGHT_MASK ((uint8_t)(1U << 2))

namespace 
{
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

// esp_lcd_touch_gt911 example
/*
{
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = io_config.dev_addr,
    };

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_LCD_HRES,
        .y_max = CONFIG_LCD_VRES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &tp_gt911_config,
    };

    esp_lcd_touch_handle_t tp;
    esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &tp);
}
*/

esp_err_t Display::backlight_init(i2c_bus_handle_t i2c_bus)
{
  // Diagnostic: probe the bus for any responding devices before CH422G writes.
  uint8_t found[16];
  const uint8_t count = i2c_bus_scan(i2c_bus, found, sizeof(found));
  ESP_LOGI(TAG, "i2c scan found %u device(s):", (unsigned)count);
  for (uint8_t i = 0; i < count; ++i)
    ESP_LOGI(TAG, "  addr=0x%02X", found[i]);

  // Init config bus device
  bl_cfg_dev_ = i2c_bus_device_create(board_i2c_bus_get(), I2C_ADDR_CH422G_CFG, I2C_MASTER_FREQ_HZ);
  if (!bl_cfg_dev_)
  {
    ESP_LOGE(TAG, "Failed to create bus device for CH422G backlight config");
    return ESP_FAIL;
  }

  // Init io bus device
  bl_dev_ = i2c_bus_device_create(board_i2c_bus_get(), I2C_ADDR_CH422G, I2C_MASTER_FREQ_HZ);
  if (!bl_dev_)
  {
    ESP_LOGE(TAG, "Failed to create bus device for CH422G backlight driver");
    return ESP_FAIL;
  }

  // Set output mode on config dev
  if (i2c_bus_write_byte(bl_cfg_dev_, NULL_I2C_MEM_ADDR, 0x01) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set CH422G output mode");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t Display::backlight_set(uint8_t value)
{
  if (bl_dev_ == nullptr)
    return ESP_FAIL;


  // const uint8_t duty = (percent > 100) ? 100 : percent;
  
  esp_err_t err = i2c_bus_write_byte(bl_dev_, NULL_I2C_MEM_ADDR, value);

  // Write inital state on io dev
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
     "CH422G IO write: addr=0x%02X, freq=%lu, byte=0x%02X, err=%s (0x%x)",
     (unsigned)I2C_ADDR_CH422G,
     (unsigned long)I2C_MASTER_FREQ_HZ,
     esp_err_to_name(err),
     value,
     (unsigned)err);
  }

  return err; 
}

esp_err_t Display::touch_init(i2c_bus_handle_t i2c_bus)
{
  tp_io_cfg_ = {                                                                            
    .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 
    .control_phase_bytes = 1, 
    .dc_bit_offset = 0, 
    .lcd_cmd_bits = 16,          
    .flags = {                                                                 
      .disable_control_phase = 1,                                              
    },                                                                          
    .scl_speed_hz = 0, // default says 100000, but waveshare says 0    
  };
  if (esp_lcd_new_panel_io_i2c_v2(
        i2c_bus_get_internal_bus_handle(i2c_bus), 
        &tp_io_cfg_, 
        &tp_io_handle_) 
      != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create new i2c handle for touch panel");
    return ESP_FAIL;
  }

  esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
    .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
  };

  const esp_lcd_touch_config_t tp_cfg = {
    .x_max = LCD_H_RES,
    .y_max = LCD_V_RES,
    .rst_gpio_num = (gpio_num_t)LCD_TOUCH_PIN_NUM_RST,
    .int_gpio_num = (gpio_num_t)LCD_TOUCH_PIN_NUM_INT,
    .levels = {
      .reset = 0,
      .interrupt = 0,
    },
    .flags = {
      .swap_xy = 0,
      .mirror_x = 0,
      .mirror_y = 0,
    },
    .driver_data = &tp_gt911_config,
  };

  if (esp_lcd_touch_new_i2c_gt911(tp_io_handle_, &tp_cfg, &tp_handle_) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create GT911 driver");
    return ESP_FAIL;
  }

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
  i2c_bus_handle_t i2c_bus = board_i2c_bus_get();
  if (!i2c_bus)
  {
    ESP_LOGE(TAG, "Global i2c bus not created");
    return ESP_FAIL;
  }

  // Init RGB display
  if (this->rgb_init() != ESP_OK)
  {
    ESP_LOGE(TAG, "RGB display panel failed to init");
    return ESP_FAIL;
  }

  // Init touch
  if (this->touch_init(i2c_bus) != ESP_OK)
  {
    ESP_LOGE(TAG, "Touch panel failed to init");
    return ESP_FAIL;
  }

  // Init backlight
  if (this->backlight_init(i2c_bus) != ESP_OK)
  {
    ESP_LOGE(TAG, "Backlight failed to init");
    return ESP_FAIL;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  if (this->backlight_set(0x1E) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set backlight");
    return ESP_FAIL;
  }

  return ESP_OK;
}


void display_task(void* parameter)
{
    (void)parameter;

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

        display_buffer.refresh_count++;

        ESP_LOGI(TAG, "Display buffer update frame %lu", static_cast<unsigned long>(display_buffer.refresh_count));
    }
}
