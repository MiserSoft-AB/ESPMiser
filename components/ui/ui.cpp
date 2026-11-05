#include "ui.hpp"
#include "display_update.hpp"
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
  if (!lv_scr_act()) {
    ESP_LOGE(TAG, "No active screen, display not ready");
    return ESP_ERR_INVALID_STATE;
  }

  if (esp_lv_adapter_lock(-1) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to take LVGL lock");
    return ESP_FAIL;
  }

  lv_obj_t* scr = lv_scr_act();

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Title
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Live Data");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(title, &notosans_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // Price
  label_price_ = lv_label_create(scr);
  lv_label_set_text(label_price_, "Price: -- kr");
  lv_obj_set_style_text_color(label_price_, lv_color_hex(0xFFD166), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_price_, &notosans_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_price_, LV_ALIGN_CENTER, 0, -60);

  // Temperature
  label_temp_ = lv_label_create(scr);
  lv_label_set_text(label_temp_, "Temp: -- C");
  lv_obj_set_style_text_color(label_temp_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_temp_, &notosans_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_temp_, LV_ALIGN_CENTER, 0, -10);

  // Humidity
  label_humidity_ = lv_label_create(scr);
  lv_label_set_text(label_humidity_, "Humidity: -- %");
  lv_obj_set_style_text_color(label_humidity_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_humidity_, &notosans_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_humidity_, LV_ALIGN_CENTER, 0, 25);

  // Pressure
  label_pressure_ = lv_label_create(scr);
  lv_label_set_text(label_pressure_, "Pressure: -- hPa");
  lv_obj_set_style_text_color(label_pressure_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_pressure_, &notosans_14, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_pressure_, LV_ALIGN_CENTER, 0, 60);

  // Show initial queue values immediately, if available
  if (DataQueue::get(&Qd_, 0) == 0) {
    update(Qd_);
  }

  lv_obj_invalidate(scr);
  esp_lv_adapter_refresh_now(display_.lv_disp_get());
  esp_lv_adapter_unlock();

  ESP_LOGI(TAG, "Main page initialized");
  return ESP_OK;
}

void Ui::update(const DataQueue::Data& Qd)
{
  char buf[48];

  if (esp_lv_adapter_lock(-1) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to take LVGL lock");
    return;
  }

  if (label_price_) {
    lv_snprintf(buf, sizeof(buf), "Price: %.2f kr", Qd.leop.energy_plan.price[0]);
    lv_label_set_text(label_price_, buf);
  }

  if (label_temp_) {
    lv_snprintf(buf, sizeof(buf), "Temp: %.1f C", Qd.sensor.temperature);
    lv_label_set_text(label_temp_, buf);
  }

  if (label_humidity_) {
    lv_snprintf(buf, sizeof(buf), "Humidity: %.1f %%", Qd.sensor.humidity);
    lv_label_set_text(label_humidity_, buf);
  }

  if (label_pressure_) {
    lv_snprintf(buf, sizeof(buf), "Pressure: %.1f hPa", Qd.sensor.pressure);
    lv_label_set_text(label_pressure_, buf);
  }

  esp_lv_adapter_refresh_now(display_.lv_disp_get());
  esp_lv_adapter_unlock();
}

esp_err_t Ui::init()
{
    return initialize_display(display_);
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

      // ESP_LOGI(TAG, "UI data updated! price: %f temp: %f, humidity: %f, press: %f.", 
      //   This->Qd.leop.energy_plan.price[0],
      //   This->Qd_.sensor.temperature,
      //   This->Qd_.sensor.pressure,
      //   This->Qd_.sensor.humidity
      // );
    }
    else
      ESP_LOGW(TAG, "Didn't update queue data");

    ui_buffer.refresh_count++;

    ESP_LOGI(TAG, "UI buffer update frame %lu", static_cast<unsigned long>(ui_buffer.refresh_count));

  }
}
