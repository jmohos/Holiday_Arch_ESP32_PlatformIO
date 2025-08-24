#include "SensorManager.h"
#include "IoSync.h"


void SensorManager::begin() {
    //Logger.logInfo("SensorManager", "SensorManager begin (stub).");
}

void SensorManager::update() {
  if (sensor_update_elapsed_msec >= SENSOR_UPDATE_PERIOD_MSEC) {
    sensor_update_elapsed_msec = 0;

    //io_printf("Sensor update tick...\n");

  }
}

bool SensorManager::isPersonDetected() {
    return false; // Stub: No real detection yet
}
