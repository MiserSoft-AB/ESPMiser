#include "bme280.hpp"
#include "board_config.h"

#include <esp_log.h>

static const char* TAG = "bme280";

/*
Pin assignment:

    master:
        GPIO2 is assigned as the clock signal of i2c master port
        GPIO1 is assigned as the data signal of i2c master port
    Connection:
        connect sda of sensor with GPIO1
        connect scl of sensor with GPIO2
*/

esp_err_t Bme280::init()
{
  i2c_bus_handle_t i2c_bus = board_i2c_bus_get();
  if (!i2c_bus)
  {
    ESP_LOGE(TAG, "Global i2c bus not created");
    return ESP_FAIL;
  }
  
  bme280_ = bme280_create(i2c_bus, I2C_ADDR_BME280);
  if (!bme280_)
  {
    ESP_LOGE(TAG, "Failed to create bme280 handle");
    return ESP_FAIL;
  }
  return bme280_default_init(bme280_);
}

Bme280::Readings Bme280::get_readings()
{
  float temp, humi, pres;

  if (bme280_read_temperature(bme280_, &temp) == ESP_OK)
    readings_.temperature = temp;
  else
    readings_.temperature = -999.0f;

  if (bme280_read_humidity(bme280_, &humi) == ESP_OK)
    readings_.humidity = humi;
  else
    readings_.humidity = -999.0f;

  if (bme280_read_pressure(bme280_, &pres) == ESP_OK)
    readings_.pressure = pres;
  else
    readings_.pressure = -999.0f;

  return readings_;
}
