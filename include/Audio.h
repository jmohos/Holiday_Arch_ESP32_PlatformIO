#pragma once
#include <Arduino.h>


// Starts the audio task that drives the mp3 player hardware.
//
// Returns: true on success, false on failure.
bool audio_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);