#pragma once

#include "esp_err.h"

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    virtual esp_err_t init() = 0;
    virtual bool is_ready() = 0;
    virtual esp_err_t set_backlight(bool enabled) = 0;
};