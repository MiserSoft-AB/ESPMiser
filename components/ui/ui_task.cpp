#include "ui_task.hpp"
#include "data.h"
#include "ui_queue.hpp"
#include <esp_log.h>

static const char* TAG = "ui_task";

void Ui::ui_task(void* param) {
  // Local copy of ui data
  UiQueue::Data UiD;
  data_leop_init(&UiD.leop);
  data_sensor_init(&UiD.sensor);
  data_wifi_init(&UiD.wifi);

  while (true) {
    // Block until data arrives, timeout after 100ms
    if (UiQueue::receive(UiD, pdMS_TO_TICKS(100)) == 0) 
    {
      switch (UiD.type) 
      {
        case DATA_NONE:
          ESP_LOGI(TAG, "Data recieved with type none...");
          break;
        case DATA_LEOP:
          ESP_LOGI(TAG, "Leop Data recieved: %f", UiD.leop.price);
          break;
        case DATA_SENSOR:
          ESP_LOGI(TAG, "Sensors Data recieved: temp: %f, humidity: %f, press: %f.", 
            UiD.sensor.temperature,
            UiD.sensor.pressure,
            UiD.sensor.humidity
          );
          break;
        case DATA_ALL:
          ESP_LOGI(TAG, "All data recieved! price: %ftemp: %f, humidity: %f, press: %f.", 
            UiD.leop.price,
            UiD.sensor.temperature,
            UiD.sensor.pressure,
            UiD.sensor.humidity
          );
        default:
          ESP_LOGW(TAG, "Unknown data recieved");
          break;
      }
    }
    
    // NOTE: Only the last recieved data is recent for certain here on out 
    // Should maybe handle type-specific stuff inside the switch

    
    // No delay yolo (queue recieve should be enough)
  }
}

