#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_event.h>

#include <string>

#include "../components/wifi/wifi.hpp"

static const char* TAG = "esp_miser";

WifiHandler g_wifihandler(WIFI_INIT_CONFIG_DEFAULT());  

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(g_wifihandler.wifi_init());
  ESP_ERROR_CHECK(g_wifihandler.wifi_connect());

  ESP_LOGI(TAG, "IP: %s", g_wifihandler.get_ip().c_str());

  ESP_LOGI(TAG, "hello");
}
