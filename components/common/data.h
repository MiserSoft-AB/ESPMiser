#ifndef __DATA_H__
#define __DATA_H__

/* Common data structure lib for ESPMiser 
 * Meant for use in shared consumer-producer queues between components */

#include <ctime>
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Task Diagnostics
// Since this is very expensive resource-wise,
// we want it to be optional
// TODO: Make into config (runtime perhaps? needs handling in sysmon task)
#define TASK_DIAG_ENABLED 1

#ifdef TASK_DIAG_ENABLED

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>

#define SYSTEM_DATA_MAX_TASKS 16

#endif // TASK_DIAG_ENABLED

// For producer to set and consumer to know what is being recieved
typedef enum
{
  DATA_ALL,
  DATA_LEOP,
  DATA_SENSOR,
  DATA_SYSTEM,
  DATA_WIFI,
  DATA_NONE,

} DataStructType; 

/*--------------------------- Data structs --------------------------- */

typedef struct 
{
  char* ap_ssid;
  bool  ap_connected;

} Wifi_Data;

#ifdef TASK_DIAG_ENABLED
typedef struct 
{
  const char* name;
  uint32_t    stack_words_remaining;
  eTaskState  state;
  uint8_t     cpu_usage_pct;

} Task_Data;
#endif // TASK_DIAG_ENABLED

/* 32bit bit-field struct for storing system status */
typedef struct
{
  uint32_t error_flag   : 1;
  uint32_t wdt_enabled  : 1; 
  uint32_t wifi_enabled : 1; 
  uint32_t leop_conn    : 1; 
  uint32_t sensor_conn  : 1; 

  // Adjust if more flags added
  uint32_t padding      : 27;

} System_Status;

typedef struct 
{
#ifdef TASK_DIAG_ENABLED
  Task_Data tasks[SYSTEM_DATA_MAX_TASKS];
  size_t    tasks_n;
#endif // TASK_DIAG_ENABLED

  float         temp_cpu;
  System_Status sys_status;

} System_Data;

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

} Sensor_Data;

/*--------------------------- Init helpers --------------------------- */

static void data_init_system(System_Data* SyD)
{
  if (!SyD)
    return;

  memset(SyD, 0, sizeof(System_Data));
}

static void data_wifi_init(Wifi_Data* WiD)
{
  if (!WiD)
    return;

  *WiD = { 
    .ap_ssid = NULL,
    .ap_connected = false,
  };
}

static void data_leop_init(Leop_Data* LeD)
{
  if (!LeD)
    return;

  *LeD = { 

  };
}

static void data_sensor_init(Sensor_Data* SeD)
{
  if (!SeD)
    return;

  *SeD = { 
    .temperature = 0.0,
    .humidity    = 0.0,
    .pressure    = 0.0,
  };
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __LEOP_DATA_HPP__
