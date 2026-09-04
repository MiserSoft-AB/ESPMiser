#include "wifi.hpp"
#include "esp_wifi.h"

static const char* TAG = "WIFIHANDLER";

esp_err_t WifiHandler::wifi_init() {
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSPHRASE,
        },
    };

    esp_err_t result;
    
    result = esp_wifi_init(&init_config);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "INIT FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "INIT SUCCESS"); //DBG

    result = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "MODE SET FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "MODE SET SUCCESS"); //DBG

    result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "CONFIG SET FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "CONFIG SET SUCCESS"); //DBG

    result = esp_wifi_start();
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "WIFI START FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "WIFI START SUCCESS"); //DBG

    return result;
}

esp_err_t WifiHandler::wifi_connect() {
    esp_err_t result = esp_wifi_connect();
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "WIFI CONNECTION FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "WIFI CONNECTION SUCCESS"); //DBG

    return result;
}

std::string WifiHandler::get_ip() {

    return "Not implemented";
}


// Unnecessary for basic wifi connectivity test
// esp_err_t esp_wifi_deinit();
// esp_err_t esp_wifi_stop();
// esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);

// esp_err_t disconn_result = esp_wifi_disconnect();
// esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);