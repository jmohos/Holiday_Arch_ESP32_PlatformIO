#include <Arduino.h>
#include "Faults.h"
#include "IoSync.h"

uint32_t system_faults = 0;
const char *FAULT_STRING[] = {
  "CONSOLE_TASK_FAULT",
  "COMMAND_EXEC_TASK_FAULT",
  "SHOW_TASK_FAULT",
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
