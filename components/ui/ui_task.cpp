#include "ui_task.hpp"
#include "data.h"
#include "data_queue.hpp"
#include "freertos/projdefs.h"
#include <esp_log.h>

static const char* TAG = "ui_task";

void Ui::ui_task(void* param) {
  // Local copy of queue data
  DataQueue::Data Qd;
  data_leop_init(&Qd.leop);
  data_sensor_init(&Qd.sensor);
  data_wifi_init(&Qd.wifi);

  while (true) {
    // Refresh local data copy from data queue
    if (DataQueue::get(&Qd, pdMS_TO_TICKS(100)) == 0) 
      // Might just want to use software timer and pass zero timeout to queue for a try-lock
    {
      ESP_LOGI(TAG, "UI data updated! price: %f temp: %f, humidity: %f, press: %f.", 
        Qd.leop.price,
        Qd.sensor.temperature,
        Qd.sensor.pressure,
        Qd.sensor.humidity
      );
    }
    
    // vTaskDelay(100);
  }
}

