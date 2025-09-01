#include <Arduino.h>
#include "CommandQueues.h"
#include "IoSync.h"
#include "Logging.h"
#include "Motor.h"
#include "Pins.h"

// Servo management library.
//  From Dlloydev/ESP32-ESP32S2-AnalogWrite
//  Example: https://github.com/Dlloydev/ESP32-ESP32S2-AnalogWrite/blob/main/examples
#include <Servo.h>

// Servo manager
Servo myservo = Servo();


static void MotorTask(void*) {
  MotorCmdQueueMsg in_msg{};

  // Setup the timer and PWM outputs for hobby servo compliant patterns.
  myservo.write(SERVO_1_PIN, 90);
  myservo.write(SERVO_2_PIN, 10);

  io_printf("MOTOR - Seeded default PWM values.\n");

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