#include "sysmon_task.hpp"
// #include "esp_log_level.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"

#include "esp_chip_info.h"

static const char* TAG = "sysmon";
static const size_t MAX_TASKS = 16;

esp_err_t MiserSysMon::start() 
{
  esp_err_t res = ESP_OK;

  // Allocate and construct SysMon on heap location
  MiserSysMon* SysMon = new MiserSysMon();
  if (!SysMon)
  {
    ESP_LOGE(TAG, "Failed to allocate SysMon to heap! Consider rebooting or another career");
    res = ESP_ERR_NO_MEM;
    return res;
  }

  // // Init WDT watchdog timer
  // This is done automatically 
  // esp_task_wdt_config_t twdt_config = {
  //   .timeout_ms = 5000,
  //   .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1, // Monitor both cores
  //   .trigger_panic = false   // DON'T PANIC, Reset on watchdog timeout
  // };
  //
  // res = esp_task_wdt_init(&twdt_config);
  // if (res != ESP_OK)
  // {
  //   ESP_LOGE(TAG, "Watchdog failed to init: %s", esp_err_to_name(res));
  //   res = ESP_ERR_INVALID_STATE;
  //   return res;
  // }

  // Init  and enable temp sensor
  // Setting min and max temperature range between 20 and 70 degrees
  // temperature_sensor_handle_t temp_handle = NULL;
  temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 60);
  // res = temperature_sensor_install(&temp_sensor_config, &temp_handle);
  res = temperature_sensor_install(&temp_sensor_config, &(SysMon->temp_handle_));
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "Internal ESP temp sensor failed to init: %s", esp_err_to_name(res));
    res = ESP_ERR_INVALID_STATE;
    return res;
  }
  res = temperature_sensor_enable(SysMon->temp_handle_);
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "Internal ESP temp sensor failed to enable: %s", esp_err_to_name(res));
    res = ESP_ERR_INVALID_STATE;
    return res;
  }

  xTaskCreatePinnedToCore(
    [](void* arg) { static_cast<MiserSysMon*>(arg)->run(); },
    "SysMon",
    4096, // Stack size
    SysMon,
    5,    // Mid priority
    nullptr,
    0     // Core 0
  );

  return res;
}

float MiserSysMon::get_temp()
{
  if (temp_handle_ == NULL)
  {
    ESP_LOGE(TAG, "temp_handle_ is NULL, can't read temp");
    return 0.0;
  }

  float temp;
  if (temperature_sensor_get_celsius(temp_handle_, &temp) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read temp");
    return 0.0;
  }

  return temp;
}

void MiserSysMon::run() 
{
  esp_task_wdt_add(NULL); // Add current task to watchdog 

  // Some intial info
  static char model_info[87] = {0};
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  snprintf(model_info, sizeof(model_info), 
    "Model: ESP32-S3 with %i cores\n Features:%s%s%s%s%s\n",
    chip_info.cores,
    (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? " 802.11bgn" : "",
    (chip_info.features & CHIP_FEATURE_BLE) ? " BLE" : "",
    (chip_info.features & CHIP_FEATURE_IEEE802154) ? " IEEE802154" : "",
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? " SD Flash" : "",
    (chip_info.features & CHIP_FEATURE_BT) ? " Bluetooth" : "");

  ESP_LOGI(TAG, "--- ESPMiser board info --- \n%s", model_info);

  // Periodic checks
  while (true) 
  {
    // 1. Feed Watchdog
    esp_task_wdt_reset();

    // 2. Check Heap
    size_t free_heap = esp_get_free_heap_size();
    if (free_heap < heap_warning_threshold) 
    {
      ESP_LOGW(TAG, "Low heap: %zu bytes", free_heap);
    }
    ESP_LOGI(TAG, "Heap available (bytes): %d", esp_get_free_heap_size());

    // 3. Inspect Task Stacks 
    // NOTE: is expensive, potentially remove if overhead is needed
    static TaskStatus_t tasks[MAX_TASKS];
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    ESP_LOGI(TAG, "Tasks managed by FreeRTOS: %d", (unsigned)task_count);

    if (task_count > MAX_TASKS) 
    {
      ESP_LOGW(TAG, "Too many tasks: %u, array capacity: %u",
        (unsigned)task_count, (unsigned)MAX_TASKS);
      task_count = MAX_TASKS;
    }

    UBaseType_t reported_count =
      uxTaskGetSystemState(tasks, task_count, NULL);

    ESP_LOGI(TAG, "Tasks registered: %u", (unsigned)reported_count);

    // Checks for individual tasks health
    for (UBaseType_t i = 0; i < reported_count; i++) {

      // Check used stack space
      if (tasks[i].usStackHighWaterMark < stack_warning_threshold) 
      {
        ESP_LOGW(TAG,
          "Task '%s' has only %u words of stack remaining",
          tasks[i].pcTaskName,
          (unsigned)tasks[i].usStackHighWaterMark);
      }

      // Debug log task names
      ESP_LOGI(TAG, "Task %i: %s", i, tasks[i].pcTaskName);
    }
    // 4. WiFi Status ?

    // 5. Get silicon temperature
    ESP_LOGI(TAG, "Chip temp: %.1fC", get_temp());

    vTaskDelay(pdMS_TO_TICKS(1000)); // Run every second
  }
}
