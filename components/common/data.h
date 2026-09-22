#ifndef __LEOP_DATA_HPP__
#define __LEOP_DATA_HPP__

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
  float    price;

} Leop_Data; // Replace with actual struct from shared lib

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
  *UiD = {
    .leop_data = {
      .price = 0,
    },
    .sensor_data = {
      .temperature = 0.0,
      .humidity    = 0.0,
      .pressure    = 0.0,
    },
    .type = UI_NO_DATA,
  };
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LEOP_DATA_HPP__
