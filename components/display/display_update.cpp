#include "display_update.hpp"

esp_err_t initialize_display(IDisplay& display)
{
    if (display.is_ready())
    {
        return ESP_OK;
    }

    return display.init();
}