#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

namespace BoardI2c
{
    esp_err_t init();
    i2c_master_bus_handle_t handle();
}