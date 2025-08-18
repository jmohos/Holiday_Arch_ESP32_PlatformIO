#pragma once

// Include ESP32 logging support with the following headers:
// Provides support for: LOGI, LOGW, LOGE, LOGD
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "esp_log.h"
}
