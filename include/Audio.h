#pragma once
#include <Arduino.h>

// Audio animation show indices.
enum class AudioAnim : uint8_t { 
  SILENCE = 0,  // Stop playback
  ONE = 1,      // 
  TWO = 2,      // 
  THREE = 3,    // 
  FOUR = 4,     // 
  FIVE = 5,     // 
  EMPTY6 = 6,   //
  EMPTY7 = 7,   //
  EMPTY8 = 8,   //
  EMPTY9 = 9,   //
  FIRE = 10,    // 
  THUNDER = 11,  //
  COUNT         // Must be last 
};
static constexpr uint8_t NUM_AUDIO_ANIMATIONS = static_cast<uint8_t>(AudioAnim::COUNT);


// Starts the audio task that drives the mp3 player hardware.
//
// Returns: true on success, false on failure.
bool audio_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 0);

// Halts any audio playback.
void stop_audio_player();

// Starts playing audio file in specific numbered folder.
void play_audio_file(int folder, int file);

// Sets playback volume.
void set_volume(uint8_t volume);

// Reports true is audio is currently playing.
bool is_audio_playing();
