#pragma once

#include "sensors_hal.hpp"

struct SensorsUpdate
{
    SensorReadings readings{};
    esp_err_t read_result = ESP_FAIL;

    bool should_publish(bool timer_signaled) const;
};

SensorsUpdate read_sensor_update(ISensor& sensor);