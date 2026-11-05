#include "sensors_update.hpp"

SensorsUpdate read_sensor_update(ISensor& sensor)
{
    SensorsUpdate update;
    update.read_result = sensor.read(update.readings);
    return update;
}

bool SensorsUpdate::should_publish(bool timer_signaled) const
{
    return timer_signaled && read_result == ESP_OK;
}