#include "leop_fetcher.hpp"
#include "data.h"
#include "freertos/idf_additions.h"
#include "ui_queue.hpp"

#include "http_client.hpp"

#include <time.h> // for rand might remove

static const char*  TAG = "leop_fetcher";
static const char*  LEOP_ENDPOINT_DAILY_AVG = "api/v1/average-daily";

// C-style callback function for retrieving data from http_client
// Can/should this be part of the LeopServer class?

LeopFetcher::LeopFetcher(size_t res_buf_size) 
  : res_size_(res_buf_size) 
{ 
  res_buf_ = (char*)heap_caps_malloc(res_buf_size, MALLOC_CAP_SPIRAM);
  if (!res_buf_)
  {
    ESP_LOGW(TAG, "Failed to allocate buffer to PSRAM! Falling back to internal heap");

    res_buf_ = (char*)malloc(res_size_); // Fallback to internal heap
    if (!res_buf_) 
    {
      ESP_LOGE(TAG, "Out of memory entirely...");
      return;
    }
  }
}

LeopFetcher::~LeopFetcher()
{
  if (res_buf_)
    free(res_buf_);
}

void LeopFetcher::leop_fetch_task(void* param) 
{
  esp_err_t res;
  char url[256];

  // Get pointer to this from param (since function is static)
  LeopFetcher* This = static_cast<LeopFetcher*>(param);
  if (!This || !This->res_buf_) 
  {
    ESP_LOGE(TAG, "Invalid param passed to task");
    return;
  }

  // Define URL to LEOP server
  size_t url_len = snprintf(url, 256, "%s:%s/%s", 
    CONFIG_LEOP_SERVER_URL, CONFIG_LEOP_SERVER_PORT, LEOP_ENDPOINT_DAILY_AVG);
  
  if (url_len >= 256)
  {
    ESP_LOGE(TAG, "URL too long, was truncated. Increase buffer size and rebuild program (oh, you're not a programmer with source code? That's too bad lol)");
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGD(TAG, "URL set: %s", url);

  // Initialize http client
  EspHttpClient::Config HttpCfg = {
    .timeout_ms = 1000,
  };
  EspHttpClient Http(HttpCfg);
  res = Http.client_init();
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "client_init failed");
    vTaskDelete(NULL);
    return;
  }

  // Local copy of ui data
  Ui_Data UiD;
  ui_data_init(&UiD);

  while (1) 
  {
    // --- Call for new data (blocking)
    res = Http.get(url, &LeopFetcher::on_data_recieve, This);
    
    // --- Parse data
    ESP_LOGD(TAG, "LEOP Response:\n %s", This->get_res_buf());

    // --- Send new parsed data to UI Queue
    // Mock sending data over queue to ui
    srand(time(NULL));
    UiD.type = UI_LEOP_DATA;
    UiD.leop_data.price = (float)rand() / RAND_MAX;
    ESP_LOGI(TAG, "Sending %f to queue.", UiD.leop_data.price);

    UiQueue::send(UiD);

    vTaskDelay(pdMS_TO_TICKS(5000)); // TODO: Implement ISR and GPTIMER instead
  }
}

void LeopFetcher::on_data_recieve(void* ctx, const char* data, size_t len) 
{
  LeopFetcher* This = static_cast<LeopFetcher*>(ctx);

  if (!This || !This->res_buf_) 
  {
    ESP_LOGE(TAG, "Invalid context passed");
    return;
  }

  // Make sure we have enough space left in buffer
  if (This->res_len_ + len > This->res_size_) 
  {
    ESP_LOGW(TAG, "Response too large, truncating");
    len = This->res_size_ - This->res_len_;
  }
  
  // Write data to response buffer from last index
  memcpy(&This->res_buf_[This->res_len_], data, len);  
  This->res_len_ += len;
}


//
// void LeopFetcher::leop_fetch_task_test(void* params) 
// {
//   // Local copy of ui data
//   Ui_Data UiD;
//   ui_data_init(&UiD);
//
//   while (1) 
//   {
//     // Mock sending data over queue to ui
//     srand(time(NULL));
//     UiD.type = UI_LEOP_DATA;
//     UiD.leop_data.price = (float)rand() / RAND_MAX;
//     ESP_LOGI(TAG, "Sending %f to queue.", UiD.leop_data.price);
//
//     UiQueue::send(UiD);
//
//     vTaskDelay(pdMS_TO_TICKS(5000));
//   }
// }
