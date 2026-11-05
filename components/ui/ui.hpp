#ifndef __UI_TASK_HPP__
#define __UI_TASK_HPP__

#include "display.hpp"
#include "data_queue.hpp"

#include <esp_lv_adapter.h>
#include <esp_err.h>
#include <lvgl.h>

class Ui 
{
public:
  Ui() {}; 
  ~Ui() {};

  // No copy, no move
  Ui(const Ui&) = delete;
  Ui& operator=(const Ui&) = delete;
  Ui(Ui&&) noexcept = delete;
  Ui& operator=(Ui&&) noexcept = delete;

  // Must be run before task is started
  esp_err_t init();

  // Must pass inited Ui class as param 
  static void ui_task(void* param);

  // Build the main page widgets. Must run with display ready (LVGL initialized).
  esp_err_t init_page_main();

  // Push latest queue data into the widgets.
  void update(const DataQueue::Data& Qd);

private:
  Display         display_;
  DataQueue::Data Qd_{};

  lv_obj_t* label_price_    = nullptr;
  lv_obj_t* label_temp_     = nullptr;
  lv_obj_t* label_humidity_ = nullptr;
  lv_obj_t* label_pressure_ = nullptr;
};

#endif // __UI_TASK_HPP__
