#include "display_adapter.hpp"

#include <cstdint>

esp_err_t DisplayAdapter::init()
{
    return display_.init();
}

bool DisplayAdapter::is_ready()
{
    return display_.is_ready();
}

esp_err_t DisplayAdapter::set_backlight(bool enabled)
{
    constexpr std::uint8_t backlight_on = 0x04;
    constexpr std::uint8_t backlight_off = 0x00;

    return display_.backlight_set(enabled ? backlight_on : backlight_off);
}