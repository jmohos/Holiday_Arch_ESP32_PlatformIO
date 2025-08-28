#pragma once
#include <Arduino.h>


// Motor animation show indices.
enum class MotorAnim : uint8_t { Still = 0, Jiggle = 1, Hammer = 2, Count };
static constexpr uint8_t NUM_MOTOR_ANIMATIONS = static_cast<uint8_t>(MotorAnim::Count);


// Starts the motor task that drives the motor hardware.
//
// Returns: true on success, false on failure.
bool motor_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);
