#include "CommandExec.h"
#include "Console.h"
#include "ConsoleUtils.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"


static void CommandExecTask(void*) {
  QueueHandle_t q = console_get_queue();
  CommandMsg msg;
  while (true) {
    if (xQueueReceive(q, &msg, portMAX_DELAY) == pdTRUE) {
      if (!strcasecmp(msg.cmd, "help")) {
        io_printf("help:      This list of help commands\n");
        io_printf("faults:    Report list of active faults\n");
        io_printf("trigger x: Trigger animaion sequence x\n");

      } else if (!strcasecmp(msg.cmd, "trigger")) {
        const char* kind = arg_as_str(msg, 0);
        int index;
        if (kind && arg_as_int(msg, 1, index)) {
          io_printf("trigger: %s %d\n", kind, index);
          // TODO: trigger handler here
        } else {
          io_printf("usage: trigger <kind> <index>\n");
        }

      } else if (!strcasecmp(msg.cmd, "faults")) {
        io_printf("Active system faults:\n");
        print_faults();

      } else if (!strcasecmp(msg.cmd, "mode")) {
        const char* mode = arg_as_str(msg, 0);
        if (mode) {
          io_printf("mode: %s\n", mode);
          // TODO: apply
        } else {
          io_printf("usage: mode <off|on|auto>\n");
        }

      } else if (!strcasecmp(msg.cmd, "enable")) {
        bool val;
        if (arg_as_bool(msg, 0, val)) {
          io_printf("enable: %s\n", val ? "true" : "false");
          // TODO: apply
        } else {
          io_printf("usage: enable <on|off|true|false|1|0>\n");
        }

      } else {
        io_printf("unknown command '%s'\n", msg.cmd);
      }

      // Draw prompt on newline
      io_printf("\n> ");
    }
  }
}

bool command_exec_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      CommandExecTask, "CommandExec", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}
