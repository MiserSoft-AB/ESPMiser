#ifndef __DISPLAY_HPP__
#define __DISPLAY_HPP__

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_lcd_touch.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_io.h>
// #include <lvgl.h>

class Display
{
  public:

    Display() {}; 
    ~Display() {};

    // No copy, no move
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&) noexcept = delete;
    Display& operator=(Display&&) noexcept = delete;

    // Must be run before task is started
    esp_err_t init();

    // Set backlight level (CH422G PWM duty cycle)
    // 0: off, 100: full brightness
    esp_err_t backlight_set(uint8_t percent);

    // Is display inited?
    bool is_ready() { return ready_; }

  private:
    esp_lcd_panel_handle_t  rgb_handle_ = nullptr;

    // CH422G LED driver
    i2c_master_dev_handle_t bl_cfg_dev_ = nullptr;
    i2c_master_dev_handle_t bl_out_dev_ = nullptr;

    // esp_lcd_panel_io_handle_t     bl_io_handle_ = nullptr;
    // We need a seperate handle for ch422g config on addr 0x24

    // i2c_bus_device_handle_t       bl_cfg_dev_   = nullptr;
    // Main ch422g backlight handle
    // i2c_bus_device_handle_t       bl_dev_       = nullptr;

    // GT911 touch panel 
    esp_lcd_touch_handle_t        tp_handle_    = nullptr;
    esp_lcd_panel_io_handle_t     tp_io_handle_ = nullptr;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg_{};

    esp_err_t rgb_init();
    esp_err_t touch_init();
    esp_err_t touch_reset();
    esp_err_t backlight_init();
    esp_err_t ch422g_write_output(uint8_t value);

    bool ready_ = false;
};

void display_task(void* parameter); //explain


#endif // __DISPLAY_HPP__
