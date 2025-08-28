#pragma once
#include <Arduino.h>


// Motor animation show indices.
enum class MotorAnim : uint8_t { 
  HOME = 0,   // Halt at home
  JIGGLE = 1, //
  HAMMER = 2, //
  COUNT       // Must be last 
};
static constexpr uint8_t NUM_MOTOR_ANIMATIONS = static_cast<uint8_t>(MotorAnim::COUNT);


// Starts the motor task that drives the motor hardware.
//
// Returns: true on success, false on failure.
bool motor_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);
