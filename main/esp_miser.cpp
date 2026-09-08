#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "esp_miser";

extern "C" void app_main(void) {
  struct DisplayBuffer 
  {
    uint32_t refresh_count;
  };

  static DisplayBuffer display_buffer{};

  void display_task(void* parameter)
  {
    while (true)
    {
      display_buffer.refresh_count++;

      ESP_LOGI(TAG, "Display buffer updated: %lu",
              static_cast<unsigned long>(display_buffer.refresh_count));
      
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}
