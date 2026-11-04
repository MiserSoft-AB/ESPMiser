#include "ui.hpp"
#include "data.h"
#include "data_queue.hpp"
#include "freertos/projdefs.h"
#include <esp_log.h>

namespace 
{
  static const char* TAG = "ui";
  constexpr uint32_t UI_REFRESH_MS = 2000;

  struct DisplayBuffer
  {
    uint32_t refresh_count;
  };

  DisplayBuffer ui_buffer{};
  TaskHandle_t  ui_task_handle = nullptr;

  void ui_timer_callback(TimerHandle_t timer)
  {
    (void)timer;

    if (ui_task_handle != nullptr)
    {
      xTaskNotifyGive(ui_task_handle);
    }
  }

} // Namespace

// esp_err_t Ui::init_page_main(Display& Disp)
// {
//   /* Create full-screen black background */
//   lv_screen = lv_scr_act();  // Active screen
//   lv_obj_set_style_bg_color(lv_screen, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
//
//   /* Add label with ASCII art */
//   lv_label_main = lv_label_create(lv_screen);
//   lv_obj_align(lv_label_main, LV_ALIGN_CENTER, 0, 0);
//   lv_obj_set_style_text_font(lv_label_main, &notosans_14, 0);
//   lv_obj_set_style_text_color(lv_label_main, lv_color_hex(0x32CD32), LV_PART_MAIN | LV_STATE_DEFAULT);
//
// }

esp_err_t Ui::init()
{
  // Init display
  if (display_.is_ready())
    return ESP_OK;

  esp_err_t err = display_.init();

  return err;
}

void Ui::ui_task(void* param) 
{
  Ui* This = (Ui*)param;
  if (!This || !This->display_.is_ready())
  {
    ESP_LOGE(TAG, "Invalid arg");
    vTaskDelete(nullptr);
    return;
  }

  // Local copy of queue data
  DataQueue::Data Qd;
  data_leop_init(&Qd.leop);
  data_sensor_init(&Qd.sensor);
  data_wifi_init(&Qd.wifi);

  // Setup software timer
  ui_task_handle = xTaskGetCurrentTaskHandle();
  static StaticTimer_t timer_storage;
  TimerHandle_t refresh_timer = xTimerCreateStatic
  (
    "ui_refresh",
    pdMS_TO_TICKS(UI_REFRESH_MS),
    pdTRUE,
    nullptr,
    ui_timer_callback,
    &timer_storage
  );

  if (refresh_timer == nullptr)
  {
    ESP_LOGE(TAG, "Failed to create ui timer");
    ui_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  if (xTimerStart(refresh_timer, pdMS_TO_TICKS(100)) != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to start ui timer");
    ui_task_handle = nullptr;
    vTaskDelete(nullptr);
    return;
  }
  ESP_LOGI(TAG, "Ui timer start requested");

  while (true) 
  {
    // Get timer notification
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000)) == 0) 
    {
      ESP_LOGW(TAG, "No display timer notification within 3 seconds");
      continue;
    }

    // Refresh local data copy from data queue
    // Might just want to use software timer and pass zero timeout to queue for a try-lock
    if (DataQueue::get(&Qd, 0) == 0) 
    {
      
      ESP_LOGI(TAG, "UI data updated! price: %f temp: %f, humidity: %f, press: %f.", 
        Qd.leop.price,
        Qd.sensor.temperature,
        Qd.sensor.pressure,
        Qd.sensor.humidity
      );
    }
    else
      ESP_LOGW(TAG, "Didn't update queue data");
    
    if (ui_buffer.refresh_count % 2 == 0) 
      This->display_.backlight_set(0x00);
    else 
      This->display_.backlight_set(BACKLIGHT_MASK);


    ui_buffer.refresh_count++;

    ESP_LOGI(TAG, "UI buffer update frame %lu", static_cast<unsigned long>(ui_buffer.refresh_count));

  }
}

