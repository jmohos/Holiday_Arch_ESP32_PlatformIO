#include <Arduino.h>
#include "CommandQueues.h"
#include "IoSync.h"
#include "Logging.h"
#include "Motor.h"
#include "Pins.h"

#if defined(ESP32C3_BOARD)
// Servo management library.
//  From Dlloydev/ESP32-ESP32S2-AnalogWrite
//  Example: https://github.com/Dlloydev/ESP32-ESP32S2-AnalogWrite/blob/main/examples
#include <Servo.h>

// Servo manager
Servo myservo = Servo(); /* Supports 0-180 degrees */

#elif defined(ESP32C6_BOARD)
// Servo management header class.
#include "SmoothServos.h"

SmoothServos servos(2);
#endif

static bool g_playing = false;
static uint8_t g_animIndex = static_cast<uint8_t>(MotorAnim::HOME);
static bool g_animReset = true; // set true on animation change

static float HOME_ANGLE = 90.0;
static float LEFT_ANGLE = 0.0;
static float RIGHT_ANGLE = 180.0;

static uint16_t g_targetFps = 50;

// ---------- Helpers ----------
static inline uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

// Jiggle animation pattern
static constexpr uint16_t NUM_JIGGLE_STEPS = 8;
static constexpr float jiggle_positions[NUM_JIGGLE_STEPS] = {90.0, 100.0, 110.0, 100.0, 90.0, 80.0, 70.0, 80.0};
static constexpr uint32_t jiggle_delays[NUM_JIGGLE_STEPS] = {50, 50, 50, 50, 50, 50, 50, 50};

// Hammer animation pattern
static constexpr uint16_t NUM_HAMMER_STEPS = 8;
static constexpr float hammer_positions[NUM_HAMMER_STEPS] = {180.0, 0.0, 120.0, 40.0};
static constexpr uint32_t hammer_delays[NUM_HAMMER_STEPS] = {500, 500, 500, 500};


void config_servos() {
#if defined(ESP32C6_BOARD)
  ServoConfig cfgs[2];
  cfgs[0] = {
    .pin = SERVO_1_PIN,
    .minUs = 600,
    .maxUs = 2400,
    .invert = false,
    .maxDegPerSec = 120,       // limit speed to tame spikes
    .maxAccelDegPerSec2 = 400, // optional accel shaping
    .deadbandDeg = 0.5f
  };
  cfgs[1] = {
    .pin = SERVO_2_PIN,
    .minUs = 600,
    .maxUs = 2400,
    .invert = false,
    .maxDegPerSec = 90,
    .maxAccelDegPerSec2 = 300,
    .deadbandDeg = 0.5f
  };

  if (!servos.begin(cfgs, 2, /*periodHz=*/50)) {
    Serial.println("Servo init failed (pin or LEDC channel issue).");
    while (true) delay(1000);
  }

  // Start positions
  servos.snapTo(0, 90);
  servos.snapTo(1, 90);

#endif
}


// Force the motors to their idle position immediately.
void motor_idle()
{
#if defined(ESP32C3_BOARD)  
  myservo.write(SERVO_1_PIN, HOME_ANGLE);
  myservo.write(SERVO_1_PIN, HOME_ANGLE);
#elif defined(ESP32C6_BOARD)
  servos.snapTo(0, HOME_ANGLE);
  servos.snapTo(1, HOME_ANGLE);
#endif
}

// Motor animation routine to home
static void anim_motor_home(bool reset)
{
  static uint32_t t0 = 0;
  static double speed = 90; /* deg/sec */
  static double ke = 0.0;   /* 0 = linear ramping/easing */

  if (reset)
  {
    t0 = now_ms();
    motor_idle();
  }

  const uint32_t t = now_ms() - t0;

  // TODO: Ramp positions to home position
#if defined(ESP32C3_BOARD)  
  myservo.write(SERVO_1_PIN, HOME_ANGLE, speed, ke);
  myservo.write(SERVO_2_PIN, HOME_ANGLE, speed, ke);
#elif defined(ESP32C6_BOARD)
  servos.snapTo(0, HOME_ANGLE);
  servos.snapTo(1, HOME_ANGLE);  
#endif
}

// Motor animation routine to jiggle
static void anim_motor_jiggle(bool reset)
{
  static uint32_t last_update_time = 0;
  static uint8_t index = 0;
  static bool in_delay = false;

  static double speed = 180; /* deg/sec */
  static double ke = 0.1;    /* Slight damping */

  if (reset)
  {
    index = 0;
    last_update_time = 0;
    in_delay = false;
  }

  if (index >= NUM_JIGGLE_STEPS)
  {
    index = 0;
  }

  // Update the servos often to support the ramping in between changes.
#if defined(ESP32C3_BOARD)  
  myservo.write(SERVO_1_PIN, jiggle_positions[index], speed, ke);
  myservo.write(SERVO_2_PIN, jiggle_positions[index], speed, ke);
#elif defined(ESP32C6_BOARD)
  servos.snapTo(0, jiggle_positions[index]);
  servos.snapTo(1, jiggle_positions[index]);    
#endif

  if (!in_delay)
  {
    last_update_time = now_ms();
    in_delay = true;
  }
  else
  {
    uint32_t delta_msec = now_ms() - last_update_time;
    if (delta_msec >= jiggle_delays[index])
    {
      index++;
      in_delay = false;
    }
  }
}

// Motor animation routine to hammer
static void anim_motor_hammer(bool reset)
{
  static uint32_t last_update_time = 0;
  static uint8_t index = 0;
  static bool in_delay = false;

  static double speed = 180; /* deg/sec */
  static double ke = 0.0;    /* No damping */

  if (reset)
  {
    index = 0;
    last_update_time = 0;
    in_delay = false;
  }

  if (index >= NUM_JIGGLE_STEPS)
  {
    index = 0;
  }

  // Update the servos often to support the ramping in between changes.
#if defined(ESP32C3_BOARD)  
  myservo.write(SERVO_1_PIN, hammer_positions[index], speed, ke);
  myservo.write(SERVO_2_PIN, hammer_positions[index], speed, ke);
#elif defined(ESP32C6_BOARD)
  servos.snapTo(0, hammer_positions[index]);
  servos.snapTo(1, hammer_positions[index]);   
#endif

  if (!in_delay)
  {
    last_update_time = now_ms();
    in_delay = true;
  }
  else
  {
    uint32_t delta_msec = now_ms() - last_update_time;
    if (delta_msec >= hammer_delays[index])
    {
      index++;
      in_delay = false;
    }
  }
}

// Dispatch table for all motor animation routines.
typedef void (*AnimFn)(bool reset);
static AnimFn kAnims[NUM_MOTOR_ANIMATIONS] = {anim_motor_home, anim_motor_jiggle, anim_motor_hammer};

static void MotorTask(void *)
{
  MotorCmdQueueMsg msg{};
  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / g_targetFps);
  TickType_t lastWake = xTaskGetTickCount();

  config_servos();

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
            // Start a new motor animation.
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

#if defined(ESP32C3_BOARD)  
        myservo.write(SERVO_1_PIN, HOME_ANGLE);
        myservo.write(SERVO_1_PIN, HOME_ANGLE);
#elif defined(ESP32C6_BOARD)
        servos.snapTo(0, HOME_ANGLE);
        servos.snapTo(1, HOME_ANGLE);           
#endif
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

    // Frame pacing
    vTaskDelayUntil(&lastWake, frameTicks);
  }
}



bool motor_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core)
{
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      MotorTask, "MotorTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}