#include <Arduino.h>
#include "CommandQueues.h"
#include "Audio.h"

#include "IoSync.h"
#include "Logging.h"


static void AudioTask(void*) {
  AudioCmdQueueMsg in_msg{};

  for (;;)
  {

    if (xQueueReceive(queueBus.audioCmdQueueHandle, &in_msg, 0) == pdPASS) {
      io_printf("Received incoming audio command: %d, param: %d\n", in_msg.cmd, in_msg.param);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool audio_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      AudioTask, "AudioTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}