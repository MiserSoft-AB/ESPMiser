#ifndef __BME280_HPP__
#define __BME280_HPP__

#include "board_config.h"
#include "bme280.h" // dep: espressif/bme280


class Bme280
{
public:
  struct Readings
  {
    float temperature;
    float humidity;
    float pressure;
  };

  Bme280() {}; 
  ~Bme280() {};

  // No copy, no move
  Bme280(const Bme280&) = delete;
  Bme280& operator=(const Bme280&) = delete;
  Bme280(Bme280&&) noexcept = delete;
  Bme280& operator=(Bme280&&) noexcept = delete;

  // Inits the i2c bus for bme280 sensors
  esp_err_t init();

  // Read from sensors and return the results
  Readings get_readings();

private:

  i2c_bus_handle_t i2c_bus_ = NULL;
  bme280_handle_t  bme280_  = NULL;

  Readings readings_ = { 0.0, 0.0, 0.0 };

};

#endif // __BME280_HPP__
