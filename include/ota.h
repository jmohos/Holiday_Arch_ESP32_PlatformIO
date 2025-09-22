#pragma once
#include <Arduino.h>

//
// How to upload via ArduinoOTA over WiFi:
//
// 1. You must upload firmware via USB that has the ArduinoOTA task running
//    to receive uploads.  Uncomment "upload_protocol = espota" in the platformio.ini.
// 2. After a board has the OTA support, make note of its IP address.
// 3. To upload a build, open a terminal and run:
//    pio run -t upload -e xiao_esp32c6 --upload-port 192.168.50.30
//
//    Make sure to select the same environment and IP address as the board.
//

// Starts the over-the-air udpater task.
//
// Returns: true on success, false on failure.
bool ota_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 0);