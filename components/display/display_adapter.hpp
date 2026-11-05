#pragma once

#include "display_hal.hpp"
#include "display.hpp"

class DisplayAdapter final : public IDisplay
{
public:
    esp_err_t init() override;
    bool is_ready() override;
    esp_err_t set_backlight(bool enabled) override;
    lv_display_t* lv_disp_get();

private:
    Display display_;
};