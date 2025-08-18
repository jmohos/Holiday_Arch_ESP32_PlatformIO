#include <Arduino.h>
#include <elapsedMillis.h>
#include "IoSync.h"
#include "Console.h"
#include "CommandExec.h"

extern "C" {
  #include "esp_log.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
}

// ===== Tasks =====
TaskHandle_t netTaskHandle = nullptr; // Core 0 task
TaskHandle_t showTaskHandle = nullptr; // Core 1 task
TaskHandle_t consoleTaskHandle = nullptr;

// ===== Task Log Names =====
static const char* TAG_BOOT = "BOOT";
static const char* TAG_NET = "NET";
static const char* TAG_SHOW = "SHOW";
static const char* TAG_CONSOLE = "CON";
static const char* TAG_BACKG = "BKG";

// A small helper to show which core we're on
static inline int core() { return xPortGetCoreID(); }

void show_task(void *)
{
  uint16_t counter = 0;

  for (;;)
  {
    ESP_LOGD(TAG_SHOW, "[core %d] doing periodic work...", core());
    //Serial.println("Show tick...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}



void setup() {
  Serial.begin(115200);
  delay(500);  // Give serial interface time to startup before using.

  // One-time init: install locked vprintf + create I/O mutex so logs & prompts serialize
  io_sync_init();

  // Note, normally we would apply an esp_log_level_set() call to adjust the
  // logging output.  The Arduino ESP logging interface is a simmplified version of the
  // IDF logger.  There is only one filter level and it is set in the platformio.ini
  // file in the -DCORE_DEBUG_LEVEL=5 definition.

  ESP_LOGI(TAG_BOOT, "System startup, about to start tasks...");
  
  // Start the console reader (core 1) and the executor (core 1)
  if (!console_start(8, /*prio*/2, /*core*/1)) {
    ESP_LOGE(TAG_BOOT, "console_start failed");
  }
  if (!command_exec_start(/*core*/1, /*prio*/2, /*stack*/4096)) {
    ESP_LOGE(TAG_BOOT, "command_exec_start failed");
  }

  // Core 1: Main app tasks
  xTaskCreatePinnedToCore(
      show_task,
      "show",
      8192,
      nullptr,
      configMAX_PRIORITIES - 1 /*high prio*/, 
      &showTaskHandle, 
      1 /*core 1*/);
  
  ESP_LOGI(TAG_BOOT, "All tasks started, running system.");

}

void loop() {
  // Keep Arduino loop empty so tasks do the work.
  vTaskDelay(pdMS_TO_TICKS(1000));

  ESP_LOGD(TAG_BACKG, "[core %d] Background...", core());
}

