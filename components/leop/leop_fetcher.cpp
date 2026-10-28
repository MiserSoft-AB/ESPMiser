#include "leop_fetcher.hpp"
#include "data.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "freertos/idf_additions.h"
#include "sdkconfig.h"
#include "data_queue.hpp"

#include "http_client.hpp"

#include <time.h> // for rand might remove

// --------------------- Privates -------------------------
//
static const char*  TAG = "leop_fetcher";
static const char*  LEOP_ENDPOINT_DAILY_AVG = "api/v1/average-daily?name=";

#define DAILY_INTERVAL_US    (24 * 60 * 60 * 1000 * 1000)
#define FREQUENT_INTERVAL_US (5 * 60 * 1000 * 1000)       // 300,000 us (5 mins)

void LeopFetcher::on_data_recieve(void* ctx, const char* data, size_t len) 
{
  LeopFetcher* This = static_cast<LeopFetcher*>(ctx);

  if (!This || !This->res_buf_ || len <= 0 || !data) 
    return;

  // ESP_LOGI(TAG, "on_recieive: Writing %zu bytes to buffer", len);

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

bool IRAM_ATTR LeopFetcher::on_timer_alarm(
  gptimer_handle_t timer,
  const gptimer_alarm_event_data_t* edata, 
  void* user_ctx) 
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (user_ctx) // Task to notify 
  {
    vTaskNotifyGiveFromISR((TaskHandle_t)user_ctx, &xHigherPriorityTaskWoken);
  }
  return xHigherPriorityTaskWoken == pdTRUE; // Yield
}

// --------------------- Interface -------------------------

LeopFetcher::LeopFetcher(size_t res_buf_size) 
  : res_size_(res_buf_size) 
{ 
  res_buf_ = (char*)heap_caps_malloc(res_buf_size, MALLOC_CAP_SPIRAM);
  if (!res_buf_)
  {
    ESP_LOGW(TAG, "Failed to allocate buffer to PSRAM, falling back to internal heap...");

    res_buf_ = (char*)malloc(res_buf_size); // Fallback to internal heap
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

esp_err_t LeopFetcher::setup_gptimer(uint32_t interval_ms)
{
  esp_err_t err;
  timer_interval_ms_ = interval_ms;

  // Guards
  if (timer_handle_)
  {
    ESP_LOGE(TAG, "timer_handle_ already initialized");
    return ESP_ERR_INVALID_STATE;
  }
  if (!task_handle_)
  {
    ESP_LOGE(TAG, "task_handle_ not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  // Create GPTIMER
  gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000, // 1 MHz = 1 us resolution
  };
  err = gptimer_new_timer(&timer_config, &timer_handle_);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "gptimer_new_timer");
    return err;
  }

  // Register Callback
  // Passing the task handle as user_ctx so the ISR knows which task to notify
  // and our ISR callback to notify it
  gptimer_event_callbacks_t cbs = { 
    .on_alarm = on_timer_alarm
  };
  err = gptimer_register_event_callbacks(timer_handle_, &cbs, task_handle_);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "gptimer_register_event_callbacks");
    gptimer_del_timer(timer_handle_);
    return err;
  }

  // Configure Alarm
  // Convert ms to us
  uint64_t interval_us = (uint64_t)interval_ms * 1000ULL;

  gptimer_alarm_config_t alarm_config = {
    .alarm_count = interval_us,
    .reload_count = 0,
    .flags = { .auto_reload_on_alarm = true }, // Periodic
  };
  err = gptimer_set_alarm_action(timer_handle_, &alarm_config);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "gptimer_set_alarm_action");
    gptimer_del_timer(timer_handle_);
    return err;
  }

  // Enable and start timer
  err = gptimer_enable(timer_handle_);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "gptimer_enable");
    gptimer_del_timer(timer_handle_);
    return err;
  }
  err = gptimer_start(timer_handle_);
  if (err != ESP_OK) 
  {
    ESP_LOGE(TAG, "gptimer_start");
    gptimer_disable(timer_handle_);
    gptimer_del_timer(timer_handle_);
    return err;
  }
  
  // Switch bool to signal that this instance is running timer
  gptimer_enabled_ = true;
  return ESP_OK;
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

  // Local copy of queue data
  DataQueue::Data Qd{};
  Qd.type = DATA_LEOP;

  // Register this task with the Task Watchdog Timer (WDT)
  res = esp_task_wdt_add(nullptr);

  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register network task with TWDT: %s", esp_err_to_name(res));

    vTaskDelete(nullptr);
    return;
  }

  // Reset the watchdog once per second while waiting
  const TickType_t twdt_feed_interval = pdMS_TO_TICKS(1000); // 1 second

  while (1) 
  {
    ESP_LOGI(TAG, "Fetching LEOP data...");

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
    Qd.leop.price = (float)rand() / RAND_MAX;
    ESP_LOGI(TAG, "Sending %f to queue.", Qd.leop.price);

    // Send to consumer queues
    DataQueue::send(Qd);
    // UartQueue::send(UiD);

    // The network task completed another iteration
    res = esp_task_wdt_reset();

    if (res != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to reset network TWDT: %s", esp_err_to_name(res));
    }

    if (This->gptimer_enabled_)
    {
      // Wait for the timer notification in one second sections
      while (ulTaskNotifyTake(pdTRUE, twdt_feed_interval) == 0)
      {
        res = esp_task_wdt_reset();

        if (res != ESP_OK)
        {
          ESP_LOGE(TAG, "Failed to reset network TWDT: %s", esp_err_to_name(res));
        }
      }
    }
    else
    {
      // Fallback: divide the full delay into one second sections
      TickType_t remaining_ticks = pdMS_TO_TICKS(This->timer_interval_ms_);

      while (remaining_ticks > 0)
      {
        const TickType_t wait_ticks = remaining_ticks > twdt_feed_interval ? twdt_feed_interval : remaining_ticks;

        vTaskDelay(wait_ticks);
        remaining_ticks -= wait_ticks;

        res = esp_task_wdt_reset();

        if (res != ESP_OK)
        {
          ESP_LOGE(TAG, "Failed to reset network TWDT: %s", esp_err_to_name(res));
        }
      }
    }
  }
}