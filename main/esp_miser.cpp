#include "esp_err.h"
#include "esp_wifi_default.h"
#include "freertos/projdefs.h"
#include "http_client.hpp"
#include "nvs.h"
#include "portmacro.h"
#include "wifi.hpp"
#include "sysmon_task.hpp"
#include "display.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <nvs_flash.h>
#include <esp_event.h>


static const char* TAG = "esp_miser";

static WifiHandler g_wifihandler(WIFI_INIT_CONFIG_DEFAULT());  


extern "C" void app_main(void)
{
  esp_err_t res;

  // Start watchdog task
  res = MiserSysMon::start();
  if (res != ESP_OK)
    ESP_LOGE(TAG, "Failed to create system watchdof task: %s", esp_err_to_name(res));

  esp_err_t flash_res = nvs_flash_init();
  ESP_ERROR_CHECK(flash_res);
  if (flash_res == ESP_ERR_NVS_NO_FREE_PAGES || ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }


  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  ESP_ERROR_CHECK(g_wifihandler.wifi_init());

  BaseType_t task_result = xTaskCreate(
    EspHttpClient::test_fetch_task,   //the function freertos should execute as a task
    "network_task", // name for debugging
    8192,           // stack size for the task in ESP-IDF
    nullptr,        // parameter to pass to the task, for example could be &config
    5,              // task priority
    nullptr         // optional task handle
  );

  if (task_result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create network task");
  }

  BaseType_t display_task_result = xTaskCreate( 
    display_task,               
    "display_task",             
    4096,                       
    nullptr,                    
    3,                         
    nullptr                  
  );

  if (display_task_result != pdPASS) //pdPASS = task created successfully
  {
    ESP_LOGE(TAG, "Failed to create display task");
  }

  ESP_LOGI(TAG, "hello");

}
