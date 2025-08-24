#pragma once
#include <Arduino.h>


// Starts the motor task that drives the motor hardware.
//
// Returns: true on success, false on failure.
bool motor_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);