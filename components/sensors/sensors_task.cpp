#include "sensors_task.hpp"
#include "sensors_update.hpp"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "data_queue.hpp"

#include <esp_log.h>

static const char* TAG = "sensors";


void Sensors::on_timer_signal(TimerHandle_t xTimer)
{
  // Retrieve class pointer stored in the timer id
  Sensors* This = static_cast<Sensors*>(pvTimerGetTimerID(xTimer));
  xSemaphoreGive(This->timer_notify_);
}

esp_err_t Sensors::timer_setup(int timeout_ms)
{
  // Create Binary Semaphore (initial state: empty)
  timer_notify_ = xSemaphoreCreateBinary();
  if (timer_notify_ == NULL) 
  {
    ESP_LOGE(TAG, "Failed to create semaphore");
    return ESP_FAIL;
  }

  // Create timer
  timer_handle_ = xTimerCreate(
    "SensorTimer",
    pdMS_TO_TICKS(timeout_ms),
    pdTRUE,
    static_cast<void*>(this), // Passing pointer to this as timer id
    &Sensors::on_timer_signal
  );

  if (!timer_handle_) 
  {
    ESP_LOGE(TAG, "xTimerCreate");
    return ESP_FAIL;
  }
  xTimerStart(timer_handle_, 0);

  return ESP_OK;
}

// Static task
void Sensors::sensors_task(void* self)
{
  DataQueue::Data Qd{};
  Qd.type = DATA_SENSOR;

  Sensors* This = static_cast<Sensors*>(self);
  if (!This)
  {
    ESP_LOGE(TAG, "Invalid parameter passed to sensor task");
    vTaskDelete(nullptr);
    return;
  }

  const esp_err_t init_result = This->sensor_.init();
  if (init_result != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize sensor: %s", esp_err_to_name(init_result));
    vTaskDelete(nullptr);
    return;
  }

  esp_err_t wdt_result = esp_task_wdt_add(nullptr);
  if (wdt_result != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register sensor task with WDT: %s", esp_err_to_name(wdt_result));
    vTaskDelete(nullptr);
    return;
  }

  while (true)
  {
    const SensorsUpdate update = read_sensor_update(This->sensor_);
    const bool timer_signaled =
      xSemaphoreTake(This->timer_notify_, pdMS_TO_TICKS(2000)) == pdTRUE;

    if (update.should_publish(timer_signaled))
    {
      Qd.sensor = {
        .temperature = update.readings.temperature,
        .humidity = update.readings.humidity,
        .pressure = update.readings.pressure
      };
      DataQueue::send(Qd);
    }
    else if (!timer_signaled)
    {
      ESP_LOGE(TAG, "Semaphore timed out, ensure timer is started");
    }
    else
    {
      ESP_LOGW(TAG, "Failed to read sensor data: %s",
               esp_err_to_name(update.read_result));
    }

    wdt_result = esp_task_wdt_reset();
    if (wdt_result != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to reset sensor WDT: %s", esp_err_to_name(wdt_result));
    }
  }
}
