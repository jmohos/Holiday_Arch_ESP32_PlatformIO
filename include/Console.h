#pragma once

#include <Arduino.h>
#include "Logging.h"


// ==== Tunables (raise if you need more) ====
#ifndef CON_MAX_ARGS
#define CON_MAX_ARGS   8      // max number of arguments after the command
#endif
#ifndef CON_MAX_TOK
#define CON_MAX_TOK    32     // max length of each token (bytes, incl. '\0')
#endif
#ifndef CON_MAX_CMD
#define CON_MAX_CMD    32     // max length of the command token
#endif

// One parsed console message: command + argv[], all as plain C strings.
struct CommandMsg {
  char    cmd[CON_MAX_CMD];                 // "trigger", "mode", ...
  uint8_t argc = 0;
  char    argv[CON_MAX_ARGS][CON_MAX_TOK];  // argument tokens as-is
};

// Start the console reader (prompts via io_printf). It pushes CommandMsg to a queue.
// Returns the queue handle (read in your executor task) or nullptr on error.
QueueHandle_t console_start(
  size_t queue_len = 16,
  UBaseType_t task_priority = 2,
  uint32_t stack_bytes = 4096,
  BaseType_t core = 0 );

// Access the queue after start.
QueueHandle_t console_get_queue();

