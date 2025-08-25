#include <Arduino.h>
#include "CommandQueues.h"
#include "Motor.h"

#include "IoSync.h"
#include "Logging.h"


static void MotorTask(void*) {
  MotorCmdQueueMsg in_msg{};

  for (;;)
  {

    if (xQueueReceive(queueBus.motorCmdQueueHandle, &in_msg, 0) == pdPASS) {
      io_printf("Received incoming motor command: %d, param: %d\n", in_msg.cmd, in_msg.param);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool motor_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      MotorTask, "MotorTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}