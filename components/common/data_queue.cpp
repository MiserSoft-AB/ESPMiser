#include "data_queue.hpp"

#include <esp_log.h>

static const char* TAG = "data_queue";

/* Global statics */
static DataQueue::Data   g_data;
static SemaphoreHandle_t g_data_mutex = nullptr;
static QueueHandle_t     g_queue      = nullptr;

// DataQueue::DataQueue() 
// {
//   if (g_queue == nullptr) 
//   {
//     g_queue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
//     // Do we know if it fails? Can it fail?
//   }
// }
// DataQueue::~DataQueue() {}

void DataQueue::data_queue_task(void* param)
{

  // Init queue
  if (g_queue == nullptr) 
  {
    g_queue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
    if (g_queue == nullptr)
    {
      ESP_LOGE(TAG, "Failed to create global data queue");
      return;
    }
  }

  while (true)
  {
    if (xSemaphoreTake(g_data_mutex, portMAX_DELAY))
    {
      xQueueReceive(g_queue, &g_data, 100);
      xSemaphoreGive(g_data_mutex);
    }
  }
}

int DataQueue::send(Data const& data, DataStructType type) 
{
  if (g_queue == nullptr) 
    return 1;

  if (xSemaphoreTake(g_data_mutex, portMAX_DELAY) != pdPASS)
    return 2;

  // Save the recieved data to global
  // Logs not good idea inside semaphore, but their cases shouldn't happen during normal usage
  switch (type) 
  {
    case DATA_NONE:
      ESP_LOGW(TAG, "Data recieved with type none...");
      break;
    case DATA_LEOP:
      g_data.leop.price = data.leop.price;
      // ESP_LOGI(TAG, "Leop Data recieved: %f", g_data.leop.price);
      break;
    case DATA_SENSOR:
      g_data.sensor.temperature = data.sensor.temperature;
      g_data.sensor.pressure    = data.sensor.pressure;
      g_data.sensor.humidity    = data.sensor.humidity;

      // ESP_LOGI(TAG, "Sensors Data recieved: temp: %f, humidity: %f, press: %f.", 
      //     g_data.sensor.temperature,
      //     g_data.sensor.pressure,
      //     g_data.sensor.humidity
      // );
      break;
    case DATA_ALL:
      g_data.leop.price         = data.leop.price;
      g_data.sensor.temperature = data.sensor.temperature;
      g_data.sensor.pressure    = data.sensor.pressure;
      g_data.sensor.humidity    = data.sensor.humidity;

      // ESP_LOGI(TAG, "All data recieved! price: %f temp: %f, humidity: %f, press: %f.", 
      //   g_data.leop.price,
      //   g_data.sensor.temperature,
      //   g_data.sensor.pressure,
      //   g_data.sensor.humidity
      // );
    default:
      ESP_LOGW(TAG, "Unknown data recieved");
      break;
  }
  if (xQueueSend(g_queue, &data, portMAX_DELAY) != pdPASS)
  {
    xSemaphoreGive(g_data_mutex);
    return 3;
  }
  xSemaphoreGive(g_data_mutex);

  return 0;
}

DataQueue::Data DataQueue::get() 
{
  if (xSemaphoreTake(g_data_mutex, portMAX_DELAY) != pdPASS)
  {
    ESP_LOGW(TAG, "Data mutex timed out");
    return g_data;
  }
  Data Out = g_data;
  xSemaphoreGive(g_data_mutex);
  return Out;
}
