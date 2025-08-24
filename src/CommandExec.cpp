#include "CommandExec.h"
#include "CommandQueues.h"
#include "Console.h"
#include "ConsoleUtils.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "NetService.h"
#include "SettingsStore.h"

Persist::SettingsStore* _cfg = nullptr;
NetService *_net = nullptr;


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
        io_printf(" net show       -Show network status.\n");        
        io_printf(" restart        -Reboot the CPU.\n");
        io_printf(" show start     -Enable show mode.\n");
        io_printf(" show stop      -Disable show mode.\n");
        io_printf(" show trigger x -Trigger show anim x.\n");
        io_printf(" audio play x   -Play audio file x.\n");
        io_printf(" audio stop     -Stop playing audio.\n");
        io_printf(" audio volume x -Set audio volume to x.\n");
        io_printf(" light play x   -Play audio file x.\n");
        io_printf(" light stop     -Stop playing audio.\n");
        io_printf(" motor play x   -Play audio file x.\n");
        io_printf(" motor stop     -Stop playing audio.\n");
        io_printf(" motor home     -Home the motor.\n");

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
            // cfg set id
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
            // cfg set ssid
            else if (!strcasecmp(arg2, "ssid")) {
              const char* ssid = arg_as_str(msg, 2);
              io_printf("Setting cfg WiFi SSID to: %s\n", ssid);
              _cfg->setSsid(ssid);
            }
            // cfg set pass
            else if (!strcasecmp(arg2, "pass")) {
              const char* pass = arg_as_str(msg, 2);
              io_printf("Setting cfg WiFI password to: %s\n", pass);
              _cfg->setPassword(pass);
            }
            else {
              io_printf("Unsupported parameter: %s\n", arg2);
            }
          }
          // cfg load
          else if (!strcasecmp(arg1, "load")) {
            if (_cfg->load()) {
              io_printf("Successfully load config parameters.");
            } else {
              io_printf("Failed to load config parameters!");
            }
          }
          // cfg save
          else if (!strcasecmp(arg1, "save")) {
            if (_cfg->save()) {
              io_printf("Successfully saved config parameters.");
            } else {
              io_printf("Failed to save config parameters!");
            }
          }
          // cfg defaults
          else if (!strcasecmp(arg1, "defaults")) {
            _cfg->setDefaults();
            io_printf("Default config parameters loaded, not saved.");
          }
          else {
            io_printf("Unsupported command: %s\n", arg1);
          }
        }
      } else if (!strcasecmp(msg.cmd, "net")) {
        if (!_net ) {
          io_printf("Network manager unavailable!");
          continue;
        }
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1) {
          if (!strcasecmp(arg1, "show")) {
            // "net show"
            io_printf("Network Status:\n");
            io_printf("  Connected: %s\n", _net->isConnected() ? "CONNECTED" : "DISCONNECTED");
            io_printf("  SSID: >%s<\n", _net->ssid().c_str());
            io_printf("  IP: %s\n", _net->localIP().c_str());
            io_printf("  GW: %s\n", _net->gatewayIP().c_str());
            io_printf("  Subnet: %s\n", _net->subnetMask().c_str());
            io_printf("  BSSID: %s\n", _net->bssid().c_str());
            io_printf("  Chan: %u\n", _net->channel());
            io_printf("  RSSI: %d dBm\n", _net->rssi());
            io_printf("  McastIP: %s\n", _net->mcastIP().c_str());
            io_printf("  McastPort: %u\n", _net->mcastPort());
            io_printf("  Mcast Listening: %s\n", _net->isListening() ? "LISTENING" : "IDLE");
            io_printf(" -------------\n");

          }
          else {
            io_printf("Unsupported command: %s\n", arg1);
          }
        } else {

        }
      } else if (!strcasecmp(msg.cmd, "show")) {
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1 && !strcasecmp(arg1, "start")){
          // show start
          io_printf("Queued up show start\n");
          SendShow( ShowInputMsg{ ShowCmd::Start } );
        }
        else if (arg1  && !strcasecmp(arg1, "stop")){
          // show stop
          io_printf("Queued up show stop\n");
          SendShow( ShowInputMsg{ ShowCmd::Stop } );
        }
        else if (arg1  && !strcasecmp(arg1, "trigger")){
          // show trigger x
          int index;
          if (arg_as_int(msg, 1, index)) {
            io_printf("Queued up show trigger index: %d\n", index);
            SendShow( ShowInputMsg{ ShowCmd::Trigger, static_cast<unsigned char>(index) } );
          } else {
            io_printf("Errr, Missing valid show index!");  
          }
        }
        else {
          io_printf("usage: show start/stop/trigger <index>\n");
        }

      } else if (!strcasecmp(msg.cmd, "audio")) {
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1 && !strcasecmp(arg1, "play")){
          // audio play x
          int index;
          if (arg_as_int(msg, 1, index)) {
            io_printf("Queued up audio play %d.\n", index);
            SendAudio( AudioCmdMsg{ AudioCmd::Play, static_cast<unsigned char>(index) } );
          } else {
            io_printf("Error, missing valid audio file number!");  
          }
        }
        else if (arg1  && !strcasecmp(arg1, "stop")){
          // audio stop
          io_printf("Queued up audio stop\n");
          SendAudio( AudioCmdMsg{ AudioCmd::Stop } );
        }
        else if (arg1  && !strcasecmp(arg1, "volume")){
          // audio volume x
          int volume;
          if (arg_as_int(msg, 1, volume)) {
            io_printf("Queued up audio volume %d\n", volume);
            SendAudio( AudioCmdMsg{ AudioCmd::Volume, static_cast<unsigned char>(volume) } );
          } else {
            io_printf("Error, invalid volume!");  
          }
        }
        else {
          io_printf("usage: audio play/stop/volume <val>\n");
        }

      } else if (!strcasecmp(msg.cmd, "light")) {
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1 && !strcasecmp(arg1, "play")){
          // light play x
          int index;
          if (arg_as_int(msg, 1, index)) {
            io_printf("Queued up light play %d.\n", index);
            SendLight( LightCmdMsg{ LightCmd::Play, static_cast<unsigned char>(index) } );
          } else {
            io_printf("Error, missing valid light index number!");  
          }
        }
        else if (arg1  && !strcasecmp(arg1, "stop")){
          // light stop
          io_printf("Queued up light stop\n");
          SendLight( LightCmdMsg{ LightCmd::Stop } );
        }
        else {
          io_printf("usage: light play/stop <val>\n");
        }

      } else if (!strcasecmp(msg.cmd, "motor")) {
        const char* arg1 = arg_as_str(msg, 0);
        if (arg1 && !strcasecmp(arg1, "play")){
          // motor play x
          int index;
          if (arg_as_int(msg, 1, index)) {
            io_printf("Queued up motor play %d.\n", index);
            SendMotor( MotorCmdMsg{ MotorCmd::Play, static_cast<unsigned char>(index) } );
          } else {
            io_printf("Error, missing valid motor index number!");  
          }
        }
        else if (arg1  && !strcasecmp(arg1, "stop")){
          // motor stop
          io_printf("Queued up motor stop\n");
          SendMotor( MotorCmdMsg{MotorCmd::Stop } );
        }
        else if (arg1  && !strcasecmp(arg1, "home")){
          // motor home
          io_printf("Queued up motor home\n");
          SendMotor( MotorCmdMsg{MotorCmd::Home } );
        }
        else {
          io_printf("usage: motor play/stop/home <val>\n");
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

bool command_exec_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core, Persist::SettingsStore* cfg, NetService *net) {
  TaskHandle_t h = nullptr;
  _cfg = cfg;
  _net = net;
  BaseType_t ok = xTaskCreatePinnedToCore(
      CommandExecTask, "CommandExec", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}
