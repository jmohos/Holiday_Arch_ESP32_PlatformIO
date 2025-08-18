#pragma once
#include "Console.h"

// Try to fetch argument i as integer.
//   Returns true on success, false if out of range or not numeric.
//   If ok==true, *out = parsed int.
bool arg_as_int(const CommandMsg& msg, int i, int& out);

// Try to fetch argument i as boolean (accepts "on/off", "true/false", "1/0").
//   Returns true if conversion succeeded.
bool arg_as_bool(const CommandMsg& msg, int i, bool& out);

// Fetch argument i as C string (null if index out of range).
const char* arg_as_str(const CommandMsg& msg, int i);
