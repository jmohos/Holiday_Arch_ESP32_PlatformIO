/*
 * Time of Flight (range) Sensor Manager class
 *
 *
 * @author Joe Mohos
 * Contact: jmohos@yahoo.com
 */
#ifndef TOF_MANAGER_H
#define TOF_MANAGER_H

#include <Arduino.h>
#include <elapsedMillis.h>
#include "pins.h"
#include <Wire.h>
#include <VL53L1X.h>

class TOFManager {

public:



private:
  VL53L1X _sensor;
  bool _sensor_online = false;
  elapsedMillis _tof_update_period_elapsed;
  static constexpr int TOF_UPDATE_PERIOD_MS = 33;  /* 33msec = 30 FPS */

public:

  //
  //
  TOFManager() {
  }

  // Return the latest ranging reading in millimeters.
  int Get_Range_Millimeters() {
    if (_sensor_online) {
      return _sensor.ranging_data.range_mm;
    } else {
      return 0;
    }
  }

  // Setup the time of flight library to activate the device.
  void Setup() {

    // Configure the I2C bus pins for DATA and CLOCK.
    Wire.setSDA(I2C_SDA_PIN);
    Wire.setSCL(I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(400000);  // use 400 kHz I2C

    _sensor.setTimeout(500);
    if (_sensor.init()) {
      _sensor_online = true;

      // You can change these settings to adjust the performance of the sensor, but
      // the minimum timing budget is 20 ms for short distance mode and 33 ms for
      // medium and long distance modes. See the VL53L1X datasheet for more
      // information on range and timing limits.
      _sensor.setDistanceMode(VL53L1X::Medium);
      //_sensor.setMeasurementTimingBudget(50000); /* Used for Long mode */
      _sensor.setMeasurementTimingBudget(33000); /* Used for Medium mode */

      // Start continuous readings at a rate of one measurement every 50 ms (the
      // inter-measurement period). This period should be at least as long as the
      // timing budget.
      //_sensor.startContinuous(50); /* Used for Long mode */
      _sensor.startContinuous(33); /* Used for medium mode */

    } else {
      Serial.println("Failed to detect and initialize TOF sensor!");
      _sensor_online = false;
    }
  }


  // Periodically update the reading from the sensor.
  void Update() {
    if (_tof_update_period_elapsed < TOF_UPDATE_PERIOD_MS) {
      return;
    }
    _tof_update_period_elapsed = 0;

    if (!_sensor_online) {
      //Serial.println("TOF offline!");
    } else {

      // Perform a read of the sensor.
      _sensor.read();

      //Serial.print("range: ");
      //Serial.println(_sensor.ranging_data.range_mm);
      // Serial.print("\tstatus: ");
      // Serial.print(VL53L1X::rangeStatusToString(_sensor.ranging_data.range_status));
      // Serial.print("\tpeak signal: ");
      // Serial.print(_sensor.ranging_data.peak_signal_count_rate_MCPS);
      // Serial.print("\tambient: ");
      // Serial.print(_sensor.ranging_data.ambient_count_rate_MCPS);
      // Serial.println();
    }
  }


private:
};


#endif