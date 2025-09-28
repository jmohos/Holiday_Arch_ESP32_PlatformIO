#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>

#include "CommandQueues.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "Pins.h"
#include "ProxDetect.h"

// Set update rate to no faster than the sensor sample rate. (<30fps)
static constexpr uint16_t TOF_DETECTION_FPS = 15; // 20;

VL53L1X tof_sensor;
bool sensor_online = false;
uint16_t range_mm = 32768;

// Detection hysterysis thresholds.
// A detection is triggered if we go from being above max for x readings
// to being below min for y readings.
static constexpr uint16_t DETECT_HYST_MIN_MM = 750; /* Must be closer than this for "close". */
static constexpr uint16_t DETECT_HYST_MAX_MM = 1000; /* Must be farther than this for "far". */
static constexpr uint16_t DETECT_MIN_CLOSE_READINGS = 2; /* Units of frames, see TOF_DETECTION_FPS */
static constexpr uint16_t DETECT_MIN_FAR_READINGS = 10;  /* " */
static constexpr uint16_t RANGE_CONSEQ_COUNT_MAX = 100;

enum class DetectState : uint8_t { UNKNOWN = 0, FAR = 1, CLOSE = 2 };
DetectState detectState = DetectState::UNKNOWN;


uint16_t prox_range() {
  return range_mm;
}

static void ProxDetectTask(void*) {

  uint16_t range_close_count = 0;
  uint16_t range_far_count = 0;

  io_printf("[Prox] Task started.\n");

  // Configure the I2C bus pins for DATA and CLOCK.
  //Wire.setSDA(I2C_SDA_PIN);
  //Wire.setSCL(I2C_SCL_PIN);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  //Wire.setClock(400000);  // use 400 kHz I2C

  tof_sensor.setTimeout(500);

  if (tof_sensor.init()) {
    sensor_online = true;

#if true
    // You can change these settings to adjust the performance of the sensor, but
    // the minimum timing budget is 20 ms for short distance mode and 33 ms for
    // medium and long distance modes. See the VL53L1X datasheet for more
    // information on range and timing limits.
    //tof_sensor.setDistanceMode(VL53L1X::Long);
    tof_sensor.setDistanceMode(VL53L1X::Medium);
    //tof_sensor.setDistanceMode(VL53L1X::Short);

    //tof_sensor.setMeasurementTimingBudget(50000); /* Used for Long mode */
    tof_sensor.setMeasurementTimingBudget(33000); /* Used for Medium mode */
    //tof_sensor.setMeasurementTimingBudget(20000); /* Used for Short mode */

    // Start continuous readings at a rate of one measurement every 50 ms (the
    // inter-measurement period). This period should be at least as long as the
    // timing budget.
    //tof_sensor.startContinuous(50); /* Used for Long mode */
    //tof_sensor.startContinuous(33); /* Used for medium mode */
    //tof_sensor.startContinuous(25); /* Used for short mode */
#else
tof_sensor.setDistanceMode(VL53L1X::Short);
tof_sensor.setMeasurementTimingBudget(50 * 1000); //us
tof_sensor.setROISize(8,8); // Very narrow beam in the center.
tof_sensor.startContinuous(70);
#endif

  } else {
    FAULT_SET(FAULT_TOF_SENSOR_INIT_FAIL);
    io_printf("Failed to detect and initialize TOF sensor!\n");
  }

  // Framerate control
  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / TOF_DETECTION_FPS);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;)
  {
    if (sensor_online) {

      // Blocking single read from the sensor.
      range_mm = tof_sensor.readSingle();
      
      if (range_mm == 0) {
        io_printf("TOF max reading!\n");
        range_mm = 2500;
      }

      io_printf("mm: %d\n", range_mm);

      // Filter range reading to one of three zones, far, near or in between.
      if (range_mm <= DETECT_HYST_MIN_MM) {
        if (range_close_count < RANGE_CONSEQ_COUNT_MAX) {
          range_close_count++;
        }
        range_far_count = 0;

      } else if (range_mm >= DETECT_HYST_MAX_MM) {
        range_close_count = 0;
        if (range_far_count < RANGE_CONSEQ_COUNT_MAX) {
          range_far_count++;
        }
      } else {
        range_close_count = 0;
        range_far_count = 0;
      }

      // Detect changes in detected zones.
      switch(detectState) {
        case DetectState::UNKNOWN:
          // Must start with clear detection area on start.
          if (range_far_count >= DETECT_MIN_FAR_READINGS) {
            detectState = DetectState::FAR;
          }
        break;

        case DetectState::FAR:
          if (range_close_count >= DETECT_MIN_CLOSE_READINGS) {
            detectState = DetectState::CLOSE;
            // Far to close detect event occurred!

            // Queue up an event indicating a local station proximity trigger.
            SendShowQueue( ShowInputQueueMsg{ ShowInputQueueCmd::TriggerLocal, 0 } );
            //io_printf("CLOSE detected!\n", range_mm);
          }
        break;

        case DetectState::CLOSE:
          if (range_far_count >= DETECT_MIN_FAR_READINGS) {
            detectState = DetectState::FAR;
            // Close to far detect event occurred!
            //io_printf("FAR detected!\n", range_mm);
          }
          
        break;
      }
    }
  
    // Frame pacing
    vTaskDelayUntil(&lastWake, frameTicks);
  }
}

bool prox_detect_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ProxDetectTask, "ProxDetectTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}