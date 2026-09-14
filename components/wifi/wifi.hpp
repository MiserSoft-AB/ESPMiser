#ifndef _WIFI_HPP_
#define _WIFI_HPP_

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_wifi_types_generic.h>
#include <string>

#include <sdkconfig.h>

class WifiHandler {
    private:
        const wifi_init_config_t init_config;

        bool wifi_ready;
        std::string IP;

        static void on_wifi_init_finished(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data);
        static void on_wifi_connected(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data);
        static void on_assigned_ip(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data);

    public:
        WifiHandler(const wifi_init_config_t init_config) : init_config(init_config), wifi_ready(false), IP("None")
        {

        }

        esp_err_t wifi_init();
        esp_err_t wifi_connect();
        std::string get_ip();

};


#endif