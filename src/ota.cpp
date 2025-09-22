#include <Arduino.h>
#include "Logging.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

// ===== Flags to coordinate with your other tasks =====
volatile bool gUpdating = false;


static void OTATask(void*) {
  // Configure OTA callbacks (optional but handy)
  // ArduinoOTA.setHostname("esp32c6-node");
  // ArduinoOTA.setPassword("MyOTAPass");

  ArduinoOTA
    .onStart([]() {
      gUpdating = true;
      // Optionally: stop peripherals, mute audio, disable PWM, etc.
      ESP_LOGI("OTA", "Start");
    })
    .onEnd([]() {
      ESP_LOGI("OTA", "End");
      // After OTA completes, device will reboot automatically.
      // No need to clear gUpdating here.
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      // Keep this lightweight
      ESP_LOGI("OTA", " %u%%\r",  (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("\n[OTA] Error %u\n", error);
      ESP_LOGI("OTA", " Error %u\n", error);
      gUpdating = false;  // allow app to resume if failed
    });

  ArduinoOTA.begin();
  ESP_LOGI("OTA", " Ready");

  // Service OTA in a loop
  for (;;) {
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(20));  // ~50 Hz is fine
  }
}


bool ota_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      OTATask, "OTATask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}