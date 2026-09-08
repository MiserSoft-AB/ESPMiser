#include "display.hpp"

#include <cstdint> //I don't remember if we should avoid using 'std's

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace 
{
    constexpr char TAG[] = "display";
    constexpr uint32_t DISPLAY_REFRESH_MS = 2000;

    struct DisplayBuffer
    {
        uint32_t refresh_count;
    };

    DisplayBuffer display_buffer{};
} // Namespace

void display_task(void* parameter)
{
    (void)parameter;

    ESP_LOGI(TAG, "Display task started");

    while (true)
    {
        display_buffer.refresh_count++;

        ESP_LOGI(
            TAG,
            "Display buffer updated, frame %lu", 
            static_cast<unsigned long>(display_buffer.refresh_count)
        );

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}