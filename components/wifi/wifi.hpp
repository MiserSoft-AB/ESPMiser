#ifndef __WIFI_HPP__
#define __WIFI_HPP__

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_wifi_types_generic.h>
#include <string>

#include <sdkconfig.h>

class WifiHandler {
    private:
    
        const wifi_init_config_t init_config;
 
        /* --- Event handling --- */
        static void wifi_event_handler(void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data);

        EventGroupHandle_t event_group_;
        esp_event_handler_instance_t instance_any_id_;

        // Bits we flip when wifi has connected and IP been retrieved
        static constexpr uint32_t WIFI_CONNECTED_BIT = (1 << 0);
        static constexpr uint32_t WIFI_GOT_IP_BIT    = (1 << 1);

    public:
        WifiHandler(const wifi_init_config_t init_config) : init_config(init_config)
        {}

        esp_err_t wifi_init();
        esp_err_t wifi_wait_for_connect();
        std::string get_ip();

};


#endif
