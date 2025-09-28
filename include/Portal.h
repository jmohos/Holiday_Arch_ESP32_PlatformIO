// Portal.h — simple moving stripes (candy-cane style) across entire strip
#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <string.h>

template <size_t N>
class PortalEffect {
public:
  // Kept for backward compatibility with previous usage; args intentionally ignored.
  PortalEffect(int /*r_start*/, int /*r_end*/, int /*l_start*/, int /*l_end*/) {
    bandWidth_  = 6;    // pixels per color band
    msPerShift_ = 70;   // lower = faster motion
    direction_  = +1;   // +1 left->right, -1 right->left (visual)
    colorA_     = CRGB::White;
    colorB_     = CRGB::Black;
    t0_         = millis();
    fill_solid(leds_, N, CRGB::Black);
  }

  // Optional simpler ctor if you ever want it:
  PortalEffect() : PortalEffect(0,0,0,0) {}

  inline void setColors(const CRGB& a, const CRGB& b) { colorA_ = a; colorB_ = b; }
  inline void setBandWidth(uint8_t px)               { bandWidth_ = px ? px : 1; }
  inline void setSpeedMsPerShift(uint16_t ms)        { msPerShift_ = ms ? ms : 1; }
  inline void setDirection(int dir)                  { direction_ = (dir >= 0) ? +1 : -1; }
  // Back-compat: accept two dirs but only the first matters now.
  inline void setDirs(int dirRight, int /*dirLeft*/) { setDirection(dirRight); }

  // Call every frame; pass reset=true to restart motion phase
  inline void update(bool reset=false) {
    const uint32_t now = millis();
    if (reset) { t0_ = now; }

    const uint32_t elapsed = now - t0_;
    const int shift  = direction_ * (int)(elapsed / msPerShift_); // pixel offset
    const int period = bandWidth_ * 2;

    for (int i = 0; i < (int)N; ++i) {
      const int phase = posmod_(i + shift, period);
      const int band  = (phase / bandWidth_) & 1;
      leds_[i] = band ? colorA_ : colorB_;
    }
  }

  inline void blitTo(CRGB* dst, size_t count) const {
    const size_t m = (count < N) ? count : N;
    memcpy(dst, leds_, m * sizeof(CRGB));
  }

  inline const CRGB* buffer() const { return leds_; }
  inline size_t size() const        { return N; }

private:
  uint8_t  bandWidth_;
  uint16_t msPerShift_;
  int      direction_;
  CRGB     colorA_, colorB_;
  uint32_t t0_ = 0;
  CRGB     leds_[N]{};

  static inline int posmod_(int a, int m) { int r = a % m; return (r < 0) ? r + m : r; }
};

