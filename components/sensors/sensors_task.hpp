#ifndef __SENSORS_TASK_HPP__
#define __SENSORS_TASK_HPP__

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

class Sensors 
{
public:
  Sensors() {};
  ~Sensors() {};

  // No copy, no move
  Sensors(const Sensors&) = delete;
  Sensors& operator=(const Sensors&) = delete;
  Sensors(Sensors&&) noexcept = delete;
  Sensors& operator=(Sensors&&) noexcept = delete;

  // Sets up the software timer
  // Must be run before task is started!
  // timeout_ms = timer notify interval
  esp_err_t timer_setup(int timeout_ms);

  // Task function (needs class instance passed as param)
  static void sensors_task(void* self);

private:

  // Timer members
  TimerHandle_t     timer_handle_ = nullptr;
  SemaphoreHandle_t timer_notify_ = nullptr;
  uint32_t          timer_read_ms_ = 500;

  // Software timer callback
  static void on_timer_signal(TimerHandle_t xTimer);
};

#endif // __SENSORS_TASK_HPP__
