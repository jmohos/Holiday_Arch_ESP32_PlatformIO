// LightningBolt.h  — header-only
//
// Generate a lightning bolt animation over the specified number of pixels.
//
// Adjustable parameters:
//   Head speed:
//     SPEED_MIN_PX_S_ = 150;   // ~1.0 s
//     SPEED_MAX_PX_S_ = 300;   // ~0.5 s
//   Trail length:
//     TRAIL_MIN_ = 6;
//     TRAIL_MAX_ = 12;
//   Per frame fade:
//     BASE_FADE_ = 120;
//     (try 100–140; smaller numbers = slower fade)
//   Global strobe flash (strike_.strobe)
//     Adjust random range to:
//       random8(1, 3);  // 1–2 frames
//       random8(0, 2);  // 0–1 frame (often none)
//
#pragma once
#include <Arduino.h>
#include <FastLED.h>

template <size_t N>
class LightningBolt {
public:
  CRGB leds[N];  // public FastLED-compatible buffer

  // Call every frame (e.g., 60 fps). Pass reset=true to clear/reseed.
  inline void update(bool reset = false) {
    const uint32_t t = millis();
    if (reset) {
      fill_solid(leds, N, CRGB::Black);
      strike_ = Strike{};
      scheduleNextStrike_(t);
      prevFadeTick_ = t;
    }

    // Frame-rate independent fade (maps ~120 per 16 ms)
    uint32_t dt = t - prevFadeTick_;
    prevFadeTick_ = t;
    uint8_t fade = (dt >= 255) ? 255 : (uint8_t)constrain((uint16_t)(BASE_FADE_ * dt / 16), 0, 255);
    fadeToBlackBy(leds, N, fade);

    // Launch strikes on schedule
    if (!strike_.active && t >= nextStrikeDue_) {
      startStrike_(t);
      scheduleNextStrike_(t);
    }

    // Draw current strike (if any)
    if (strike_.active) {
      drawStrike_(t);
    }
  }

  inline void blitTo(CRGB* dst, size_t count) const {
    const size_t m = (count < N) ? count : N;
    memcpy(dst, leds, m * sizeof(CRGB));
  }
  inline const CRGB* buffer() const { return leds; }
  inline size_t size() const { return N; }

  // Optional knobs
  void setStrikeRateHz(float minHz, float maxHz) {
    if (minHz < 0.1f) minHz = 0.1f;
    if (maxHz < minHz) maxHz = minHz;
    strikeGapMinMs_ = (uint16_t)(1000.0f / maxHz);
    strikeGapMaxMs_ = (uint16_t)(1000.0f / minHz);
  }
  void setSpeedPxPerSec(uint16_t minSpd, uint16_t maxSpd) {
    SPEED_MIN_PX_S_ = minSpd; SPEED_MAX_PX_S_ = maxSpd;
  }
  void setTrail(uint8_t minTrail, uint8_t maxTrail) {
    TRAIL_MIN_ = minTrail; TRAIL_MAX_ = maxTrail;
  }
  void setBaseFade(uint8_t fade) { BASE_FADE_ = fade; }

private:
  // Defaults chosen for a visible top→bottom motion on ~150 px
  uint8_t  BASE_FADE_       = 120;   // lower = slower fade (100–140 is nice)
  uint8_t  HALO_BLUE_DIV_   = 6;     // larger = weaker blue halo
  uint8_t  TRAIL_MIN_       = 6;
  uint8_t  TRAIL_MAX_       = 12;
  uint16_t SPEED_MIN_PX_S_  = 300;   // ~0.5 s over 150 px
  uint16_t SPEED_MAX_PX_S_  = 900;   // ~0.16 s over 150 px
  uint16_t strikeGapMinMs_  = 400;   // ~1.25–2.5 Hz default
  uint16_t strikeGapMaxMs_  = 800;

  uint32_t nextStrikeDue_   = 0;
  uint32_t prevFadeTick_    = 0;

  struct Strike {
    bool     active   = false;
    uint32_t t0       = 0;
    float    speedPxMs= 1.5f;
    uint8_t  trail    = 10;
    uint8_t  strobe   = 0;  // 0–1 frame “flash”
  } strike_;

  inline void scheduleNextStrike_(uint32_t base) {
    const uint16_t gap = random16(strikeGapMinMs_, (uint16_t)(strikeGapMaxMs_ + 1));
    nextStrikeDue_ = base + gap;
  }
  inline void startStrike_(uint32_t t) {
    strike_.active   = true;
    strike_.t0       = t;
    const uint16_t spx_s = random16(SPEED_MIN_PX_S_, (uint16_t)(SPEED_MAX_PX_S_ + 1));
    strike_.speedPxMs= spx_s / 1000.0f;
    strike_.trail    = random8(TRAIL_MIN_, (uint8_t)(TRAIL_MAX_ + 1));
    strike_.strobe   = random8(0, 2);  // often none
  }

  inline void drawStrike_(uint32_t t) {
    if (strike_.strobe) {  // gentle global flash
      for (size_t i = 0; i < N; ++i) leds[i] += CRGB(2, 2, 3);
      strike_.strobe--;
    }

    const uint32_t elapsed = t - strike_.t0;
    const float headF      = (float)(N - 1) - (strike_.speedPxMs * (float)elapsed);
    const int   head       = (int)floorf(headF);

    // Strike ends once head + trail fully below ground
    if (head < -(int)strike_.trail) { strike_.active = false; return; }

    const int topLit = min((int)N - 1, head + strike_.trail);
    const int bot    = max(0, head);

    for (int y = bot; y <= topLit; ++y) {
      const int d = y - head;         // 0 at head
      if (d < 0 || d > strike_.trail) continue;

      // Keep head solid, add dropout farther away
      uint8_t keepProb = (d <= 2) ? 255 : (uint8_t)(220 - (uint16_t)d * 200u / max<int>(1, strike_.trail));
      if (random8() > keepProb) continue;

      const uint8_t b = 255 - scale8(255, (uint8_t)((d * 255) / max<uint8_t>(1, strike_.trail)));
      if (y == head) { leds[y] = CRGB(255, 255, 255); } // crisp head
      leds[y] += CRGB(b, b, b);                         // white core
      leds[y] += CRGB(0, 0, b / HALO_BLUE_DIV_);        // blue halo
    }

    if (head < 8) {                   // faint “ground flash”
      const int span = (N < 12) ? (int)N : 12;
      for (int y = 0; y < span; ++y) leds[y] += CRGB(3, 3, 6);
    }
  }
};
