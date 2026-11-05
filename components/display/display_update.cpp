#include "display_update.hpp"

esp_err_t update_display_backlight(IDisplay& display, std::uint32_t frame)
{
    const bool enabled = frame % 2 != 0;
    return display.set_backlight(enabled);
}