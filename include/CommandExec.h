#pragma once
#include <Arduino.h>



// Starts the command executor task that consumes commands from the console queue
// and prints replies with io_printf().
//
// Returns: true on success, false on failure.
bool command_exec_start(UBaseType_t priority = 2,
                        uint32_t stack_bytes = 4096,
                        BaseType_t core = 1);