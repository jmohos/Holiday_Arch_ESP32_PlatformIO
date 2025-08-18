#include "ConsoleUtils.h"
#include <ctype.h>
#include <string.h>

bool arg_as_int(const CommandMsg& msg, int i, int& out) {
  if (i < 0 || i >= msg.argc) return false;
  const char* s = msg.argv[i];
  if (!s || !*s) return false;

  // allow optional leading '-' and digits
  int j = 0;
  if (s[0] == '-') j++;
  for (; s[j]; ++j) {
    if (!isdigit((unsigned char)s[j])) return false;
  }
  out = atoi(s);
  return true;
}

bool arg_as_bool(const CommandMsg& msg, int i, bool& out) {
  if (i < 0 || i >= msg.argc) return false;
  const char* s = msg.argv[i];
  if (!s) return false;

  if (!strcasecmp(s, "on") || !strcasecmp(s, "true") || !strcmp(s, "1")) {
    out = true; return true;
  }
  if (!strcasecmp(s, "off") || !strcasecmp(s, "false") || !strcmp(s, "0")) {
    out = false; return true;
  }
  return false;
}

const char* arg_as_str(const CommandMsg& msg, int i) {
  if (i < 0 || i >= msg.argc) return nullptr;
  return msg.argv[i];
}
