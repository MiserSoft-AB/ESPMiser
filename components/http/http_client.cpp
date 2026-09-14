#include "http_client.hpp"
#include "esp_err.h"

static const char* TAG = "http_client";

EspHttpClient::EspHttpClient(const Config& Cfg) 
  : config_(Cfg)
{
  // Could potentially run client_init here directly
  // but keeping esp_http_client resource allocations separate might be safer
  ESP_LOGI(TAG, "EspHttpClient constructed");
}

EspHttpClient::~EspHttpClient()
{
  client_clean();
  ESP_LOGI(TAG, "EspHttpClient destructed");
}

esp_err_t EspHttpClient::client_init()
{
  if (client_ != nullptr) 
  {
    ESP_LOGW(TAG, "Connection already open.");
    return ESP_ERR_NOT_ALLOWED;
  }

  // Create esp_http_client_config_t from our Config struct
  esp_http_client_config_t http_cfg = {};
  // TODO: URL-encode
  http_cfg.url = config_.def_url;
  http_cfg.timeout_ms = config_.timeout_ms;
  http_cfg.disable_auto_redirect = true;

  // http_cfg.is_async = config_.non_blocking;

  // Attach event handler cb for capturing data
  http_cfg.user_data = this;
  http_cfg.event_handler = EspHttpClient::event_handler;

  client_ = esp_http_client_init(&http_cfg);
  if (client_ == nullptr) 
  {
    ESP_LOGE(TAG, "Failed to initialize esp_http_client!");
    return ESP_FAIL;
  }

  return ESP_OK;
}

void EspHttpClient::client_reset()
{
  if (!client_) return;
  // Clear headers
  esp_http_client_set_header(client_, "Content-Type", nullptr);
  esp_http_client_set_header(client_, "Content-Length", nullptr);
  // Clear body
  response_body_.clear();
}

void EspHttpClient::client_clean()
{
  if (client_) 
  {
    esp_http_client_cleanup(client_);
    client_ = nullptr;
  }
}

int EspHttpClient::check_status_code()
{
  if (!client_) return -1;
  return esp_http_client_get_status_code(client_);
}

esp_err_t EspHttpClient::get(const std::string& full_path)
{
  esp_err_t res = ESP_OK;

  // Open (init it should be called instead? Doesn't 
  if (!client_)
  {
    res = client_init();
    if (res != 0)
    {
      ESP_LOGE(TAG, "Couldn't init esp_http");
      return res;
    }
  }

  // Must reset headers/response_body if previous request's been made
  client_reset();

  ESP_LOGI(TAG, "Setting URL: %s", full_path.c_str());

  // Set the URL
  res = esp_http_client_set_url(client_, full_path.c_str());
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_http_client_set_url failed");
    return res;
  }

  // Set method to GET
  res = esp_http_client_set_method(client_, HTTP_METHOD_GET);
  if (res != ESP_OK) {
    ESP_LOGE(TAG, "esp_http_client_set_method failed");
    return res;
  }

  // Perform the request specified by client config
  res = esp_http_client_perform(client_);
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_http_client_perform failed");
    return res;
  }

  return res;
}

esp_err_t EspHttpClient::post(const std::string& full_path, const std::string& payload) 
{
  esp_err_t res = ESP_OK;

  // Try to open connection
  if (!client_)
  {
    res = client_init();
    if (res != 0)
    {
      ESP_LOGE(TAG, "Couldn't init esp_http");
      return res;
    }
  }

  // Must reset headers/response_body if previous request's been made
  client_reset();

  // Set the URL
  res = esp_http_client_set_url(client_, full_path.c_str());
  if (res != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_http_client_set_url failed");
    return res;
  }

  // Set method to POST
  res = esp_http_client_set_method(client_, HTTP_METHOD_POST);
  if (res != ESP_OK) 
  {
    ESP_LOGE(TAG, "esp_http_client_set_method failed");
    return res;
  }

  // Set content length and body
  size_t payload_len = payload.length();
  res = esp_http_client_set_header(client_, "Content-Length", 
    std::to_string(payload_len).c_str());
  if (res != ESP_OK) 
  {
    ESP_LOGE(TAG, "esp_http_client_set_header Content-Length failed");
    return res;
  }

  // Default to application/json
  // TODO: Make this configurable
  res = esp_http_client_set_header(client_, "Content-Type", "application/json");
  if (res != ESP_OK) 
  {
    ESP_LOGE(TAG, "esp_http_client_set_header Content-Type failed");
    return res;
  }

  // Write payload
  if (payload_len > 0) 
  {
    int written = esp_http_client_write(client_, payload.c_str(), payload_len);
    if (written < 0 || ((size_t)written != payload_len)) {
      ESP_LOGE(TAG, "esp_http_client_write failed: wrote %d of %zu", written, payload_len);
      return ESP_FAIL;
    }
  }

  // Perform the request
  res = esp_http_client_perform(client_);
  if (res != ESP_OK) 
  {
    ESP_LOGE(TAG, "esp_http_client_perform failed");
    return res;
  }

  return res;
}

esp_err_t EspHttpClient::read_body(std::string &out_body)
{
  if (!client_) 
  {
    ESP_LOGE(TAG, "Client not open");
    return ESP_ERR_NOT_ALLOWED;
  }

  // In blocking mode with event handler capturing data, the body is already in response_body_
  out_body = response_body_;
  return ESP_OK;
}

esp_err_t EspHttpClient::event_handler(esp_http_client_event_t* event)
{
  // Retrieve 'this' pointer from user_data
  EspHttpClient* Client = static_cast<EspHttpClient*>(event->user_data);
  
  if (!Client) 
  {
    ESP_LOGE("http_client", "event_handler: user_data is null!");
    return ESP_ERR_INVALID_ARG;
  }

  switch (event->event_id) 
  {
    case HTTP_EVENT_ERROR:
      ESP_LOGE("http_client", "HTTP_EVENT_ERROR");
      break;
      
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGI("http_client", "HTTP_EVENT_ON_CONNECTED");
      break;
      
    case HTTP_EVENT_HEADERS_SENT:
      ESP_LOGI("http_client", "HTTP_EVENT_HEADERS_SENT");
      break;
      
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGI("http_client", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", 
        event->header_key, event->header_value);
      break;
      
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI("http_client", "HTTP_EVENT_ON_DATA, len=%d", event->data_len);

      // Append data chunk to response body
      if (event->data_len > 0 && event->data != nullptr) 
      {
        // Avoid writing more data than we allow
        if (event->data_len < Client->config_.response_body_max_len)
        {
          Client->response_body_.append(
            static_cast<const char*>(event->data), 
            event->data_len
          );
        } else {
          Client->response_body_.append(
            static_cast<const char*>(event->data), 
            Client->config_.response_body_max_len
          );
        }
      }
      break;
      
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGI("http_client", "HTTP_EVENT_ON_FINISH");
      break;
      
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGI("http_client", "HTTP_EVENT_DISCONNECTED");
      break;
      
    case HTTP_EVENT_REDIRECT:
      ESP_LOGI("http_client", "HTTP_EVENT_REDIRECT");
      break;
  }

  return ESP_OK;
}

