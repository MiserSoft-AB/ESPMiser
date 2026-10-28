#include "board_i2c.hpp"
#include "board_config.h"

namespace
{
    i2c_master_bus_handle_t bus_handle = nullptr;
}

esp_err_t BoardI2c::init()
{
    if (bus_handle != nullptr)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t config{};
    
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = static_cast<gpio_num_t>(BOARD_I2C_SDA_PIN);
    config.scl_io_num = static_cast<gpio_num_t>(BOARD_I2C_SCL_PIN);
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;

    return i2c_new_master_bus(&config, &bus_handle);
}

i2c_master_bus_handle_t BoardI2c::handle()
{
    return bus_handle;
}