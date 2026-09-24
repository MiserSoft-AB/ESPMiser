#include "data.h"

#include <esp_log.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_heap_caps.h>
#include <cstring>

class LeopFetcher 
{
public:
  LeopFetcher(size_t res_buf_size);   
  ~LeopFetcher();

  // No copy, no move
  LeopFetcher(const LeopFetcher&) = delete;
  LeopFetcher& operator=(const LeopFetcher&) = delete;
  LeopFetcher(LeopFetcher&&) noexcept = delete;
  LeopFetcher& operator=(LeopFetcher&&) noexcept = delete;

  // static void leop_fetch_task_test(void* param);

  // Task function (needs class instance passed as param)
  static void leop_fetch_task(void* param);
  
  // Since callback  of class we need a way for it to see buffer size
  size_t get_res_buf_size() { return res_size_; }
  size_t get_res_buf_len() { return res_len_; }
  char* get_res_buf() { return res_buf_; }

private:
  char*  res_buf_ = nullptr; // Response buffer
  size_t res_len_ = 0;       // How much is currently stored in buffer
  const size_t res_size_;    // How much fits in buffer (allocates to PSRAM || heap)

  // Callback function (must be static to match C style function pointer)
  // Pass 
  static void on_data_recieve(void* ctx, const char* data, size_t len);

};
