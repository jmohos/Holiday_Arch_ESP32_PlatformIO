#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include <cstring>   // for memset

// -------- Configuration for each servo ----------
struct ServoConfig {
  int   pin;                 // GPIO pin
  int   minUs = 500;         // microseconds at 0°
  int   maxUs = 2500;        // microseconds at 180°
  bool  invert = false;      // flip direction if needed

  // Smoothing controls
  float maxDegPerSec = 180;      // velocity limit (°/s). 0 = disable smoothing
  float maxAccelDegPerSec2 = 0;  // acceleration limit (°/s^2). 0 = none
  float deadbandDeg = 0.4f;      // ignore tiny moves to reduce chatter
};

// -------- Manager ----------
class SmoothServos {
public:
  explicit SmoothServos(size_t count) : n(count) {
    sv   = new Servo[n];
    cfg  = new ServoConfig[n];
    tgt  = new float[n];
    ang  = new float[n];
    vel  = new float[n];
    std::memset(tgt, 0, sizeof(float)*n);
    std::memset(ang, 0, sizeof(float)*n);
    std::memset(vel, 0, sizeof(float)*n);
  }

  ~SmoothServos() {
    delete[] sv; delete[] cfg; delete[] tgt; delete[] ang; delete[] vel;
  }

  // Must be called before use. Returns false on any attach error.
  bool begin(const ServoConfig* configs, size_t configCount, uint16_t periodHz = 50) {
    if (configCount != n) return false;

    for (size_t i = 0; i < n; ++i) {
      cfg[i] = configs[i];

      // IMPORTANT: some ESP32Servo versions make this an instance method.
      // Set period BEFORE attach on each instance.
      sv[i].setPeriodHertz(periodHz);

      int ch = sv[i].attach(cfg[i].pin, cfg[i].minUs, cfg[i].maxUs);
      if (ch < 0) return false;

      // Initialize to mid-angle
      ang[i] = 90.0f;
      tgt[i] = 90.0f;
      writeAngleImmediate(i, ang[i]);
    }

    lastMs = millis();
    return true;
  }

  // Set a target angle (clamped to [0,180])
  void setTarget(size_t i, float degrees) {
    if (i >= n) return;
    tgt[i] = clamp180(degrees);
  }

  // Convenience: set multiple targets in one call
  void setTargets(std::initializer_list<float> degreesList) {
    size_t i = 0;
    for (float d : degreesList) { if (i < n) setTarget(i++, d); else break; }
  }

  // Jump immediately without smoothing, then continue smoothing from there
  void snapTo(size_t i, float degrees) {
    if (i >= n) return;
    ang[i] = clamp180(degrees);
    vel[i] = 0;
    writeAngleImmediate(i, ang[i]);
  }

  // Call frequently (e.g., each loop). It staggers updates to reduce simultaneous surges.
  void update() {
    uint32_t now = millis();
    float dt = (now - lastMs) / 1000.0f;
    if (dt <= 0.f) return;
    lastMs = now;

    // Round-robin start index to avoid commanding all servos at exactly the same moment
    startIndex = (startIndex + 1) % n;

    for (size_t k = 0; k < n; ++k) {
      size_t i = (startIndex + k) % n;
      float error = (tgt[i] - ang[i]);
      if (fabsf(error) <= cfg[i].deadbandDeg) continue;

      float desiredVel = (cfg[i].maxDegPerSec > 0)
                         ? constrain(error / dt, -cfg[i].maxDegPerSec, cfg[i].maxDegPerSec)
                         : (error / dt); // no vel limit

      float newVel = desiredVel;
      if (cfg[i].maxAccelDegPerSec2 > 0) {
        float maxDv = cfg[i].maxAccelDegPerSec2 * dt;
        newVel = constrain(desiredVel, vel[i] - maxDv, vel[i] + maxDv);
      }

      float step = newVel * dt;
      // Don’t overshoot target
      if ((error > 0 && step > error) || (error < 0 && step < error)) step = error;

      float next = clamp180(ang[i] + step);
      if (fabsf(next - ang[i]) >= 0.05f) { // tiny write suppression
        ang[i] = next;
        vel[i] = newVel;
        writeAngleImmediate(i, ang[i]);
      } else {
        vel[i] = 0; // effectively stopped
      }
    }
  }

  size_t count() const { return n; }
  float  angle(size_t i) const { return (i < n) ? ang[i] : 0.f; }
  float  target(size_t i) const { return (i < n) ? tgt[i] : 0.f; }

private:
  size_t n;
  Servo* sv;
  ServoConfig* cfg;
  float* tgt;
  float* ang;
  float* vel;
  uint32_t lastMs{0};
  size_t startIndex{0};

  static float clamp180(float d) { return d < 0 ? 0 : (d > 180 ? 180 : d); }

  void writeAngleImmediate(size_t i, float degrees) {
    // Map angle -> µs using per-servo limits and optional inversion
    float d = cfg[i].invert ? (180.0f - degrees) : degrees;
    int us = cfg[i].minUs + (int)((cfg[i].maxUs - cfg[i].minUs) * (d / 180.0f));
    sv[i].writeMicroseconds(us);
  }
};
