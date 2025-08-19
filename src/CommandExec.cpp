#include "CommandExec.h"
#include "Console.h"
#include "ConsoleUtils.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "SettingsStore.h"

Persist::SettingsStore* _cfg = nullptr;

static void CommandExecTask(void*) {
  QueueHandle_t q = console_get_queue();
  CommandMsg msg;
  while (true) {
    if (xQueueReceive(q, &msg, portMAX_DELAY) == pdTRUE) {
      if (!strcasecmp(msg.cmd, "help")) {
        io_printf(" help           -This list of help commands.\n");
        io_printf(" cfg show       -Show all config items.\n");
        io_printf(" cfg set id x   -Set config device id. (1-254)\n");
        io_printf(" cfg set ssid x -Set WiFi SSID to x.\n");
        io_printf(" cfg set pass x -Set WiFi password to x.\n");
        io_printf(" cfg defaults   -Load config defaults.\n");
        io_printf(" cfg load       -Load config from memory.\n");
        io_printf(" cfg save       -Save config to memory.\n");
        io_printf(" faults         -Report list of active faults.\n");
        io_printf(" restart        -Reboot the CPU.\n");
        io_printf(" trigger x      -Trigger animaion sequence. x\n");

      } else if (!strcasecmp(msg.cmd, "cfg")) {
        if (!_cfg) {
          io_printf("Invalid configuration contents!");
          continue;
        }
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1) {
          if (!strcasecmp(arg1, "show")) {
            // "cfg show"
            io_printf("Configuration Contents:\n");
            io_printf(" device id: %u\n", _cfg->deviceId());
            io_printf(" WiFi SSID: >%s< Len: %u\n", _cfg->ssid().c_str(), _cfg->ssid().length());
            io_printf(" WiFi pass: >%s< Len: %u\n", _cfg->password().c_str(), _cfg->password().length());
          }
          else if (!strcasecmp(arg1, "set")) {
            // We need two arguments after set.
            const char* arg2 = arg_as_str(msg, 1);
            const char* arg3 = arg_as_str(msg, 2);
            if ((!arg2) || !(arg3)) {
              io_printf("Invalid cfg set parameters!");
              continue;
            }
            if (!strcasecmp(arg2, "id")) {
              int id = 0;
              if (arg_as_int(msg, 2, id)) {
                if (id < 0 || id > 254) {
                  io_printf("Invalid id, must be between 1 and 254!");
                } else {
                  io_printf("Setting cfg device id to: %u\n", id);
                  _cfg->setDeviceId(id);
                }
              } else {
                io_printf("Invalid id input!");
              }
            }
            else if (!strcasecmp(arg2, "ssid")) {
              const char* ssid = arg_as_str(msg, 2);
              io_printf("Setting cfg WiFi SSID to: %s\n", ssid);
              _cfg->setSsid(ssid);
            }
            else if (!strcasecmp(arg2, "pass")) {
              const char* pass = arg_as_str(msg, 2);
              io_printf("Setting cfg WiFI password to: %s\n", pass);
              _cfg->setPassword(pass);
            }
            else {
              io_printf("Unsupported parameter: %s\n", arg2);
            }
          }
          else if (!strcasecmp(arg1, "load")) {
            if (_cfg->load()) {
              io_printf("Successfully load config parameters.");
            } else {
              io_printf("Failed to load config parameters!");
            }
          }
          else if (!strcasecmp(arg1, "save")) {
            if (_cfg->save()) {
              io_printf("Successfully saved config parameters.");
            } else {
              io_printf("Failed to save config parameters!");
            }
          }
          else if (!strcasecmp(arg1, "defaults")) {
            _cfg->setDefaults();
            io_printf("Default config parameters loaded, not saved.");
          }
          else {
            io_printf("Unsupported command: %s\n", arg1);
          }
        }

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
  
      } else if (!strcasecmp(msg.cmd, "restart")) {
        io_printf("Rebooting...\n");
        Serial.flush();
        delay(1000);
        ESP.restart();

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

bool command_exec_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core, Persist::SettingsStore* cfg) {
  TaskHandle_t h = nullptr;
  _cfg = cfg;
  BaseType_t ok = xTaskCreatePinnedToCore(
      CommandExecTask, "CommandExec", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}
