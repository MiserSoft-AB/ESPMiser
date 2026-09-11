#pragma once

#include <freertos/FreeRTOS.h>

namespace task_priorities
{

    constexpr UBaseType_t NETWORK = 5;

    constexpr UBaseType_t SENSOR = 5;

    constexpr UBaseType_t DISPLAY = 3;

    constexpr UBaseType_t MONITOR = 1;


    static_assert(
        NETWORK == SENSOR,
        "NETWORK and SENSOR must have the same/equal priority to one another"
    );
    static_assert(
        NETWORK > DISPLAY,
        "NETWORK must have a higher priority than DISPLAY"
    );
    static_assert(
        SENSOR > DISPLAY,
        "SENSOR must have a higher priority than DISPLAY"
    );
    static_assert(
        DISPLAY > MONITOR,
        "DISPLAY must have a higher priority than MONITOR"
    );

}