#pragma once

//
// IoSync provides mutex protected access to the Serial port output.
// This allows multiple threads to print to the Serial port without
// conflict.
// Use io_print() in place of Serial.print().
//
#include <Arduino.h>
#include "Logging.h"
#include <stdarg.h>

// Initialize the shared I/O lock and hook esp_log's vprintf to a locked version.
void io_sync_init();

// Thread-safe print helpers for anything that would otherwise use Serial.print()
void io_print(const char* s);         // prints raw string (no newline)
void io_printf(const char* fmt, ...); // printf-style

// For convenience if you need the lock elsewhere
SemaphoreHandle_t io_mutex();
