#pragma once
#include <Arduino.h>


// Starts the proximity detector task.
//
// Returns: true on success, false on failure.
bool prox_detect_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);