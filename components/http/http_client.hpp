#pragma once // in case included multiple places

#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include <esp_http_client.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string>


// WARNING: Not thread-safe as-is, need careful handling in tasks


class EspHttpClient 
{
public:
  struct Config {
    // const char* url = ""; // No need to set this, it will be overwritten in get/post
    // const char* cert_pem; //for https
    int timeout_ms = 5000;
    int event_handler_mask = HTTP_EVENT_ERROR | HTTP_EVENT_ON_DATA;
    size_t response_body_max_len = 8192;
    const char* def_url = "http://httpforever.com";
    
    // TODO: (maybe) add flow for async functionality
    // blocking flow right now calling perform() and using event_handler
    // nonblocking should manually use esp_http_client_fetch_headers(), esp_http_client_read() 
    // from a task loop, and rely on events differently.
    // bool non_blocking = false;
  };

  // Only inits class, http_client opened on get()/post() using client_init()
  EspHttpClient(const Config& cfg);

  // Also runs client_clean()
  ~EspHttpClient();

  // No copy, no move
  EspHttpClient(const EspHttpClient&) = delete;
  EspHttpClient& operator=(const EspHttpClient&) = delete;
  EspHttpClient(EspHttpClient&&) noexcept = delete;
  EspHttpClient& operator=(EspHttpClient&&) noexcept = delete;

  // Perform a GET request. Streams response to event_handler
  esp_err_t get(const std::string& full_path);

  // Perform a POST request with JSON or form data.
  esp_err_t post(const std::string& full_path, const std::string &payload = "");

  // Not sure if we should keep this, if stuff should be done via event_handler only
  esp_err_t read_body(std::string &out_body);
  // const std::string get_response_body() const { return response_body_; }
  
  // Inits the esp_http_client with config values
  esp_err_t client_init();

  // Task for running simple fetch
  static void test_fetch_task (void* parameter);

  // Checks HTTP status code
  int check_status_code();

private:
  Config config_;
  esp_http_client_handle_t client_ = nullptr;

  std::string response_body_;

  void client_reset();
  void client_clean();

  // Helper to parse response code
  // void check_response_code();

  // Event handler callback 
  static esp_err_t event_handler(esp_http_client_event_t* event);
};

#endif // __HTTP_CLIENT_HPP__
