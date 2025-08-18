#include "IoSync.h"

static SemaphoreHandle_t g_ioMtx = nullptr;

// esp_log will call this instead of its default vprintf
static int locked_vprintf(const char* fmt, va_list args)
{
  if (!g_ioMtx) {
    // minimal fallback if init order changes
    return vprintf(fmt, args);
  }
  xSemaphoreTake(g_ioMtx, portMAX_DELAY);
  int ret = vprintf(fmt, args);   // keep IDF’s default sink (UART/USB CDC)
  xSemaphoreGive(g_ioMtx);
  return ret;
}

void io_sync_init()
{
  if (!g_ioMtx) {
    g_ioMtx = xSemaphoreCreateMutex();
  }
  esp_log_set_vprintf(locked_vprintf);
}

SemaphoreHandle_t io_mutex() { return g_ioMtx; }

void io_print(const char* s)
{
  if (!g_ioMtx) { Serial.print(s); return; }
  xSemaphoreTake(g_ioMtx, portMAX_DELAY);
  Serial.print(s);
  xSemaphoreGive(g_ioMtx);
}

void io_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  if (!g_ioMtx) {
    // fallback: no lock yet
    vprintf(fmt, args);
    va_end(args);
    return;
  }
  xSemaphoreTake(g_ioMtx, portMAX_DELAY);
  // Use Arduino Serial to keep all output on the same sink the serial monitor is reading.
  // Format into a small buffer in chunks to avoid large allocations.
  char buf[256];
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  if (n > 0) {
    Serial.write((const uint8_t*)buf, (size_t)std::min(n, (int)sizeof(buf)));
  }
  xSemaphoreGive(g_ioMtx);
  va_end(args);
}
