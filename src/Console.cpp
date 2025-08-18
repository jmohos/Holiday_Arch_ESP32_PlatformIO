#include "Console.h"
#include "IoSync.h"

extern "C" {
  #include "esp_log.h"
  #include "freertos/task.h"
}

static const char* TAG_CON = "console";
static QueueHandle_t g_cmdq = nullptr;

static inline int core_id() { return xPortGetCoreID(); }

static inline bool is_ws(char c) { return c == ' ' || c == '\t'; }

// Tokenize a line into cmd + argv without dynamic allocation.
// Truncates tokens that exceed CON_MAX_* limits.
static void tokenize_line(const String& line, CommandMsg& out)
{
  out.cmd[0] = '\0';
  out.argc   = 0;

  // Walk the string and extract tokens separated by spaces/tabs
  auto push_tok = [&](const String& tok, bool first){
    if (tok.length() == 0) return;
    if (first) {
      strncpy(out.cmd, tok.c_str(), CON_MAX_CMD - 1);
      out.cmd[CON_MAX_CMD - 1] = '\0';
    } else if (out.argc < CON_MAX_ARGS) {
      strncpy(out.argv[out.argc], tok.c_str(), CON_MAX_TOK - 1);
      out.argv[out.argc][CON_MAX_TOK - 1] = '\0';
      out.argc++;
    }
  };

  String tok;
  bool first = true;
  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];
    if (is_ws(c)) {
      if (tok.length()) { push_tok(tok, first); first = false; tok = ""; }
    } else {
      tok += c;
    }
  }
  if (tok.length()) { push_tok(tok, first); }
}

void console_print_menu() {
  io_printf(
    "\nCommands (examples):\n"
    "  help\n"
    "  trigger audio 5\n"
    "  trigger light 2\n"
    "  trigger show 1\n"
    "  mode off\n"
    "  log dbg\n"
  );
}

static void ConsoleReaderTask(void* /*pv*/)
{
  String line;
  ESP_LOGI(TAG_CON, "Console reader on core %d. Type 'help' + Enter.", core_id());
  io_printf("> ");

  while (true) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\r') continue;

      if (c == '\n') {
        line.trim();
        if (line.length() > 0) {
          CommandMsg msg;
          tokenize_line(line, msg);
          if (msg.cmd[0] != '\0' && g_cmdq) {
            xQueueSend(g_cmdq, &msg, 0);
          }
        }
        line = "";
        io_printf("> ");
      } else {
        // simple backspace handling
        if ((c == 0x08 || c == 0x7F)) {
          if (line.length() > 0) line.remove(line.length() - 1);
        } else if (isPrintable(c)) {
          line += c;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

QueueHandle_t console_start(size_t queue_len, UBaseType_t task_priority, BaseType_t core)
{
  if (!g_cmdq) {
    g_cmdq = xQueueCreate(queue_len, sizeof(CommandMsg));
    if (!g_cmdq) {
      ESP_LOGE(TAG_CON, "Failed to create command queue");
      return nullptr;
    }
  }
  BaseType_t ok = xTaskCreatePinnedToCore(
      ConsoleReaderTask, "ConsoleReader", 4096, nullptr, task_priority, nullptr, core);
  if (ok != pdPASS) {
    ESP_LOGE(TAG_CON, "Failed to start ConsoleReader task");
    vQueueDelete(g_cmdq);
    g_cmdq = nullptr;
    return nullptr;
  }
  return g_cmdq;
}

QueueHandle_t console_get_queue() { return g_cmdq; }
