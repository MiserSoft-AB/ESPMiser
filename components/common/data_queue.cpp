#include "data_queue.hpp"
#include "data.h"

#include <esp_log.h>

static const char* TAG = "data_queue";

// .bss statics
static DataQueue::Data    g_data{};
static QueueHandle_t      g_queue  = nullptr;
static SemaphoreHandle_t  g_mutex  = nullptr;
static volatile bool      g_ready  = false; // if inited

int DataQueue::init() 
{
  if (g_queue != nullptr) 
    return -1; // Already initialized

  g_queue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
  if (g_queue == nullptr) 
  {
    ESP_LOGE(TAG, "Failed to create data queue");
    return 1; // xQueueCreate
  }

  g_mutex = xSemaphoreCreateMutex();
  if (g_mutex == nullptr) 
  {
    ESP_LOGE(TAG, "Failed to create data mutex");
    vQueueDelete(g_queue);
    g_queue = nullptr;
    return 2; // xSemaphoreCreateMutex
  }

  // Start merge task: drains queue and updates snapshot under lock
  TaskHandle_t task_handle = nullptr;
  if (xTaskCreate(
        data_queue_task,
        "data_q_task",
        2048,
        nullptr,
        5,
        &task_handle) 
      != pdPASS) 
  {
    ESP_LOGE(TAG, "Failed to create data queue task");
    vSemaphoreDelete(g_mutex);
    g_mutex = nullptr;
    vQueueDelete(g_queue);
    g_queue = nullptr;
    return 4;
  }
  
  g_ready = true;
  return 0;
}

void DataQueue::data_queue_task(void* param) 
{
  (void)param;

  DataQueue::Data item; // queue item
  while (true) 
  {
    // Block until a producer sends something
    if (xQueueReceive(g_queue, &item, pdMS_TO_TICKS(100)) != pdPASS) 
      continue;

    if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(50)) != pdPASS) 
    {
      ESP_LOGW(TAG, "Something's blocking the mutex...");
      continue; // Should never happen
    }

    switch (item.type)
    {
      case DATA_ALL:
        g_data = item; 
        break;
      case DATA_SYSTEM:
        g_data.system = item.system;
        break;
      case DATA_WIFI:
        g_data.wifi = item.wifi;
        break;
      case DATA_LEOP:
        g_data.leop = item.leop;
        break;
      case DATA_SENSOR:
        g_data.sensor = item.sensor;
        break;
      case DATA_NONE:
        break;
      default: break;
    }

    // g_data.wifi   = item.wifi;
    // g_data.leop   = item.leop;
    // g_data.sensor = item.sensor;

    xSemaphoreGive(g_mutex);
  }
}

int DataQueue::send(Data const& data) 
{
  if (g_queue == nullptr || !g_ready) 
    return 1; // Not initialized yet

  // Queue send is the only synchronization needed here:
  // producers never touch g_data directly, so no mutex required.
  if (xQueueSend(g_queue, &data, pdMS_TO_TICKS(50)) != pdPASS) 
  {
    ESP_LOGW(TAG, "Data queue full, dropping stale update (type %d)", (int)data.type);
    return 2;
  }

  return 0;
}

int DataQueue::get(Data* out, TickType_t timeout)
{
  if (out == nullptr || g_mutex == nullptr || !g_ready) 
    return 1;

  if (xSemaphoreTake(g_mutex, timeout) != pdPASS) 
  {
    ESP_LOGW(TAG, "Data mutex timed out");
    return 2;
  }

  // Copy the wanted data from global to out pointer
  switch (out->type)
  {
    case DATA_ALL:
      *out = g_data;
      break;
    case DATA_SYSTEM:
      (*out).system = g_data.system;
      break;
    case DATA_WIFI:
      (*out).wifi = g_data.wifi;
      break;
    case DATA_LEOP:
      (*out).leop = g_data.leop;
      break;
    case DATA_SENSOR:
      (*out).sensor = g_data.sensor;
      break;
    case DATA_NONE:
      break;
    default:
      break;
  }

  xSemaphoreGive(g_mutex);
  return 0;
}

