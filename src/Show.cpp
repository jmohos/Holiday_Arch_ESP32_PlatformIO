#include <Arduino.h>
#include "CommandQueues.h"
#include "Show.h"

#include "IoSync.h"
#include "Logging.h"


static void ShowTask(void*) {
  uint16_t counter = 0;
  ShowInputMsg in_msg{};

  for (;;)
  {

    if (xQueueReceive(queueBus.showInput, &in_msg, 0) == pdPASS) {
      io_printf("Received incoming command: %d, param: %d\n", in_msg.cmd, in_msg.param);
    }

    //ESP_LOGD(TAG_SHOW, "[core %d] doing periodic work...", core());
    //Serial.println("Show tick...");
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


bool show_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ShowTask, "ShowTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}