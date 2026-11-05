#include "ui.hpp"
#include "data.h"
#include "data_queue.hpp"
#include "freertos/projdefs.h"
#include <esp_log.h>
#include <lvgl.h>

extern const lv_font_t notosans_14;

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

esp_err_t Ui::init_page_main()
{
  // (void)Disp;

  // if (esp_lv_adapter_lock(-1) != ESP_OK) {
  //   ESP_LOGE(TAG, "Failed to take LVGL lock");
  //   return ESP_FAIL;
  // }

  if (!lv_scr_act())
  {
    ESP_LOGE(TAG, "No active screen, display not ready");
    return ESP_ERR_INVALID_STATE;
  }

  if (esp_lv_adapter_lock(-1) == ESP_OK) {

    // Full-screen black background
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF54927), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Test: solid blue circle, no text — tests if ANY child widget renders
    lv_obj_t* circle = lv_obj_create(scr);
    lv_obj_set_size(circle, 100, 100);
    lv_obj_set_style_radius(circle, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(circle, LV_ALIGN_CENTER, -150, 0);

    // Test: solid green square, no text
    lv_obj_t* square = lv_obj_create(scr);
    lv_obj_set_size(square, 80, 80);
    lv_obj_set_style_radius(square, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(square, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(square, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(square, LV_ALIGN_CENTER, 150, 0);

    // Minimal test: one label with the built-in default font
    lv_obj_t* title = lv_label_create(scr);
    lv_label_set_text(title, "LVGL works!");
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -20);

    ESP_LOGI(TAG, "Label created: %p, text='%s'", (void*)title, lv_label_get_text(title));

    // Second test: explicit Montserrat 14 font
    lv_obj_t* title2 = lv_label_create(scr);
    lv_label_set_text(title2, "Montserrat 14");
    lv_obj_align(title2, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_text_font(title2, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Force an immediate redraw of the whole screen so we don't have to wait for a refresh cycle
    lv_obj_invalidate(scr);

    esp_lv_adapter_refresh_now(display_.lv_disp_get());
    esp_lv_adapter_unlock();
  }

  ESP_LOGI(TAG, "Main page initialized");
  return ESP_OK;
}

void Ui::update(const DataQueue::Data& Qd)
{
  char buf[32];

  if (esp_lv_adapter_lock(-1) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to take LVGL lock");
      return;
  }

  if (label_price_)
  {
    lv_snprintf(buf, sizeof(buf), "%.2f kr", Qd.leop.price);
    lv_label_set_text(label_price_, buf);
  }

  if (label_temp_)
  {
    lv_snprintf(buf, sizeof(buf), "%.1f", Qd.sensor.temperature);
    lv_label_set_text(label_temp_, buf);
  }

  if (label_humidity_)
  {
    lv_snprintf(buf, sizeof(buf), "%.1f", Qd.sensor.humidity);
    lv_label_set_text(label_humidity_, buf);
  }

  if (label_pressure_)
  {
    lv_snprintf(buf, sizeof(buf), "%.1f", Qd.sensor.pressure);
    lv_label_set_text(label_pressure_, buf);
  }

  esp_lv_adapter_refresh_now(display_.lv_disp_get());
  esp_lv_adapter_unlock();
}

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

  // Build the main page once before entering the loop
  esp_err_t err = This->init_page_main();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to init main page: %s", esp_err_to_name(err));
    vTaskDelete(nullptr);
    return;
  }

  // Local copy of queue data
  // data_init_system(&Qd.system);
  // data_leop_init(&Qd.leop);
  // data_sensor_init(&Qd.sensor);
  // data_wifi_init(&Qd.wifi);

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
    if (DataQueue::get(&This->Qd_, 0) == 0) 
    {
      This->update(This->Qd_);

      ESP_LOGI(TAG, "UI data updated! price: %f temp: %f, humidity: %f, press: %f.", 
        This->Qd_.leop.price,
        This->Qd_.sensor.temperature,
        This->Qd_.sensor.pressure,
        This->Qd_.sensor.humidity
      );
    }
    else
      ESP_LOGW(TAG, "Didn't update queue data");

    ui_buffer.refresh_count++;

    ESP_LOGI(TAG, "UI buffer update frame %lu", static_cast<unsigned long>(ui_buffer.refresh_count));

  }
}
