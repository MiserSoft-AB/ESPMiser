#ifndef __DATA_UPDATER_HPP__
#define __DATA_UPDATER_HPP__

/* Component for producers to send common data 
 * and "consumers" to get produced data */

#include <freertos/FreeRTOS.h>
#include <freertos/projdefs.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "data.h"

class DataQueue 
{
public:
  // Mybe better to typedef this and put it in data.h as Queue_Data?
  struct Data
  {
    // System_Data    system;
    Wifi_Data      wifi;
    Leop_Data      leop;
    Sensor_Data    sensor;
  };

  static const int    QUEUE_LENGTH = 16; // TODO: optimize perfect size
  static const size_t ITEM_SIZE = sizeof(Data);

  // Creates the queue
  // DataQueue();
  // ~DataQueue();
  //
  // // No copy, no move
  // DataQueue(const DataQueue&) = delete;
  // DataQueue& operator=(const DataQueue&) = delete;
  // DataQueue(DataQueue&&) noexcept = delete;
  // DataQueue& operator=(DataQueue&&) noexcept = delete;

  // Task function continuously reads from queue
  // Pass pointer to TickType_t to set recieve interval 
  static void data_queue_task(void* param);

  // Called by producers to set data
  // Returns 1 on invalid state, 2 on mutex failure, 3 on sending failure
  static int send(Data const& data, DataStructType type);

  // Returns copy of current Data object
  static Data get();

  // Optional: Check if data is available without blocking
  // static bool hasData() {
  //     if (queue == nullptr) return false;
  //     return uxQueueMessagesWaiting(queue) > 0;
  // }

private:
  // static Data   q_buf_[QUEUE_LENGTH];
  DataQueue() {};

};

#endif // __DATA_UPDATER_HPP__
