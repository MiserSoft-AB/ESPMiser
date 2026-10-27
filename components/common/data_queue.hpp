#ifndef __DATA_QUEUE_HPP__
#define __DATA_QUEUE_HPP__

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
  struct Data
  {
    System_Data    system;
    Wifi_Data      wifi;
    Leop_Data      leop;
    Sensor_Data    sensor;

    DataStructType type;
  };

  static const int    QUEUE_LENGTH = 16; // TODO: optimize perfect size
  static const size_t ITEM_SIZE    = sizeof(Data);

  // Initialize the queue, mutex and merge task. Call once from app_main.
  static int init();

  // Called by producers to set data.
  // Only data specified by in member 'type' will be copied
  static int send(Data const& in);

  // Copies the current merged snapshot into out.
  // Only data specified by out member 'type' will be copied
  // Timeout sets how long to wait for mutex, set 0 for a try-lock if blocking is unacceptable
  static int get(Data* out, TickType_t timeout);

private:
  DataQueue() {}; // No instantiation

  // Task function continuously reads from queue and merges into snapshot
  static void data_queue_task(void* param);
};

#endif // __DATA_QUEUE_HPP__
