#include "sensors_task.hpp"
#include "data.h"
#include "ui_queue.hpp"

#include <esp_log.h>
#include <time.h>

static const char* TAG = "sensors";

void Sensors::sensors_task_test(void* params) 
{
  // Local copy of ui data
  Ui_Data UiD;
  ui_data_init(&UiD);

  while (1) 
  {
    // Mock sending sensor read data over queue to ui
    srand(time(NULL));

    UiD.type = UI_SENSOR_DATA;
    UiD.sensor_data = {
      .temperature  = (float)rand() / RAND_MAX,
      .humidity     = (float)rand() / RAND_MAX,
      .pressure     = (float)rand() / RAND_MAX,
    };
    ESP_LOGI(TAG, "Sending temp: %f, humidity: %f, press: %f to queue.", 
      UiD.sensor_data.temperature,
      UiD.sensor_data.pressure,
      UiD.sensor_data.humidity
    );

    UiQueue::send(UiD);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
