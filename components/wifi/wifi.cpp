#include "wifi.hpp"

static const char* TAG = "WIFIHANDLER";

// PRIVATE //

void WifiHandler::on_wifi_init_finished(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;

    ESP_LOGI(TAG, "WIFI STARTED SUCCESSFULLY!!"); //DBG
    
    if(xSemaphoreTake(wifihandler->mutex_, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        wifihandler->wifi_ready = true;
        xSemaphoreGive(wifihandler->mutex_);
    }
    else
    {
        ESP_LOGE(TAG, "WIFI MUTEX LOCK FAILURE :(");
        return;
    }

    wifihandler->wifi_connect();

    return;
}

void WifiHandler::on_wifi_connected(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;
    
    ESP_LOGI(TAG, "WIFI CONNECTED SUCCESSFULLY!!"); //DBG

    return;
}

void WifiHandler::on_assigned_ip(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = static_cast<WifiHandler*>(handler_args);
    ip_event_got_ip_t* data = static_cast<ip_event_got_ip_t*>(event_data);
    char ip_buf[16] = {};
    
    std::string new_ip;

    if (esp_ip4addr_ntoa(&data->ip_info.ip, ip_buf, sizeof(ip_buf)) != nullptr)
    {
            new_ip = ip_buf;
    }
    else
    {
        new_ip = "";
    }

    if (xSemaphoreTake(wifihandler->mutex_, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGE(TAG, "WIFI MUTEX LOCK FAILURE :(");
        return;
    }

    wifihandler->IP = new_ip;

    xSemaphoreGive(wifihandler->mutex_);

    ESP_LOGI(TAG, "IP: %s\n", new_ip.c_str());
}

////////////

esp_err_t WifiHandler::wifi_init() {
    if (mutex_ == nullptr)
    {
        mutex_ = xSemaphoreCreateMutex(); // Create mutex_

        if (mutex_ == nullptr)
        {
            ESP_LOGE(TAG, "WIFI MUTEX FAILURE");
            return ESP_ERR_NO_MEM;
        }
    }

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &on_wifi_init_finished, this));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connected, this));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_assigned_ip, this));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_CREDENTIAL_SSID,
            .password = CONFIG_WIFI_CREDENTIAL_PASSPHRASE,
        },
    };

    esp_err_t result;
    
    result = esp_wifi_init(&init_config);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "INIT FAILURE");
        return result;
    }

    result = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "MODE SET FAILURE");
        return result;
    }

    result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "CONFIG SET FAILURE");
        return result;
    }

    result = esp_wifi_start();
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "WIFI START FAILURE");
        return result;
    }

    return result;
}

esp_err_t WifiHandler::wifi_connect() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGE(TAG, "WIFI MUTEX LOCK FAILURE :(");
        return ESP_ERR_TIMEOUT;
    }
    
    if (!wifi_ready)
    {
        xSemaphoreGive(mutex_);
        return ESP_ERR_WIFI_NOT_STARTED;
    }
    
    esp_err_t result = esp_wifi_connect();
    
    xSemaphoreGive(mutex_);
        
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "WIFI CONNECTION FAILURE");
        return result;
    }

    return ESP_OK;
}

std::string WifiHandler::get_ip() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        ESP_LOGE(TAG, "WIFI MUTEX LOCK FAILURE :(");
        return "";
    }

    std::string current_ip = IP;

    xSemaphoreGive(mutex_);

    return current_ip;
}


// Unnecessary for basic wifi connectivity test
// esp_err_t esp_wifi_deinit();
// esp_err_t esp_wifi_stop();
// esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);

// esp_err_t disconn_result = esp_wifi_disconnect();
// esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);