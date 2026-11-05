#ifndef __LEOP_DATA_HPP__
#define __LEOP_DATA_HPP__

#include <ctime>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef enum
{
  UI_ALL_DATA,
  UI_LEOP_DATA,
  UI_SENSOR_DATA,
  UI_NO_DATA,

} UiDataType; // For consumer to know what is being recieved

typedef struct 
{
  time_t timestamp;

  float temperature;
  float precipitation;
  float windspeed;

  int radiation_shortwave;

} Weather_Sample; // Weather state at specified timestamp

typedef struct {
  // POSSIBLE CHEAPNESS VALUES:
  //   1 = BUY
  //   0 = STORE
  //  -1 = SELL
  
  time_t timestamp;

  int price[4];
  int cheapness_quarter[4];
  int solar_quarter[4];
  // int consumption_quarter[4]; // Does not exist in the api response without a separate endpoint call and connected P1 meter?

} Hourly_Energy_Plan; // Energy plan is recieved in increments of 15 min from the LEOP server. Value of 4 = store 1hr forecast.

typedef struct 
{
  Weather_Sample      weather_forecast[24];
  Hourly_Energy_Plan  energy_plan;

} Leop_Data; // Data from LEOP server

typedef struct 
{
  float temperature;
  float humidity;
  float pressure;

} Sensor_Data; // Replace with actual struct from shared lib

typedef struct 
{
  Leop_Data   leop_data;
  Sensor_Data sensor_data;

  UiDataType  type;

} Ui_Data;

static void ui_data_init(Ui_Data* UiD)
{
  if (UiD == nullptr) {return;}

  *UiD = Ui_Data{};
  UiD->type = UI_NO_DATA;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LEOP_DATA_HPP__
