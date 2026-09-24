#include "http_client.hpp"
#include "wifi.hpp"
#include "task_priorities.hpp"
#include "sysmon_task.hpp"
#include "display.hpp"
#include "ui_queue.hpp"
#include "ui_task.hpp"
#include "leop_fetcher.hpp"
#include "sensors_task.hpp"

#include <esp_psram.h>
#include <portmacro.h>
#include <esp_err.h>
#include <freertos/projdefs.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <nvs_flash.h>
#include <esp_event.h>

#include <esp_partition.h>

static const char* TAG = "esp_miser";

static constexpr size_t LEOP_RESPONSE_BUF_SIZE = 1024 * 64; // 64kb of juicy PSRAM

static WifiHandler g_wifihandler(WIFI_INIT_CONFIG_DEFAULT());  

extern "C" void app_main(void)
{
  esp_err_t res;
  BaseType_t task_result;

  // Initialize PSRAM
  ESP_ERROR_CHECK(esp_psram_init());
  if (esp_psram_is_initialized()) 
  {
    ESP_LOGI(TAG, "PSRAM found! Size: %u bytes\n", esp_psram_get_size()); // Use psram_get_size() for total
  } else {
    ESP_LOGI(TAG, "PSRAM not found.\n");
  }

  // Start watchdog task
  res = MiserSysMon::start();
  if (res != ESP_OK)
    ESP_LOGE(TAG, "Failed to create system watchdog task: %s", esp_err_to_name(res));

  // Start nvs and wifi
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  ESP_ERROR_CHECK(g_wifihandler.wifi_init());

  // Start display (maybe make part of ui init or keep separate?)
  task_result = xTaskCreate( 
    display_task,
    "display_task",
    4096,
    nullptr,
    task_priorities::DISPLAY,
    nullptr
  );
  if (task_result != pdPASS)
    ESP_LOGE(TAG, "Failed to create display task");

  // Start UI
  UiQueue::init(); // Create queue
  task_result = xTaskCreate(
    Ui::ui_task,
    "ui_task",
    4096,
    nullptr,
    task_priorities::DISPLAY, //UI prio?
    nullptr
  );
  if (task_result != pdPASS)
    ESP_LOGE(TAG, "Failed to create ui task");

  // Start leop fetcher task
  LeopFetcher Lf(LEOP_RESPONSE_BUF_SIZE);
  task_result = xTaskCreate(
    LeopFetcher::leop_fetch_task,
    "leop_fetcher",
    4096,
    &Lf,
    task_priorities::NETWORK,
    nullptr
  );
  if (task_result != pdPASS) //pdPASS = task created successfully
    ESP_LOGE(TAG, "Failed to create leoppp task");

  // Start sensors task
  task_result = xTaskCreate(
    Sensors::sensors_task_test,
    "sensors_task",
    4096,
    nullptr,
    task_priorities::SENSOR,
    nullptr
  );
  if (task_result != pdPASS) //pdPASS = task created successfully
    ESP_LOGE(TAG, "Failed to create sensors task");

}
