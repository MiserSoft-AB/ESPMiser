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

Bme280::~Bme280() noexcept
{
  cleanup();
}

void Bme280::cleanup() noexcept
{
  if (bme280_ != nullptr)
  {
    const esp_err_t result = bme280_delete(&bme280_);
    if (result != ESP_OK)
    {
      ESP_LOGW(TAG, "BME280 cleanup failed: %s", esp_err_to_name(result));
    }
  }
}

esp_err_t Bme280::init()
{
  if (bme280_ != nullptr)
  {
    return ESP_OK;
  }

  i2c_master_bus_handle_t i2c_master = board_i2c_master_bus_get();
  if (!board_i2c_master_is_ready() || !i2c_master)
  {
    ESP_LOGE(TAG, "I2C master not initialized");
    return ESP_FAIL;
  }

  bme280_ = bme280_create(i2c_master, I2C_ADDR_BME280);
  if (!bme280_)
  {
    ESP_LOGE(TAG, "Failed to create BME280 handle");
    return ESP_FAIL;
  }

  const esp_err_t result = bme280_default_init(bme280_);
  if (result != ESP_OK)
  {
    cleanup(); 
  }

  return result;
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
