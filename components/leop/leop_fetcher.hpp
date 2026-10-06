#include "data.h"

#include <esp_log.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_heap_caps.h>
#include <driver/gptimer.h>
#include <cstring>

class LeopFetcher 
{
public:
  //TODO: Create config for settup up in constructor: URL/port, type of data to be parsed
  // so we can create another instance with separate timer for fetching weather for example
  // make sure UiQueue works from both tasks then aswell
  LeopFetcher(size_t res_buf_size);   
  ~LeopFetcher();

  // No copy, no move
  LeopFetcher(const LeopFetcher&) = delete;
  LeopFetcher& operator=(const LeopFetcher&) = delete;
  LeopFetcher(LeopFetcher&&) noexcept = delete;
  LeopFetcher& operator=(LeopFetcher&&) noexcept = delete;

  // Task function (needs class instance passed as param)
  static void leop_fetch_task(void* param);
  
  // Call this after task has been created to setup hardware timer interval for data fetching
  esp_err_t setup_gptimer(uint32_t interval_ms);

  // Needs to be passed in task registration for timer setup to work
  TaskHandle_t* get_task_handle() { return &task_handle_; }

  // Since callback  of class we need a way for it to see buffer size
  size_t get_res_buf_size() { return res_size_; }
  size_t get_res_buf_len() { return res_len_; }
  char*  get_res_buf() { return res_buf_; }

  // Reset buffer for new read
  void reset_buffer(void) { if (res_buf_) { res_buf_[0] = '\0'; res_len_ = 0; } }

private:
  TaskHandle_t task_handle_ = nullptr;

  // Data members
  char*  res_buf_ = nullptr; // Response buffer
  size_t res_len_ = 0;       // How much is currently stored in buffer
  const size_t res_size_;    // How much fits in buffer (allocates to PSRAM || heap)

  // Data recieve callback (must be static to match C style function pointer)
  static void on_data_recieve(void* ctx, const char* data, size_t len);

  // GPTIMER ISR callback
  static bool IRAM_ATTR on_timer_alarm(
    gptimer_handle_t timer,
    const            gptimer_alarm_event_data_t* edata, 
    void*            user_ctx                           // TaskHandle_t
  );

  // GPTIMER members
  bool gptimer_enabled_          = false;
  gptimer_handle_t timer_handle_ = nullptr;
  uint32_t timer_interval_ms_    = 15 * 60 * 1000; // 15 mins
};
