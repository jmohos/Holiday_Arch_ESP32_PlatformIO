#pragma once
#include <Arduino.h>


// Starts the light task that drives the addressable LED hardware.
//
// Returns: true on success, false on failure.
bool light_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);