#include <Arduino.h>

#include "CommandQueues.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "Pins.h"
#include "ProxDetect.h"
#include <Wire.h>  // I2C drivers

// Uncomment only ONE sensor type
//#define TOF_SENSOR_POPOLU_VL53L1X
#define TOF_SENSOR_TYPE_TOFSENSE_UART


//---------------- Pololu VL53L1X ----------------------
#if defined(TOF_SENSOR_POPOLU_VL53L1X)

#include <VL53L1X.h>
VL53L1X tof_sensor;
static constexpr uint16_t TOF_DETECTION_FPS = 15;

//---------------- TOFSense-F Mini Lidar ----------------------
#elif defined(TOF_SENSOR_TYPE_TOFSENSE_UART)

static constexpr uint16_t TOF_DETECTION_FPS = 25;
const int HEADER = 0x59;      //frame header of data package

#endif



bool sensor_online = false;
float range_mm = 32768.0;

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

#if defined(TOF_SENSOR_TYPE_TOFSENSE_UART)
//
// Process the serial input from TOFSense-F device in NLINK protocol.
// We expect to receive repeating frames with the following format
// from the sensor without any config on our part:
//
// 57 00 ff 00 73 d2 2f 00 2e 00 00 01 64 00 ff 5c
// Byte Field
// 0     57 = Header
// 1     00 = Function Mark
// 2     xx = Reserved
// 3     00 = id
// 4-7   xx = System Time
// 8-10  xx = Distance
// 11    xx = Distance Status
// 12-13 xx = Signal Strength
// 14    xx = Range Precision
// 15    xx = Checksum
uint16_t read_tofsense_f2() 
{
  int msg_index=0;
  int check;
  int dist;                     //actual distance measurements of LiDAR
  int strength;                 //signal strength of LiDAR  
  static int msg[9];

  while(TOFSerial.available())
  {
    if (msg_index == 0) {
      if (TOFSerial.read() == HEADER){
        msg[0] = HEADER;
        msg_index=1;
      }
    }
    else if (msg_index < 16)
    {
      msg[msg_index] = TOFSerial.read();
      msg_index++;
    }

    if (msg_index >= 16) {
      // Verify checksum

      // Extract data if a valid frame
      io_printf("TOF FRAME COMPLETE!\n");
      msg_index = 0;
    }
  }





  // if (TOFSerial.available())
  // {
  //   if (TOFSerial.read() == HEADER)
  //   {
  //     uart[0] = HEADER;
  //     if (TOFSerial.read() == HEADER)      //assess data package frame header 0x59
  //     {
  //       uart[1] = HEADER;
  //       for (i = 2; i < 9; i++)         //save data in array
  //       {
  //         uart[i] = TOFSerial.read();
  //       }
  //       check = uart[0] + uart[1] + uart[2] + uart[3] + uart[4] + uart[5] + uart[6] + uart[7];
  //       if (uart[8] == (check & 0xff))        //verify the received data as per protocol
  //       {
  //         dist = uart[2] + uart[3] * 256;     //calculate distance value
  //         strength = uart[4] + uart[5] * 256; //calculate signal strength value
  //         Serial.print("distance = ");
  //         Serial.print(dist);                 //output measure distance value of LiDAR
  //         Serial.print('\t');
  //         Serial.print("strength = ");
  //         Serial.print(strength);             //output signal strength value
  //         Serial.print('\n');
  //       }
  //     }
  //   }
  // }
  return 0;
}  
#endif



float prox_range() {
  return range_mm;
}

static void ProxDetectTask(void*) {

  uint16_t range_close_count = 0;
  uint16_t range_far_count = 0;

  io_printf("[Prox] Task started.\n");

#if defined(TOF_SENSOR_POPOLU_VL53L1X)

  // Configure the I2C bus pins for DATA and CLOCK.
  //Wire.setSDA(I2C_SDA_PIN);
  //Wire.setSCL(I2C_SCL_PIN);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  //Wire.setClock(400000);  // use 400 kHz I2C

  tof_sensor.setTimeout(500);

  if (tof_sensor.init()) {
    sensor_online = true;

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


  } else {
    FAULT_SET(FAULT_TOF_SENSOR_INIT_FAIL);
    io_printf("Failed to detect and initialize TOF sensor!\n");
  }
#elif defined(TOF_SENSOR_TYPE_TOFSENSE_UART)
  // Open serial port to the TOFSense uart
  TOFSerial.begin(115200, EspSoftwareSerial::SWSERIAL_8N1, TOF_RX_PIN, -1);
#endif // VL53L1X sensor init


  // Framerate control
  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / TOF_DETECTION_FPS);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;)
  {
    if (sensor_online) {

#if defined(TOF_SENSOR_POPOLU_VL53L1X)

  // Blocking single read from the sensor.
  range_mm = tof_sensor.readSingle();
#elif defined(TOF_SENSOR_TYPE_TOFSENSE_UART)
  range_mm = read_tofsense_f2();
#endif
      
      if (range_mm < 1) {
        //io_printf("TOF max reading!\n");
        range_mm = 2500;
      }

      //io_printf("mm: %f\n", range_mm);

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
            // Far to close detect event occurred!
            detectState = DetectState::CLOSE;

            // Queue up an event indicating a local station proximity trigger.
            SendShowQueue( ShowInputQueueMsg{ ShowInputQueueCmd::TriggerLocal, 0 } );
          }
        break;

        case DetectState::CLOSE:
          if (range_far_count >= DETECT_MIN_FAR_READINGS) {
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

bool prox_detect_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ProxDetectTask, "ProxDetectTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}