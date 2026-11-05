#include "bme280_adapter.hpp"

esp_err_t Bme280Adapter::init()
{
    return bme280_.init();
}

esp_err_t Bme280Adapter::read(SensorReadings& readings)
{
    const auto raw = bme280_.get_readings();
    constexpr float failed_reading = -999.0f;

    if (raw.temperature == failed_reading ||
        raw.humidity == failed_reading ||
        raw.pressure == failed_reading)
    {
        return ESP_FAIL;
    }

    readings.temperature = raw.temperature;
    readings.humidity = raw.humidity;
    readings.pressure = raw.pressure;
    return ESP_OK;
}