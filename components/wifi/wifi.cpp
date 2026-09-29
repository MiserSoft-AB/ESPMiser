#include "wifi.hpp"

static const char* TAG = "WIFIHANDLER";

// PRIVATE //

void WifiHandler::on_wifi_init_finished(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;

    ESP_LOGI(TAG, "WIFI STARTED SUCCESSFULLY!!"); //DBG
    wifihandler->wifi_ready = true;
    
    wifihandler->wifi_connect(); // Can be moved if necessary

    return;
}

void WifiHandler::on_wifi_connected(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;

    wifihandler->wifi_ready = false;
    wifihandler->connecting = false;
    
    ESP_LOGI(TAG, "WIFI CONNECTED SUCCESSFULLY, WAITING FOR IP!!"); //DBG

    return;
}

void WifiHandler::on_wifi_disconnected(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;
    wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*)event_data;
    wifihandler->wifi_ready = true;

    ESP_LOGI(TAG, "WIFI DISCONNECTED!!"); //DBG

// NOTE: Logic for manual disconnection attempts should be added if we add connection 
// functionality via the touchscreen.

    if (disconnected->reason != WIFI_REASON_ASSOC_LEAVE) { 
        wifihandler->wifi_connect();
    }

    return;
}

void WifiHandler::on_assigned_ip(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiHandler* wifihandler = (WifiHandler*)handler_args;
    
    ip_event_got_ip_t* data = (ip_event_got_ip_t*)event_data;

    char ip_buf[16];
    memset(ip_buf, 0, sizeof(ip_buf));
    if (esp_ip4addr_ntoa(&data->ip_info.ip, ip_buf, sizeof(ip_buf)) != nullptr) {
        wifihandler->IP = std::string(ip_buf);
    } else {
        wifihandler->IP = "";
    }

    ESP_LOGI(TAG, "Got IP: %s!", wifihandler->get_ip().c_str());

    return;
}

////////////

esp_err_t WifiHandler::wifi_init() {
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &on_wifi_init_finished, this));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connected, this));
    
    // Called automatically in the event of failed connection attempt.
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnected, this));
    
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
    result = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(result);
    result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_ERROR_CHECK(result);
    result = esp_wifi_start();
    ESP_ERROR_CHECK(result);

    return result;
}

esp_err_t WifiHandler::wifi_connect() {
    if (wifi_ready) {
        this->connecting = true;
        esp_err_t result = esp_wifi_connect();
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "WIFI CONNECTION FAILURE");
            return result;
        }

        return result;
    }
    return ESP_ERR_WIFI_NOT_STARTED;
}

std::string WifiHandler::get_ip() {
    return this->IP;
}


// Unnecessary for basic wifi connectivity test
// esp_err_t esp_wifi_deinit();
// esp_err_t esp_wifi_stop();
// esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);

// esp_err_t disconn_result = esp_wifi_disconnect();
// esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);