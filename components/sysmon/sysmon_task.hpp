#include <esp_log.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

#include "driver/temperature_sensor.h"


class MiserSysMon 
{
public:

  // This runs constructor and allocates it on heap
  static esp_err_t start(); 

  // Return internal temperature
  float get_temp();

private:

  temperature_sensor_handle_t temp_handle_ = NULL;

  const size_t heap_warning_threshold = 10240; // 10KB max heap
  const size_t stack_warning_threshold = 512; // when a task has this little left

  void run();
};
