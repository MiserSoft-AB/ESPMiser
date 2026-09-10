#include "esp_err.h"
#include "freertos/projdefs.h"
#include "http_client.hpp"
#include "portmacro.h"
#include "tasks/tasks.hpp"
#include "wifi.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <nvs_flash.h>
#include <esp_event.h>


static const char* TAG = "esp_miser";

WifiHandler g_wifihandler(WIFI_INIT_CONFIG_DEFAULT());  

extern "C" void app_main(void)
{
  esp_err_t res;

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta(); // This line is the result of many headbashes

  res = g_wifihandler.wifi_init();
  ESP_ERROR_CHECK(res);
  if (res == ESP_FAIL)
  {
    ESP_LOGE(TAG, "Failed to init wifi, res: %s", esp_err_to_name(res)); 
  }

  g_wifihandler.wifi_connect();

  // Blocks until we have an IP or die
  // res = g_wifihandler.wifi_wait_for_connect();
  // ESP_ERROR_CHECK(res);
  // if (res != ESP_OK)
  // {
  //   ESP_LOGE(TAG, "Failed to connect wifi try again, res: %s", esp_err_to_name(res));
  // }

  ESP_LOGI(TAG, "IP: %s", g_wifihandler.get_ip().c_str());

  BaseType_t task_result = xTaskCreate(
    network_task,     //the function freertos should execute as a task
    "network_task",       // name for debugging
    8192,           // stack size for the task in ESP-IDF
    nullptr,        // parameter to pass to the task, for example could be &config
    5,                // task priority
  nullptr          // optional task handle
  );

  if (task_result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create network task");
  }

  BaseType_t sensor_task_result = xTaskCreate(
    sensor_task,
    "sensor_task",
    4096,
    nullptr,
    5,
    nullptr
);

if (sensor_task_result != pdPASS)
{
    ESP_LOGE(TAG, "Failed to create sensor task");
}


  ESP_LOGI(TAG, "hello");
}
