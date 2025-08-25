#include <Arduino.h>
#include "Audio.h"
#include "CommandQueues.h"
#include "IoSync.h"
#include "Console.h"
#include "CommandExec.h"
#include "Faults.h"
#include "Light.h"
#include "Logging.h"
#include "Motor.h"
#include "NetService.h"
#include "Protocol.h"
#include "ProxDetect.h"
#include "SettingsStore.h"
#include "Show.h"


static const char* TAG_BOOT = "BOOT";

Persist::SettingsStore settingsConfig;
NetService *networkService = nullptr;

// Network receive callback
void onPacket(const uint8_t *data, size_t len, const IPAddress &from, void *user)
{

  Proto::View v;
  if (!Proto::parse(data, len, v))
  {
    ESP_LOGI("NET", "[RX] Malformed Mcast packet from 0x%02X",
      v.hdr.src);
    return; // bad or too short
  }

  ESP_LOGI("NET", "[RX] Mcast Packet from 0x%02X...",
    v.hdr.src);

  if (!Proto::isForMe(settingsConfig.deviceId(), v.hdr.dst))
  {
    ESP_LOGI("NET", "[RX] Mcast Packet not for me.");
    return; // not for me
  }

  switch (v.hdr.cmd)
  {
    case Proto::CMD_PING:
    // No payload; maybe reply or blink an LED
    ESP_LOGI("NET", "[RX] PING from 0x%02X (%s)", 
      v.hdr.src, 
      from.toString().c_str());
    break;

  case Proto::CMD_CHANGE_MODE:
  {
    uint8_t mode;
    if (Proto::decodeChangeMode(v, mode))
    {
      ESP_LOGI("NET", "[RX] CHANGE_MODE -> %u (from 0x%02X)",
        mode,
        v.hdr.src);
      
      // TODO: signal your control task to apply 'mode'
    }
  }
  break;

  case Proto::CMD_TRIGGER_ANIM:
  {
    uint8_t animId;
    if (Proto::decodeTriggerAnim(v, animId))
    {
      ESP_LOGI("NET", "[RX] TRIGGER_ANIM -> %u (from 0x%02X)\n", animId, v.hdr.src);

      // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      // xQueueSendFromISR(animQueue, &animId, &xHigherPriorityTaskWoken);
    }
  }
  break;

  default:
    // Unknown command—safe to ignore for forward compatibility
    ESP_LOGW("NET", "[RX] Unknown cmd 0x%02X from 0x%02X, %uB payload\n",
      v.hdr.cmd, v.hdr.src, (unsigned)v.payload_len);                  
    break;
  }
}

bool send_ping() {
  uint8_t ping_packet[sizeof(Proto::Header)];
  size_t len=0;

  len = Proto::buildPing(ping_packet, sizeof(ping_packet), Proto::BROADCAST, settingsConfig.deviceId());
  if (len == 0) {
    ESP_LOGE("NET", "Failed to pack ping message!");
    return false;
  }  

  ESP_LOGI("NET", "Sending ping...");
  int sent = networkService->send(ping_packet, len);
  if (sent > 0) {
    return true;
  }

  ESP_LOGE("NET", "Ping send failed!");
  return false;
}

// Bootup initialization.
void setup() {
  BaseType_t ok;

  Serial.begin(115200);
  delay(2000);  // Give serial interface time to startup before using.

  // One-time init: install locked vprintf + create I/O mutex so logs & prompts serialize
  io_sync_init();

  // Create all of the queues between tasks.
  if (!QueueBus_Init(queueBus)) {
    io_printf("QueueBus init failed!\n");
    abort();
  }

  // Note, normally we would apply an esp_log_level_set() call to adjust the
  // logging output.  The Arduino ESP logging interface is a simmplified version of the
  // IDF logger.  There is only one filter level and it is set in the platformio.ini
  // file in the -DCORE_DEBUG_LEVEL=5 definition.

  ESP_LOGI(TAG_BOOT, "System startup...\n");
  
  // Restore the persistent memory parameters
  if (!settingsConfig.begin()) {
    // Failed to restore settings
    ESP_LOGE(TAG_BOOT, "Failed to restore config!");
    FAULT_SET(FAULT_CONFIG_RESTORE_FAULT);
  } else {
    ESP_LOGI(TAG_BOOT, "Config restored.");
  }

  // Create the network services manager
  //TODO: Use the settingsConfig for multicast IP and Port!!!!!
  networkService = new NetService(
      settingsConfig.ssid(),
      settingsConfig.password(),
      IPAddress(239, 255, 0, 1), /* Multicast address */
      49400, /* Multicast port */
      1); /* TTL */

  // Map a callback for network incoming message processing.
  networkService->onPacket(onPacket);

  // Start the networking task on Core 0
  ok = xTaskCreatePinnedToCore(
    NetService::task,
    "net",
    8192, /* Wifi needs larger stack */
    networkService,
    5 /*prio*/,
    nullptr,
    0 /*core0*/);
  if (ok == pdPASS) {
    ESP_LOGI(TAG_BOOT, "Network task started.");
  } else {
    FAULT_SET(FAULT_NETWORK_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start network task!");
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
    &settingsConfig, /* Configuration */
    networkService /* Net Manager */))
  {
    FAULT_SET(FAULT_CMD_EXEC_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start command executor task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Command executor started.");
  }

  // Start the audio playback task.
  if (!audio_start(
    configMAX_PRIORITIES - 1, /* Priority */
    4096, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_AUDIO_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start audio task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Audio task started.");
  }

  // Start the light animation task.
  if (!light_start(
    configMAX_PRIORITIES - 1, /* Priority */
    4096, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_LIGHT_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start light task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Light task started.");
  }

  // Start the motor animation task.
  if (!motor_start(
    configMAX_PRIORITIES - 1, /* Priority */
    4096, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_MOTOR_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start motor task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Motor task started.");
  }

  // Start the proximity detection task.
  if (!prox_detect_start(
    configMAX_PRIORITIES - 1, /* Priority */
    4096, /* Stack Bytes */
    1 /* Core */ ))
  {
    FAULT_SET(FAULT_PROX_DETECT_TASK_FAULT);
    ESP_LOGE(TAG_BOOT, "Failed to start proximity detection task!");
  } else {
    ESP_LOGI(TAG_BOOT, "Proximity detection task started.");
  }

  // Start the master show task.
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
  vTaskDelay(pdMS_TO_TICKS(10));

  //send_ping();

  //ESP_LOGD(TAG_BACKG, "Background...");
}

