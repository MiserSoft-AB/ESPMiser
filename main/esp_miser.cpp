#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "esp_miser";

extern "C" void app_main(void) {

  ESP_LOGI(TAG, "hello\n");
}
