#include <Arduino.h>
#include <elapsedMillis.h>
#include "IoSync.h"
#include "Console.h"
#include "CommandExec.h"
#include "Faults.h"
#include "Logging.h"
#include "SettingsStore.h"
#include "Show.h"

static const char* TAG_BOOT = "BOOT";

// Persistent storage handle
Persist::SettingsStore gCfg;


// Bootup initialization.
void setup() {
  BaseType_t ok;

  Serial.begin(115200);
  delay(2000);  // Give serial interface time to startup before using.

  // One-time init: install locked vprintf + create I/O mutex so logs & prompts serialize
  io_sync_init();

  // Note, normally we would apply an esp_log_level_set() call to adjust the
  // logging output.  The Arduino ESP logging interface is a simmplified version of the
  // IDF logger.  There is only one filter level and it is set in the platformio.ini
  // file in the -DCORE_DEBUG_LEVEL=5 definition.

  ESP_LOGI(TAG_BOOT, "System startup, about to start tasks...");
  
  // Restore the persistent memory parameters
  if (!gCfg.begin()) {
    // Failed to restore settings
    ESP_LOGE(TAG_BOOT, "Failed to restore config!");
    FAULT_SET(FAULT_CONFIG_RESTORE_FAULT);
  } else {
    ESP_LOGI(TAG_BOOT, "Config restored.");
  }

  // Start the console interface task.
  if (!console_start(
    8, /* Queue Length */
    2, /* Priority */
    4096, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_CONSOLE_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start console task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Console task started.");
  }

  // Start the remote command executor task.
  if (!command_exec_start(
    2, /* Priority */
    4096, /* Stack Bytes */
    1, /* Core */
    &gCfg /* Configuration */ ))
  {
    FAULT_SET(FAULT_CMD_EXEC_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start command executor task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Command executor started.");
  }

    // Start the remote command executor task.
  if (!show_start(
    configMAX_PRIORITIES - 1, /* Priority */
    8192, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_SHOW_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start show task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Show task started.");
  }
}

// Background task
void loop() {
  // Keep Arduino loop empty so tasks do the work.
  vTaskDelay(pdMS_TO_TICKS(1000));

  ESP_LOGD(TAG_BACKG, "Background...");
}

