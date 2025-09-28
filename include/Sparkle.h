// Sparkle.h — header-only additive sparkle overlay
#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <string.h>

// Add twinkles on top of an existing CRGB buffer (in-place).
// Keeps a small pool of active sparkles with random lifetimes.
//
// Usage:
//   #include "Sparkle.h"
//   fx::SparkleOverlay<32> sparkle; // pool size 32
//   sparkle.setRate(10.0f);
//   sparkle.setLifetime(90, 200);
//   sparkle.setIntensity(255);
//   sparkle.setMaxActive(28);
//   ...
//   sparkle.apply(g_leds, NUM_LEDS); // after drawing base effect

template <size_t POOL=32>
class SparkleOverlay {
public:
  struct Config {
    float births_per_sec = 8.0f;
    uint16_t min_ms = 80;
    uint16_t max_ms = 180;
    uint8_t intensity = 255;
    uint8_t max_active = 24;
  };

  SparkleOverlay() { reset(); }

  inline void reset() {
    for (size_t i=0;i<POOL;++i) pool_[i].used = false;
    last_ms_ = millis();
  }

  inline void setRate(float births_per_sec) { cfg_.births_per_sec = births_per_sec; }
  inline void setLifetime(uint16_t min_ms, uint16_t max_ms) {
    cfg_.min_ms = min_ms; cfg_.max_ms = max_ms ? max_ms : min_ms;
    if (cfg_.max_ms < cfg_.min_ms) cfg_.max_ms = cfg_.min_ms;
  }
  inline void setIntensity(uint8_t intensity) { cfg_.intensity = intensity; }
  inline void setMaxActive(uint8_t m) { cfg_.max_active = m; }

  // Optional: exclude a center index ±radius from spawning
  inline void setExclude(int centerIndex, int radius) { ex_center_ = centerIndex; ex_radius_ = radius; }

  // Apply sparkles additively to leds[0..num_leds-1]
  inline void apply(CRGB* leds, int num_leds) {
    if (!leds || num_leds <= 0) return;

    const uint32_t now = millis();
    float dt_s = (now - last_ms_) / 1000.0f;
    if (dt_s < 0) dt_s = 0;
    last_ms_ = now;

    // expire
    int active = 0;
    for (size_t i=0;i<POOL;++i) {
      auto &s = pool_[i];
      if (s.used && now >= s.end_ms) s.used = false;
      if (s.used) ++active;
    }

    // spawn by expected rate
    float expected = cfg_.births_per_sec * dt_s;
    while (expected > 0.0f && active < cfg_.max_active) {
      if (expected >= 1.0f) { spawn_(num_leds); ++active; expected -= 1.0f; }
      else {
        if (random16(65535) < (uint16_t)(expected * 65535.0f)) { spawn_(num_leds); ++active; }
        break;
      }
    }

    // render
    for (size_t i=0;i<POOL;++i) {
      auto &s = pool_[i];
      if (!s.used) continue;
      if (s.idx < 0 || s.idx >= num_leds) { s.used = false; continue; }

      const uint32_t age = now - s.start_ms;
      const uint32_t dur = s.end_ms - s.start_ms;
      if (dur == 0) { s.used = false; continue; }

      // fast rise to ~20%, then decay
      float x = (float)age / (float)dur;                    // 0..1
      float env = (x < 0.2f) ? (x / 0.2f) : (1.0f - (x - 0.2f) / 0.8f);
      if (env < 0.0f) env = 0.0f;
      uint8_t b = (uint8_t)(env * cfg_.intensity);

      leds[s.idx] += CRGB(b, b, b); // cool white pop
    }
  }

  inline Config& config() { return cfg_; }
  inline const Config& config() const { return cfg_; }

private:
  struct Sparkle {
    int idx = -1;
    uint32_t start_ms = 0;
    uint32_t end_ms = 0;
    bool used = false;
  };

  inline void spawn_(int num_leds) {
    int free_i = -1;
    for (size_t i=0;i<POOL;++i) { if (!pool_[i].used) { free_i = (int)i; break; } }
    if (free_i < 0) return;

    int candidate = random16(num_leds);
    if (ex_radius_ >= 0) {
      int tries = 4;
      while (tries-- && abs(candidate - ex_center_) <= ex_radius_) candidate = random16(num_leds);
    }

    Sparkle &sp = pool_[free_i];
    sp.idx = candidate;
    sp.start_ms = millis();
    const uint16_t life = random16(cfg_.min_ms, (uint16_t)(cfg_.max_ms + 1));
    sp.end_ms = sp.start_ms + life;
    sp.used = true;
  }

  Config cfg_;
  Sparkle pool_[POOL];
  uint32_t last_ms_ = 0;

  // optional exclusion (e.g., apex)
  int ex_center_ = -1;
  int ex_radius_ = -1;
};

