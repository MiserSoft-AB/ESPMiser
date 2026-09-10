#include "tasks/tasks.hpp"
#include "http_client.hpp"

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>

void network_task (void* parameter) 
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