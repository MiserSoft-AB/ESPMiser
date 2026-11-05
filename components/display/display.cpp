#include "display.hpp"

#include <cstdint> //I don't remember if we should avoid using 'std's
                   // Nah dude, yolo

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

namespace 
{
    constexpr char TAG[] = "display";
    constexpr uint32_t DISPLAY_REFRESH_MS = 2000;

    struct DisplayBuffer
    {
        uint32_t refresh_count;
    };

    DisplayBuffer display_buffer{};

    TaskHandle_t display_task_handle = nullptr;

    void display_timer_callback(TimerHandle_t timer)
    {
        (void)timer;

        if (display_task_handle != nullptr)
        {
            xTaskNotifyGive(display_task_handle);
        }
    }
} // Namespace

void display_task(void* parameter)
{
    (void)parameter;

    display_task_handle = xTaskGetCurrentTaskHandle();

    static StaticTimer_t timer_storage;

    TimerHandle_t refresh_timer = xTimerCreateStatic
    (
        "display_refresh",
        pdMS_TO_TICKS(DISPLAY_REFRESH_MS),
        pdTRUE,
        nullptr,
        display_timer_callback,
        &timer_storage
    );

    if (refresh_timer == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create display timer");
        display_task_handle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    if (xTimerStart(refresh_timer, pdMS_TO_TICKS(100)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to start display timer");
        display_task_handle = nullptr;
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(TAG, "Display timer start requested");

    while (true)
    {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000)) == 0) 
        {
            ESP_LOGW(TAG, "No display timer notification within 3 seconds");
            continue;
        }

        display_buffer.refresh_count++;

        ESP_LOGI(TAG, "Display buffer update frame %lu", static_cast<unsigned long>(display_buffer.refresh_count));
    }
}
