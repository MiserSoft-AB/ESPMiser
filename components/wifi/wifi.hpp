#ifndef _WIFI_HPP_
#define _WIFI_HPP_

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_wifi_types_generic.h>
#include <string>

#define SSID ""
#define PASSPHRASE ""

class WifiHandler {
    private:
    
        const wifi_init_config_t init_config;


    public:
        WifiHandler(const wifi_init_config_t init_config) :  init_config(init_config)
        {}

        esp_err_t wifi_init();
        esp_err_t wifi_connect();
        std::string get_ip();

};


#endif