#include "leop_fetcher.hpp"
#include "data.h"
#include "freertos/idf_additions.h"
#include "sdkconfig.h"
#include "ui_queue.hpp"

#include "http_client.hpp"

#include <time.h> // for rand might remove

static const char*  TAG = "leop_fetcher";
static const char*  LEOP_ENDPOINT_DAILY_AVG = "api/v1/average-daily?name=";

// C-style callback function for retrieving data from http_client
// Can/should this be part of the LeopServer class?

LeopFetcher::LeopFetcher(size_t res_buf_size) 
  : res_size_(res_buf_size) 
{ 
  res_buf_ = (char*)heap_caps_malloc(res_buf_size, MALLOC_CAP_SPIRAM);
  if (!res_buf_)
  {
    ESP_LOGW(TAG, "Failed to allocate buffer to PSRAM, falling back to internal heap...");

    res_buf_ = (char*)malloc(res_size_); // Fallback to internal heap
    if (!res_buf_) 
    {
      ESP_LOGE(TAG, "Out of memory entirely...");
      return;
    }
  }
  ESP_LOGI(TAG, "LeopFetcher has been created with response buffer size: %zu", res_size_);
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
  size_t url_len = snprintf(url, 256, "%s:%s/%s%s", 
    CONFIG_LEOP_SERVER_URL, 
    CONFIG_LEOP_SERVER_PORT, 
    LEOP_ENDPOINT_DAILY_AVG, 
    CONFIG_LEOP_SERVER_FACILITY_NAME);
  
  if (url_len >= 256)
  {
    ESP_LOGE(TAG, "URL too long, was truncated.");
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
    // Clean response buffer
    This->reset_buffer();

    // --- Call for new data (blocking)
    res = Http.get(url, &LeopFetcher::on_data_recieve, This);
    
    // --- Parse data
    if (res == ESP_OK && This->get_res_buf() != nullptr)
    {
      ESP_LOGI(TAG, "LEOP Response len:\n %zu", This->get_res_buf_len());
      ESP_LOGI(TAG, "LEOP Response:\n %s", This->get_res_buf());
    }

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

  if (!This || !This->res_buf_ || len <= 0 || !data) 
    return;

  ESP_LOGI(TAG, "on_recieive: Writing %zu bytes to buffer", len);

  // Make sure we have enough space left in buffer (+1 for nullterm)
  if (This->res_len_ + len + 1 > This->res_size_) 
  {
    ESP_LOGW(TAG, "Response too large, truncating");
    len = This->res_size_ - This->res_len_ - 1;
  }
  
  // Write data to response buffer from last index
  memcpy(&This->res_buf_[This->res_len_], data, len);  
  This->res_len_ += len;
  This->res_buf_[This->res_len_] = '\0'; // nullterm last byte
}
