#ifndef __UI_TASK_HPP__
#define __UI_TASK_HPP__

#include "display.hpp"

#include <lvgl.h>
#include <esp_err.h>

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

  esp_err_t init_page_main(Display& Disp);

private:
  // Ui() {} // no instantiation, might reconsider
          // we should learn to use templates though

  Display   display_;
};

#endif // __UI_TASK_HPP__
