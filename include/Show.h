#pragma once
#include <Arduino.h>


// Starts the main show task that drives all animated elements.
//
// Returns: true on success, false on failure.
bool show_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);