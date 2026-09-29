#include "leop_fetcher.hpp"
#include "data.h"
#include "ui_queue.hpp"


#include <time.h> // for rand might remove

static const char* TAG = "leop_fetcher";

// void LeopFetcher::start(QueueHandle_t Queue, size_t stacksize) {
//   xTaskCreate(leop_fetch_task, "NetworkTask", 4096, (void*)Queue, 5, nullptr);
// }

void LeopFetcher::leop_fetch_task_test(void* params) 
{
  // Local copy of ui data
  Ui_Data UiD;
  ui_data_init(&UiD);

  while (1) 
  {
    // Mock sending data over queue to ui
    srand(time(NULL));
    UiD.type = UI_LEOP_DATA;
    UiD.leop_data.price = (float)rand() / RAND_MAX;
    ESP_LOGI(TAG, "Sending %f to queue.", UiD.leop_data.price);

    UiQueue::send(UiD);

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
