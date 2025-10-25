//
// Proximity Detector
//
// If wiring to a TOFSense-F2 Mini Lidar, add -DPROX_TYPE_LIDAR to the platformio.ini build flags.
// If wiring to a PIR AM312 sensor, add -DPROX_TYPE_PIR to the platformio.ini build flags.
//
#include <Arduino.h>

#include "CommandQueues.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "Pins.h"
#include "ProxDetect.h"

// Update rate for distance readings.
static constexpr uint16_t DETECTION_FPS = 25;
static constexpr float MAX_RANGE = 32767.0;
static constexpr uint16_t DETECT_MIN_CLOSE_READINGS = 2; /* Units of frames, see DETECTION_FPS */
static constexpr uint16_t DETECT_MIN_FAR_READINGS = 10;  /* " */
static constexpr uint16_t RANGE_CONSEQ_COUNT_MAX = 100;


enum class DetectState : uint8_t
{
  UNKNOWN = 0,
  FAR = 1,
  CLOSE = 2
};
DetectState detectState = DetectState::UNKNOWN;


#if defined PROX_TYPE_LIDAR
#include <Wire.h> // I2C drivers

// I2C Device address for the sensor.
// The address is a combination of the base address + programmable device ID.
// This allows multiple sensors to be wired to the same I2C bus.
#define deviceaddress 0x08




bool sensor_online = false;
float sensor_distance = MAX_RANGE;
float range_mm = MAX_RANGE;

// Detection hysterysis thresholds.
// A detection is triggered if we go from being above max for x readings
// to being below min for y readings.
static constexpr uint16_t DETECT_HYST_MIN_MM = 750;      /* Must be closer than this for "close". */
static constexpr uint16_t DETECT_HYST_MAX_MM = 1000;     /* Must be farther than this for "far". */



//
// Example code sourced from: https://wiki.dfrobot.com/SKU_SEN0646_TOF_laser_ranging_sensor_7.8m#Arduino%20Sample%20Code
//
void i2c_writeN(uint8_t registerAddress, uint8_t *buf, size_t len)
{

  Wire.beginTransmission(deviceaddress);
  Wire.write(registerAddress); // MSB
  Wire.write(buf, len);
  Wire.endTransmission();
  // delay(4);
}

int16_t i2c_readN(uint8_t registerAddress, uint8_t *buf, size_t len)
{
  uint8_t i = 0;
  Wire.beginTransmission(deviceaddress);
  Wire.write(registerAddress);
  if (Wire.endTransmission(false) != 0)
  {
    return -1;
  }

  Wire.requestFrom(deviceaddress, len);
  delay(1); // Give it a chance to start responding
  while (Wire.available())
  {
    buf[i++] = Wire.read();
  }
  return i;
}


// Read only the distance fields in registers 0x24-0x27
// See the TOF sensor I2C protocol register definitions for details.
//
bool recJustDistance()
{
  uint8_t pdata[4]; // Read cache array
  if (i2c_readN(0x24, pdata, 4) == 4)
  {
    // Capture distance in mm.
    sensor_distance = (float)(((uint32_t)pdata[0x00]) | ((uint32_t)pdata[0x01] << 8) | ((uint32_t)pdata[0x02] << 16) | ((uint32_t)pdata[0x03] << 24));
  }
  else
  {
    return false;
  }
  return true;
}

// Return the last processed reading.
float prox_range()
{
  return range_mm;
}

