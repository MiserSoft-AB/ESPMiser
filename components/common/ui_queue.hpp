#ifndef __UI_UPDATER_HPP__
#define __UI_UPDATER_HPP__

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "data.h"

class UiQueue 
{
public:
  static const int QUEUE_LENGTH = 16; // TODO: optimize perfect size
  static const size_t ITEM_SIZE = sizeof(Ui_Data);

  // Static queue handle and buffer
  static QueueHandle_t queue;
  static Ui_Data q_buf[QUEUE_LENGTH];

  static void init();

  // Called by Sensor/Network tasks
  static int send(Ui_Data const& data);

  // Called by UI task
  static int receive(Ui_Data& out_data, TickType_t timeout_ticks);


  // Optional: Check if data is available without blocking
  // static bool hasData() {
  //     if (queue == nullptr) return false;
  //     return uxQueueMessagesWaiting(queue) > 0;
  // }

private:
    UiQueue() {} // Prevent instantiation
};

#endif // __UI_UPDATER_HPP__
