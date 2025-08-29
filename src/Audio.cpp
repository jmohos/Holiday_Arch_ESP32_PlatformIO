#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"

#include "Audio.h"
#include "CommandQueues.h"
#include "Faults.h"
#include "IoSync.h"
#include "Logging.h"
#include "Pins.h"

static constexpr uint8_t MP3_MAX_VOLUME = 30;

//DFPlayer Mini MP3 Music Player Support
DFRobotDFPlayerMini myDFPlayer;

// For ESP32 we have to use hardware serial, not Serialx
HardwareSerial HWSerial1(1);

bool audio_online = false;
uint8_t volume = 20; // TODO: Store in config and restore.
uint8_t folder = 1;  // Assuming all files are in folder "01".


// Initialize the MP3 audio playback device using a dedicated serial port.
// We also setup a GPIO input pin to monitor the state of the player to know
// if it is still working on an audio file.
bool init_audio_player(int volume) {
  // Open serial port to DFPlayer device at 9600 baud
  HWSerial1.begin(9600, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);

  delay(1000);
  io_printf("Initializing DFRobot DFPlayer...");

  // Configure MP3 Busy monitoring pin at GPIO input
  pinMode(MP3_BUSY_PIN, INPUT_PULLUP);

  // Activate the audio player interface
  if (!myDFPlayer.begin(HWSerial1, true, true)) {// Use Acks, Hard reset (false=fewer pops)
    return false;
  } else {
    myDFPlayer.setTimeOut(1000); //Set serial communictaion time out 1000ms
    myDFPlayer.volume(volume);  //Set volume between 0 and 30.
    delay(30);
  }
  return true;
}

// Halt audio playback.
void stop_audio_player() {
  myDFPlayer.stop();
  io_printf("Stopping audio playback...");
}

// Play a file in the specified folder.
void play_audio_file(uint8_t folder, uint8_t file) {
  myDFPlayer.playFolder(folder, file);
  io_printf("Playing audio folder: %d, file: %d\n", folder, file);
}

// Set the volume for playback.
void set_volume(uint8_t volume) {
  if (volume > MP3_MAX_VOLUME) {
    volume = MP3_MAX_VOLUME;
  }
  myDFPlayer.volume(volume);  //Set volume between 0 and 30.
  io_printf("Setting audio volume to %d out of %d.", volume, MP3_MAX_VOLUME);
}

// Test if we are still busy playing the last audio file.
// Return true if playing, false if idle.
bool is_audio_playing() {
  // The MP3_BUSY_PIN is pulled low when the MP3 player is active.
  if (digitalRead(MP3_BUSY_PIN)) {
    // Player is idle
    return false;
  } else {
    return true;
  }
}


static void AudioTask(void*) {
  AudioCmdQueueMsg in_msg{};

  audio_online = init_audio_player(volume);
  if (!audio_online) {
    io_printf("ERROR: DFPlayer Mini is offline!\n");
    FAULT_SET(FAULT_MP3_PLAYER_INIT_FAIL);
  } else {
    io_printf("DFPlayer Mini online.\n");
  }

  for (;;)
  {

    while (xQueueReceive(queueBus.audioCmdQueueHandle, &in_msg, 0) == pdPASS) {
      //io_printf("Received incoming audio command: %d, param: %d\n", in_msg.cmd, in_msg.param);

      if (!audio_online) {
        io_printf("Cannot process audio command, device offline!");
      } else {
        switch(in_msg.cmd) {
          case AudioQueueCmd::Play:
            if (in_msg.param >= NUM_AUDIO_ANIMATIONS) {
              io_printf("Invalid audio animation index %d\n", in_msg.param);
            }
            else {
              if (in_msg.param ==  static_cast<uint8_t>(AudioAnim::SILENCE)) {
                stop_audio_player();
              } else {
                // Map the audio animation index to a file on the MP3 SD card.
                play_audio_file(folder, in_msg.param);
              }
            }
          break;

          case AudioQueueCmd::Stop:
            stop_audio_player();
          break;

          case AudioQueueCmd::Volume:
            set_volume(in_msg.param);
          break;

          default:
          // Unsupported
          break;

        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool audio_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      AudioTask, "AudioTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}