#include "ui_task.hpp"
#include "data.h"
#include "data_queue.hpp"
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
    Qd = DataQueue::get(); 
    
    ESP_LOGI(TAG, "UI data recieved! price: %f temp: %f, humidity: %f, press: %f.", 
      Qd.leop.price,
      Qd.sensor.temperature,
      Qd.sensor.pressure,
      Qd.sensor.humidity
    );
    // NOTE: Only the last recieved data is recent for certain here on out 
    // Should maybe handle type-specific stuff inside the switch

    
    // No delay yolo (queue recieve should be enough)
    vTaskDelay(100);
  }
}

