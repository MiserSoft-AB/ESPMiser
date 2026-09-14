#include "esp_err.h"
#include "esp_wifi_default.h"
#include "freertos/projdefs.h"
#include "http_client.hpp"
#include "wifi.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <nvs_flash.h>
#include <esp_event.h>


static const char* TAG = "esp_miser";

static WifiHandler g_wifihandler(WIFI_INIT_CONFIG_DEFAULT());  

void test_http_task_oneshot(void* param) 
{
  // Create client config
  EspHttpClient::Config Cfg {};
  Cfg.timeout_ms = 3000;
  Cfg.response_body_max_len = 4096;

  // Create client
  EspHttpClient client(Cfg);
  while (true)
  {
    // --- Request 1: GET ---
    esp_err_t err = client.get("http://httpforever.com");
    if (err == ESP_OK) 
    {
      int status = client.check_status_code();
      std::string body;
      client.read_body(body);
      ESP_LOGI("http_task", "GET Status: %d, Body: %s", status, body.c_str());
    }

    // --- Request 2: POST (Reusing same client) ---
    // Note: With the reset_request_state fix, headers from GET won't leak here.
    // std::string payload = "{\"key\": \"value\"}";
    // err = client.post("http://api.example.com/update", payload);
    // if (err == ESP_OK) {
    //   int status = client.check_status_code();
    //   ESP_LOGI("TASK", "POST Status: %d", status);
    // }

    // Destructor runs client_clean() automatically
    // vTaskDelete(NULL);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

extern "C" void app_main(void)
{
  esp_err_t res;

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  ESP_ERROR_CHECK(g_wifihandler.wifi_init());
  //ESP_ERROR_CHECK(g_wifihandler.wifi_connect());

  //test_http_task_oneshot(NULL);

  ESP_LOGI(TAG, "hello");

}
