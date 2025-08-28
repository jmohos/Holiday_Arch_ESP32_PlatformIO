#pragma once
#include <Arduino.h>

// Audio animation show indices.
enum class AudioAnim : uint8_t { Silence = 0, Bells = 1, Scream = 2, Count };
static constexpr uint8_t NUM_AUDIO_ANIMATIONS = static_cast<uint8_t>(AudioAnim::Count);


// Starts the audio task that drives the mp3 player hardware.
//
// Returns: true on success, false on failure.
bool audio_start(UBaseType_t priority = 2,
                uint32_t stack_bytes = 4096,
                BaseType_t core = 1);

// Halts any audio playback.
void stop_audio_player();

// Starts playing audio file in specific numbered folder.
void play_audio_file(int folder, int file);

// Sets playback volume.
void set_volume(uint8_t volume);

// Reports true is audio is currently playing.
bool is_audio_playing();
