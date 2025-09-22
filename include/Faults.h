#pragma once

#include <Arduino.h>

typedef enum {
  FAULT_CONSOLE_TASK_FAULT = 0,
  FAULT_CMD_EXEC_TASK_FAULT = 1,
  FAULT_SHOW_TASK_FAULT = 2,
  FAULT_CONFIG_RESTORE_FAULT = 3,
  FAULT_NETWORK_TASK_FAULT = 4,
  FAULT_AUDIO_TASK_FAULT = 5,
  FAULT_LIGHT_TASK_FAULT = 6,
  FAULT_MOTOR_TASK_FAULT = 7,
  FAULT_PROX_DETECT_TASK_FAULT = 8,
  FAULT_TOF_SENSOR_INIT_FAIL = 9,
  FAULT_MP3_PLAYER_INIT_FAIL = 10,
  FAULT_OTA_TASK_FAULT = 11,
  FAULT_MAX_INDEX = 12
} SYSTEM_FAULT_T;

// Forward reference to master system fault bits in master .ini file.
extern uint32_t system_faults;
extern const char *FAULT_STRING[];

// Fault macros.  Replace x with the fault enumeration name.
#define FAULT_SET(x) system_faults = (system_faults | ((uint32_t)1 << x))
#define FAULT_CLEAR(x) system_faults = (system_faults &= ~((uint32_t)1 << x))
#define FAULT_ACTIVE(x) ((system_faults & (uint32_t)1 << x) != 0)

void print_faults();