#include "tasks/tasks.hpp"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void sensor_task(void* parameter)
{
    (void)parameter;

    float temperature = 22.0f;
    float humidity = 45.0f;
    float pressure = 1013.25f;

    while (true)
    {
        ESP_LOGI(
            "sensor_task",
            "Simulated reading: %.1f C, %.1f %% RH, %.1f hPa",
            temperature,
            humidity,
            pressure
        );

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}