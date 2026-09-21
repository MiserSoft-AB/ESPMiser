#include "data.h"

#include <esp_log.h>
#include <esp_err.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// class LeopFetcher 
// {
// public:
//
//
//   LeopFetcher(Leop_Data& Data);
//
//
//   // This runs constructor and allocates it on heap
//   // static esp_err_t start(); 
//
//
// private:
//   Leop_Data Data;
//   QueueHandle_t& Display_Qeueue = NULL;
//
// };

class LeopFetcher {
public:
  // You better pass a QueueHandle_t param or else! 
  static void leop_fetch_task_test(void* param);
};
