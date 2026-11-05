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
  // Init i2c bus
  // Make more modular, pass conf as arg?
  i2c_config_t i2c_conf = {
      .mode          = I2C_MODE_MASTER,
      .sda_io_num    = I2C_MASTER_SDA_PIN,
      .scl_io_num    = I2C_MASTER_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master        = { 
        .clk_speed = I2C_MASTER_FREQ_HZ, 
      },
  };
  i2c_bus_ = i2c_bus_create(I2C_MASTER_NUM, &i2c_conf);
  if (!i2c_bus_)
  {
    ESP_LOGE(TAG, "Failed to create i2c bus");
    return ESP_FAIL;
  }

  // Init bme280
  bme280_ = bme280_create(i2c_bus_, I2C_ADDR_BME280);
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
