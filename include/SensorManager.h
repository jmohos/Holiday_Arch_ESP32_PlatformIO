#pragma once
#include <Arduino.h>
#include "elapsedMillis.h"

class SensorManager {
public:
    void begin();
    void update();
    bool isPersonDetected();
private:
    bool personDetected = false;
    elapsedMillis sensor_update_elapsed_msec;
    static constexpr int SENSOR_UPDATE_PERIOD_MSEC = 100;
};