#include "ui_task.hpp"
#include "data.h"
#include "ui_queue.hpp"
#include <esp_log.h>

static const char* TAG = "ui_task";

void Ui::ui_task(void* param) {
  // Local copy of ui data
  Ui_Data UiD;
  ui_data_init(&UiD);
  

  while (true) {
    // Block until data arrives, timeout after 100ms
    if (UiQueue::receive(UiD, pdMS_TO_TICKS(100)) == 0) 
    {
      switch (UiD.type) 
      {
        case UI_NO_DATA:
          ESP_LOGW(TAG, "Data recieved but no type specified");
          break;
        case UI_LEOP_DATA:
          ESP_LOGI(TAG, "Leop Data recieved: time: %lld, price: %d, cheapness: %d, solar_dev: %d", 
            UiD.leop_data.energy_plan.timestamp, 
            UiD.leop_data.energy_plan.price[0],
            UiD.leop_data.energy_plan.cheapness_quarter[0],
            UiD.leop_data.energy_plan.solar_quarter[0]
          );
          break;
        case UI_SENSOR_DATA:
          ESP_LOGD(TAG, "Sensors Data recieved: temp: %f, humidity: %f, press: %f.", 
            UiD.sensor_data.temperature,
            UiD.sensor_data.pressure,
            UiD.sensor_data.humidity
          );
          break;
        case UI_ALL_DATA:
          ESP_LOGD(TAG, "All data recieved! price: %ftemp: %f, humidity: %f, press: %f.", 
            UiD.leop_data.energy_plan.price[0],
            UiD.sensor_data.temperature,
            UiD.sensor_data.pressure,
            UiD.sensor_data.humidity
          );
      }
    }
    
    // NOTE: Only the last recieved data is recent for certain here on out 
    // Should maybe handle type-specific stuff inside the switch

    
    // No delay yolo (queue recieve should be enough)
  }
}

