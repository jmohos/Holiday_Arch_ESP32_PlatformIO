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
Servo myservo = Servo(); /* Supports 0-180 degrees */

static bool g_playing = false;
static uint8_t g_animIndex = static_cast<uint8_t>(MotorAnim::HOME);
static bool g_animReset = true; // set true on animation change

static float HOME_ANGLE = 90.0;
static float LEFT_ANGLE = 0.0;
static float RIGHT_ANGLE = 180.0;

// ---------- Helpers ----------
static inline uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

// Force the motors to their idle position immediately.
void motor_idle()
{
  myservo.write(SERVO_1_PIN, HOME_ANGLE);
  myservo.write(SERVO_1_PIN, HOME_ANGLE);
}

// Motor animation routine to home
static void anim_motor_home(bool reset)
{
  static uint32_t t0 = 0;
  static double speed = 20; /* deg/sec */
  static double ke = 1.0;

  if (reset)
  {
    t0 = now_ms();
    motor_idle();
  }

  const uint32_t t = now_ms() - t0;

  // TODO: Ramp positions to home position
  myservo.write(SERVO_1_PIN, HOME_ANGLE, speed, ke);
  myservo.write(SERVO_2_PIN, HOME_ANGLE, speed, ke);
}

// Motor animation routine to jiggle
static void anim_motor_jiggle(bool reset)
{
  static uint32_t t0 = 0;
  static double speed = 10; /* deg/sec */
  static double ke = 1.0;

  if (reset)
  {
    t0 = now_ms();
    motor_idle();
  }

  const uint32_t t = now_ms() - t0;

  // TODO: Execute jiggle pattern
  myservo.write(SERVO_1_PIN, LEFT_ANGLE, speed, ke);
  myservo.write(SERVO_2_PIN, RIGHT_ANGLE, speed, ke);  
}

// Motor animation routine to hammer
static void anim_motor_hammer(bool reset)
{
  static uint32_t t0 = 0;
  static double speed = 180; /* deg/sec */
  static double ke = 1.0;

  if (reset)
  {
    t0 = now_ms();
    motor_idle();
  }

  const uint32_t t = now_ms() - t0;

  // TODO: Execute hammer pattern
  myservo.write(SERVO_1_PIN, RIGHT_ANGLE, speed, ke);
  myservo.write(SERVO_2_PIN, LEFT_ANGLE, speed, ke);  
}

// Dispatch table for all motor animation routines.
typedef void (*AnimFn)(bool reset);
static AnimFn kAnims[NUM_MOTOR_ANIMATIONS] = {anim_motor_home, anim_motor_jiggle, anim_motor_hammer};

static void MotorTask(void *)
{
  MotorCmdQueueMsg msg{};

  // Setup the timer and PWM outputs for hobby servo compliant patterns.
  motor_idle();

  io_printf("MOTOR - Seeded default PWM values.\n");

  for (;;)
  {

    if (xQueueReceive(queueBus.motorCmdQueueHandle, &msg, 0) == pdPASS)
    {
      io_printf("Received incoming motor command: %d, param: %d\n", msg.cmd, msg.param);

      // Process the incoming command.
      switch (msg.cmd)
      {
      case MotorQueueCmd::Play:
      {
        if (msg.param >= NUM_MOTOR_ANIMATIONS)
        {
          io_printf("Invalid motor animation index: %d\n", msg.param);
        }
        else
        {
          if (!g_playing || msg.param != g_animIndex)
          {
            // Start a new light animation.
            g_animIndex = msg.param;
            g_animReset = true;
          }
          g_playing = true;
        }
        break;
      }

      case MotorQueueCmd::Stop:
      {
        g_playing = false;
        g_animReset = true; // ensure clean start next time

        myservo.write(SERVO_1_PIN, HOME_ANGLE);
        myservo.write(SERVO_1_PIN, HOME_ANGLE);
        break;
      }

      default:
        // Ignore other commands for now
        break;
      }
    }

    // Execute the last requested motor animation if playing.
    if (g_playing)
    {
      if (g_animIndex < NUM_MOTOR_ANIMATIONS)
      {
        // Call the selected animation function.
        kAnims[g_animIndex](g_animReset);
        g_animReset = false;
      }
      else
      {
        // Safety: if index invalid, just clear
        motor_idle();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool motor_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core)
{
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      MotorTask, "MotorTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}