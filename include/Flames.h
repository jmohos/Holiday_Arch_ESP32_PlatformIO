// Flames.h — header-only dual-arch Fire2012-style flames
//
// Usage:
//   #include "Flames.h"
//   static FlamesDual<NUM_LEDS, ARCH_R_START, ARCH_R_END, ARCH_L_START, ARCH_L_END> flames;
//   flames.update(reset);
//   memcpy(dst, flames.buffer(), flames.size()*sizeof(CRGB));
//
// Notes:
// - Implements two independent flame “columns” (right arch and left arch),
//   using the classic FastLED Fire2012 algorithm, then mirrors/places them
//   along your full strip layout defined by the template parameters.
// - Defaults for COOLING/SPARKING are set for outdoors/diffused strips but
//   you can tune them at runtime with setCooling()/setSparking().

#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <string.h>

template <
    int NUM_LEDS,
    int ARCH_R_START, int ARCH_R_END,
    int ARCH_L_START, int ARCH_L_END>
class FlamesDual
{
public:
  static_assert(ARCH_R_START <= ARCH_R_END, "Right arch indices invalid");
  static_assert(ARCH_L_START <= ARCH_L_END, "Left arch indices invalid");
  static_assert(ARCH_R_START >= 0 && ARCH_L_START >= 0, "Indices must be >= 0");
  static_assert(ARCH_R_END < NUM_LEDS && ARCH_L_END < NUM_LEDS, "Indices must be < NUM_LEDS");

  static constexpr int NUM_R = (ARCH_R_END - ARCH_R_START + 1);
  static constexpr int NUM_L = (ARCH_L_END - ARCH_L_START + 1);

  inline void update(bool reset = false)
  {
    if (reset)
    {
      memset(heat_, 0, sizeof(heat_));
      fill_solid(leds_, NUM_LEDS, CRGB::Black);
    }

    // --- Step 1: cool both halves ---
    for (int i = ARCH_R_START; i <= ARCH_R_END; ++i)
    {
      heat_[i] = qsub8(heat_[i], random8(0, ((cooling_ * 10) / NUM_R) + 2));
    }
    for (int i = ARCH_L_START; i <= ARCH_L_END; ++i)
    {
      heat_[i] = qsub8(heat_[i], random8(0, ((cooling_ * 10) / NUM_L) + 2));
    }

    // --- Step 2: heat diffusion upward (each arch independently) ---
    for (int k = ARCH_R_END; k >= ARCH_R_START + 2; --k)
    {
      heat_[k] = (heat_[k - 1] + heat_[k - 2] + heat_[k - 2]) / 3;
    }
    for (int k = ARCH_L_END; k >= ARCH_L_START + 2; --k)
    {
      heat_[k] = (heat_[k - 1] + heat_[k - 2] + heat_[k - 2]) / 3;
    }

    // --- Step 3: random sparks near the base of each arch ---
    if (random8() < sparking_)
    {
      int y = random8(7);
      heat_[ARCH_R_START + y] = qadd8(heat_[ARCH_R_START + y], random8(160, 255));
    }
    if (random8() < sparking_)
    {
      int y = random8(7);
      heat_[ARCH_L_START + y] = qadd8(heat_[ARCH_L_START + y], random8(160, 255));
    }

    // --- Step 4A: map Right arch to colors in normal order ---
    for (int j = 0; j < NUM_R; ++j)
    {
      CRGB color = HeatColor(heat_[ARCH_R_START + j]);
      leds_[ARCH_R_START + j] = color; // normal order
    }

    // --- Step 4B: map Left arch to colors in reverse order (base near center) ---
    for (int j = 0; j < NUM_L; ++j)
    {
      CRGB color = HeatColor(heat_[ARCH_L_START + j]);
      leds_[ARCH_L_END - j] = color; // reversed to put base at inner end
    }
  }

  // Accessors / utilities
  inline const CRGB *buffer() const { return leds_; }
  inline CRGB *buffer() { return leds_; }
  inline size_t size() const { return (size_t)NUM_LEDS; }

  // Tuning
  inline void setCooling(uint8_t v) { cooling_ = v; }   // 20..100 typical
  inline void setSparking(uint8_t v) { sparking_ = v; } // 50..200 typical

  inline void blitTo(CRGB *dst, size_t count) const
  {
    const size_t m = (count < NUM_LEDS) ? count : NUM_LEDS;
    memcpy(dst, leds_, m * sizeof(CRGB));
  }

private:
  // Defaults chosen for outdoor diffusers; tweak to taste
  uint8_t cooling_ = 85;  // higher = shorter flames
  uint8_t sparking_ = 75; // higher = more active base

  CRGB leds_[NUM_LEDS];
  uint8_t heat_[NUM_LEDS];
};
