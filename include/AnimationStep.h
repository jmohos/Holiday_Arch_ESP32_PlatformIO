// AnimationStep.h
#pragma once
#include <Arduino.h>
#include "Audio.h"
#include "Light.h"
#include "Motor.h"


// Struct capturing the discrete steps of a show.
struct AnimationStep {
    uint32_t duration_ms;  // Time to stay at this step
    LightAnim lightIndex;    // Light animation for this step.
    AudioAnim audioIndex;    // Audio animation for this step.
    MotorAnim motorIndex;    // Motor animation for this step.
};