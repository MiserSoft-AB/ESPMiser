#pragma once

#include "display_hal.hpp"

#include <cstdint>

esp_err_t update_display_backlight(IDisplay& display, std::uint32_t frame);