// Main proximity detection task.
static void ProxDetectTask(void *)
{

  uint16_t range_close_count = 0;
  uint16_t range_far_count = 0;

  io_printf("[Prox] Task starting...\n");

  // Configure the I2C bus pins for DATA and CLOCK.
  sensor_online = Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);

  if (!sensor_online) {
    FAULT_SET(FAULT_TOF_SENSOR_INIT_FAIL);
    io_printf("Failed to detect and initialize TOF sensor!\n");
  }

  // Framerate control
  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / DETECTION_FPS);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;)
  {
    if (sensor_online)
    {

      // For speed, read just the distance.
      if (recJustDistance())
      {
        if (sensor_distance < 0.01) {
          // Did not detect anyything, assume max range in this case.
          range_mm = MAX_RANGE;
        }
        else 
        {
          range_mm = sensor_distance;
        }
      }
      else 
      {
      range_mm = MAX_RANGE;
      }

      //io_printf("mm: %f\n", range_mm);

      // Filter range reading to one of three zones, far, near or in between.
      if (range_mm <= DETECT_HYST_MIN_MM)
      {
        if (range_close_count < RANGE_CONSEQ_COUNT_MAX)
        {
          range_close_count++;
        }
        range_far_count = 0;
      }
      else if (range_mm >= DETECT_HYST_MAX_MM)
      {
        range_close_count = 0;
        if (range_far_count < RANGE_CONSEQ_COUNT_MAX)
        {
          range_far_count++;
        }
      }
      else
      {
        range_close_count = 0;
        range_far_count = 0;
      }

      // Detect changes in detected zones.
      switch (detectState)
      {
      case DetectState::UNKNOWN:
        // Must start with clear detection area on start.
        if (range_far_count >= DETECT_MIN_FAR_READINGS)
        {
          detectState = DetectState::FAR;
        }
        break;

      case DetectState::FAR:
        if (range_close_count >= DETECT_MIN_CLOSE_READINGS)
        {
          // Far to close detect event occurred!
          detectState = DetectState::CLOSE;

          // Queue up an event indicating a local station proximity trigger.
          SendShowQueue(ShowInputQueueMsg{ShowInputQueueCmd::TriggerLocal, 0});
        }
        break;

      case DetectState::CLOSE:
        if (range_far_count >= DETECT_MIN_FAR_READINGS)
        {
          // Close to far detect event occurred!
          detectState = DetectState::FAR;
        }

        break;
      }
    }

    // Frame pacing
    vTaskDelayUntil(&lastWake, frameTicks);
  }
}



#endif


#if defined PROX_TYPE_PIR

void init_pir_sensor() 
{
  // Use the same connector for the PIR sensor as the I2C TOF sensor.
  pinMode(I2C_SDA_PIN, INPUT);
}

bool is_pir_detected()
{
  if (digitalRead(I2C_SDA_PIN))
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Return the last processed reading.
float prox_range()
{
  // Creating a pseudo reading using the PIR switch input.
  if (is_pir_detected())
  {
    return 10.0;
  }
  else{
    return MAX_RANGE;
  }
}

// Main proximity detection task.
static void ProxDetectTask(void *)
{
  uint16_t range_close_count = 0;
  uint16_t range_far_count = 0;
  bool detected = false;

  io_printf("[Prox] Task starting in PIR mode...\n");
  init_pir_sensor();

  // Framerate control
  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / DETECTION_FPS);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;)
  {
  
      detected = is_pir_detected();
      //io_printf("PIR detected: %d\n", detected);

      // Filter range reading to one of three zones, far, near or in between.
      if (detected)
      {
        if (range_close_count < RANGE_CONSEQ_COUNT_MAX)
        {
          range_close_count++;
        }
        range_far_count = 0;
      }
      else
      {
        range_close_count = 0;
        if (range_far_count < RANGE_CONSEQ_COUNT_MAX)
        {
          range_far_count++;
        }
      }

      // Detect changes in detected zones.
      switch (detectState)
      {
      case DetectState::UNKNOWN:
        // Must start with clear detection area on start.
        if (range_far_count >= DETECT_MIN_FAR_READINGS)
        {
          detectState = DetectState::FAR;
        }
        break;

      case DetectState::FAR:
        if (range_close_count >= DETECT_MIN_CLOSE_READINGS)
        {
          // Far to close detect event occurred!
          detectState = DetectState::CLOSE;

          // Queue up an event indicating a local station proximity trigger.
          SendShowQueue(ShowInputQueueMsg{ShowInputQueueCmd::TriggerLocal, 0});
        }
        break;

      case DetectState::CLOSE:
        if (range_far_count >= DETECT_MIN_FAR_READINGS)
        {
          // Close to far detect event occurred!
          detectState = DetectState::FAR;
        }

        break;
      }

    // Frame pacing
    vTaskDelayUntil(&lastWake, frameTicks);
  }
}
#endif


bool prox_detect_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core)
{
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ProxDetectTask, "ProxDetectTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}