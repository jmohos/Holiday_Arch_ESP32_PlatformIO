#include <Arduino.h>
#include "Faults.h"
#include "IoSync.h"

uint32_t system_faults = 0;
const char *FAULT_STRING[] = {
  "CONSOLE_TASK_FAULT",
  "COMMAND_EXEC_TASK_FAULT",
  "SHOW_TASK_FAULT",
  "CONFIG_RESTORE_FAULT",
  "NETWORK_TASK_FAULT",
  "AUDIO_TASK_FAULT",
  "LIGHT_TASK_FAULT",
  "MOTOR_TASK_FAULT",
  "PROX_DETECT_TASK_FAULT",  
  "TOF_SENSOR_INIT_FAIL",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT",
  "UNDEFINED_FAULT"
};

void print_faults() {
  bool any_active = false;
    for (int i = 0; i < FAULT_MAX_INDEX; i++) {
      if (FAULT_ACTIVE(i)) {
        io_printf("%s\n", FAULT_STRING[i]);
        any_active = true;
      }
    }
    if (!any_active) {
      io_printf("None\n");
    }
  }
