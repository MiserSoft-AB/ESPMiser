#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include <esp_http_client.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string>

// WARNING: Not thread-safe as-is, need careful handling in tasks (or just use in one task)

/*
Usage approach:
Caller constructs class and inits client_init()
Then provides a callback to request that is to be run when data is recieved
Either it can read it in chunks and process, or it can read it all into its own buffer
Do note that the client will be blocked until all is read
*/

// Callback function: (pointer to user context, pointer to data chunkkk, length of chunk)
using http_data_callback = void (*)(void* ctx, const char* data, size_t len);

class EspHttpClient 
{
public:
  struct Config {
    int timeout_ms = 5000;
    int event_handler_mask = HTTP_EVENT_ERROR | HTTP_EVENT_ON_DATA; // is this used?
    const char* def_url = "http://httpforever.com"; // only init, is overriden on request
    
    // TODO: (maybe) add flow for async functionality
    // blocking flow right now calling perform() and using event_handler
    // nonblocking should manually use esp_http_client_fetch_headers(), esp_http_client_read() 
    // from a task loop, and rely on events differently.
    // bool non_blocking = false;
  };

  // Only inits class, http_client opened on get()/post() using client_init()
  EspHttpClient(const Config& cfg);

  // Also runs client_clean() NOTE: AVOID, frees heap for response_body_
  ~EspHttpClient();

  // No copy, no move
  EspHttpClient(const EspHttpClient&) = delete;
  EspHttpClient& operator=(const EspHttpClient&) = delete;
  EspHttpClient(EspHttpClient&&) noexcept = delete;
  EspHttpClient& operator=(EspHttpClient&&) noexcept = delete;

  /* Pass URL, callback function for reading data, and user context */
  esp_err_t get(const char* url, http_data_callback on_data_recv, void* ctx);

  // // Perform a POST request with JSON or form data.
  // esp_err_t post(const std::string& full_path, const std::string &payload = "");
  
  // Inits the esp_http_client with config values
  esp_err_t client_init();

  // Checks HTTP status code
  int check_status_code();

private:
  Config config_;
  esp_http_client_handle_t client_ = nullptr;

  http_data_callback active_callback_ = nullptr;
  void*              active_context_ = nullptr;

  void client_reset();
  void client_clean();

  // Event handler callback 
  static esp_err_t event_handler(esp_http_client_event_t* event);
};

#endif // __HTTP_CLIENT_HPP__
