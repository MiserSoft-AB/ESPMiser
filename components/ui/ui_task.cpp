#include "ui_task.hpp"
#include "ui_queue.hpp"
#include <esp_log.h>

static const char* TAG = "ui_task";

void Ui::ui_task(void* param) {
  Ui_Data Data;
  
  while (true) {
    // Block until data arrives, timeout after 100ms
    if (UiQueue::receive(Data, pdMS_TO_TICKS(100)) == 0) 
    {
      ESP_LOGI(TAG, "Data.leop_data.price: %f", Data.leop_data.price);
    }
    
    // No delay yolo (queue recieve should be enough)
  }
}

