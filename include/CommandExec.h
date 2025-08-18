#pragma once
#include <Arduino.h>

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
}

// Starts the command executor task that consumes commands from the console queue
// and prints replies with io_printf().
//
// Returns: true on success, false on failure.
bool command_exec_start(BaseType_t core = 1,
                        UBaseType_t priority = 2,
                        uint32_t stack_bytes = 4096);
