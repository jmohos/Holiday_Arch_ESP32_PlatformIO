#include <Arduino.h>
#include "Show.h"

#include "IoSync.h"
#include "Logging.h"


static void ShowTask(void*) {
  uint16_t counter = 0;

  for (;;)
  {
    ESP_LOGD(TAG_SHOW, "[core %d] doing periodic work...", core());
    //Serial.println("Show tick...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


bool show_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ShowTask, "ShowTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}