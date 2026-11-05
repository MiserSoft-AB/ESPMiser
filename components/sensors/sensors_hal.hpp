#pragma once

#include "esp_err.h"

struct SensorReadings
{
  float temperature;
  float humidity;
  float pressure;
};

class ISensor
{
public:
    virtual ~ISensor() = default;

    virtual esp_err_t init() = 0;
    virtual esp_err_t read(SensorReadings& readings) = 0;
};