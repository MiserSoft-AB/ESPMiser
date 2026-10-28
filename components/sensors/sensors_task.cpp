#include "sensors_task.hpp"
#include "esp_err.h"
#include "ui_queue.hpp"
#include "esp_task_wdt.h"

#include <esp_log.h>
#include <time.h>

static const char* TAG = "sensors";

// Callback för software timer - körs i timer-daemon-task
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
  // Local copy of ui data
  Ui_Data UiD;
  ui_data_init(&UiD);

  Sensors* This = static_cast<Sensors*>(self);

  if (!This)
  {
    ESP_LOGE(TAG, "Invalid parameter passed to sensor task");
    vTaskDelete(nullptr);
    return;
  }

  // Register this task with the Task Watchdog Timer (WDT)
  esp_err_t wdt_result = esp_task_wdt_add(nullptr);

  if (wdt_result != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register sensor task with TWDT: %s", esp_err_to_name(wdt_result));

    vTaskDelete(nullptr);
    return;
  }

  while (1) 
  {
    // Block until semaphore is given (timeout after 2 seconds)
    if (xSemaphoreTake(This->timer_notify_, pdMS_TO_TICKS(2000)) == pdTRUE)
    {
      // Mock sending sensor read data over queue to ui
      srand(time(NULL));

      UiD.type = UI_SENSOR_DATA;
      UiD.sensor_data = {
        .temperature  = (float)rand() / RAND_MAX,
        .humidity     = (float)rand() / RAND_MAX,
        .pressure     = (float)rand() / RAND_MAX,
      };
      ESP_LOGI(TAG, "Sending temp: %f, humidity: %f, press: %f to queue.", 
        UiD.sensor_data.temperature,
        UiD.sensor_data.pressure,
        UiD.sensor_data.humidity
      );

      UiQueue::send(UiD);
    }
    else
    {
      ESP_LOGE(TAG, "Semaphore timed out, ensure timer is started!");
    }

    // tell the watchdog that the sensor task is still responsive
    wdt_result = esp_task_wdt_reset();

    if (wdt_result != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to reset sensor TWDT: %s", esp_err_to_name(wdt_result));
    }
  }
}
