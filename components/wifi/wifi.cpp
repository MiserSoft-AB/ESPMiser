#include "wifi.hpp"

static const char* TAG = "WIFIHANDLER";

void WifiHandler::wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) 
{
    WifiHandler* self = static_cast<WifiHandler*>(arg);
    if (event_base == WIFI_EVENT) {
      switch (event_id) {
          case WIFI_EVENT_STA_START: {
              esp_wifi_connect();
              break;
          }

          case WIFI_EVENT_STA_CONNECTED: {
              xEventGroupSetBits(self->event_group_, WIFI_CONNECTED_BIT);
              break;
          }

          case WIFI_EVENT_STA_DISCONNECTED: {
              xEventGroupClearBits(self->event_group_, WIFI_CONNECTED_BIT | WIFI_GOT_IP_BIT);
              ESP_LOGI(TAG, "event connecting ....");

              // Simple reconnect (add retry/backoff in production)
              esp_wifi_connect();
              break;
          }
          default:
              break;
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(self->event_group_, WIFI_GOT_IP_BIT);
    }
}

esp_err_t WifiHandler::wifi_init() {
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
    ESP_LOGI(TAG, "INIT SUCCESS"); //DBG
    
    // Register event group and handlers for WIFI and IP events
    event_group_ = xEventGroupCreate();
    if (event_group_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    result = esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "HANDLER ACTIVATION FAILURE");
        return result;
    }
    result = esp_event_handler_register(
        IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this);
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "HANDLER ACTIVATION FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "EVENT HANDLER(S) REGISTER SUCCESS"); //DBG

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

esp_err_t WifiHandler::wifi_wait_for_connect() {
    EventBits_t bits = xEventGroupWaitBits(
        event_group_,
        WIFI_GOT_IP_BIT,    // wait for IP
        pdTRUE,        // clear on exit
        pdFALSE,       // don't clear on wait
        portMAX_DELAY  // or a timeout in ticks
    );

    if (bits & WIFI_GOT_IP_BIT) {
        ESP_LOGI(TAG, "WIFI CONNECTION SUCCESS");
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "WIFI CONNECTION FAILURE / TIMEOUT");
        return ESP_FAIL;
    }

    // Connecting will be handled by the event handler instead
    /*
    esp_err_t result = esp_wifi_connect();
    ESP_ERROR_CHECK(result);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "WIFI CONNECTION FAILURE");
        return result;
    }
    ESP_LOGI(TAG, "WIFI CONNECTION SUCCESS"); //DBG

    return result;
    */
}

std::string WifiHandler::get_ip() {

    return "Not implemented :(";
}


// Unnecessary for basic wifi connectivity test
// esp_err_t esp_wifi_deinit();
// esp_err_t esp_wifi_stop();
// esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);

// esp_err_t disconn_result = esp_wifi_disconnect();
// esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
