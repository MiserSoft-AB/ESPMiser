#pragma once

#include "sensors_hal.hpp"
#include "bme280.hpp"

class Bme280Adapter final : public ISensor
{
public:
    esp_err_t init() override;
    esp_err_t read(SensorReadings& readings) override;

private:
    Bme280 bme280_;
};