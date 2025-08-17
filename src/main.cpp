#include <Arduino.h>
#include <elapsedMillis.h>

// ===== Tasks =====
TaskHandle_t netTaskHandle = nullptr; // Core 0 task
TaskHandle_t showTaskHandle = nullptr; // Core 1 task
TaskHandle_t consoleTaskHandle = nullptr;


void show_task(void *)
{
  uint16_t counter = 0;

  for (;;)
  {
    Serial.println("Show tick...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}



void setup() {
  Serial.begin(115200);
  delay(500);  // Give serial interface time to startup before using.

  // Core 1: Main app tasks
  xTaskCreatePinnedToCore(
      show_task,
      "show",
      8192,
      nullptr,
      configMAX_PRIORITIES - 1 /*high prio*/, 
      &showTaskHandle, 
      1 /*core 1*/);
}

void loop() {
  // Keep Arduino loop empty so tasks do the work.
  vTaskDelay(pdMS_TO_TICKS(200));

  Serial.println("Main Loop...");
}

