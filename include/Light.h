#pragma once
#include <Arduino.h>

// Light animation show indices.
enum class LightAnim : uint8_t { 
  BLANK = 0,     // All lights off
  CANDYCANE = 0, //
  FLAMES = 1,    // Flames from both ends
  BOUNCE = 2,    //
  COUNT          // Must be last
};
static constexpr uint8_t NUM_LIGHT_ANIMATIONS = static_cast<uint8_t>(LightAnim::COUNT);


// Starts the light task that drives the addressable LED hardware.
//
// Returns: true on success, false on failure.
bool light_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);
