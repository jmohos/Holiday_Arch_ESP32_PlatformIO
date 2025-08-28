#include <Arduino.h>
#include <FastLED.h>

#include "CommandQueues.h"
#include "IoSync.h"
#include "Light.h"
#include "Logging.h"
#include "Pins.h"


// Constants for hardware interfaces, move to pins.h
#define LED_PIN RGB_LED_DATA_PIN
#define NUM_LEDS 60
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_BRIGHTNESS_DEFAULT 128
#define LIGHT_FPS_DEFAULT 60



// ---------- Module state ----------
static CRGB     g_leds[NUM_LEDS];
static uint8_t  g_brightness = LED_BRIGHTNESS_DEFAULT;
static uint16_t g_targetFps  = LIGHT_FPS_DEFAULT;

static bool     g_playing     = false;
static uint8_t  g_animIndex   = static_cast<uint8_t>(LightAnim::CandyCane);
static bool     g_animReset   = true;   // set true on animation change

// ---------- Helpers ----------
static inline uint32_t now_ms() { return (uint32_t) (esp_timer_get_time() / 1000ULL); }

void light_set_brightness(uint8_t b) {
  g_brightness = b;
  FastLED.setBrightness(g_brightness);
}

void light_set_fps(uint16_t fps) {
  g_targetFps = (fps == 0) ? 1 : fps;
}

// ---------- Animations ----------
// Each render function draws one frame. If reset==true, clear/init its internal state.

// CandyCane: moving red/white stripes.
static void anim_candycane(bool reset) {
  static uint32_t t0 = 0;
  if (reset) { t0 = now_ms(); fill_solid(g_leds, NUM_LEDS, CRGB::Black); }

  const uint32_t t = now_ms() - t0;
  const uint8_t  stripeWidth = 3;          // pixels per color block
  const uint8_t  speedMsPerShift = 90;     // lower = faster
  const int      offset = (t / speedMsPerShift) % (stripeWidth * 2);

  for (int i = 0; i < NUM_LEDS; ++i) {
    const int band = ((i + offset) / stripeWidth) & 1;
    g_leds[i] = band ? CRGB(220,0,0) : CRGB::White;
  }
}

// Flames: classic FastLED fire (1D).
static void anim_flames(bool reset) {
  static uint8_t heat[NUM_LEDS];
  if (reset) {
    memset(heat, 0, sizeof(heat));
    fill_solid(g_leds, NUM_LEDS, CRGB::Black);
  }

  // 1) Cool down every cell a little
  for (int i = 0; i < NUM_LEDS; ++i) {
    uint8_t cooldown = random8(0, 12); // tweak for intensity
    heat[i] = qsub8(heat[i], cooldown);
  }

  // 2) Heat from each cell drifts 'up' and diffuses a little
  for (int k = NUM_LEDS - 1; k >= 2; --k) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // 3) Randomly ignite new 'sparks' near the bottom
  if (random8() < 120) { // chance of spark this frame
    int y = random8(0, min(7, NUM_LEDS)); // bottom few LEDs
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // 4) Map heat to colors
  for (int j = 0; j < NUM_LEDS; ++j) {
    g_leds[j] = HeatColor(heat[j]);
  }
}

// Bounce: single bright pixel bouncing back & forth with fading trail.
static void anim_bounce(bool reset) {
  static int pos = 0;
  static int dir = 1;
  if (reset) {
    pos = 0; dir = 1;
    fill_solid(g_leds, NUM_LEDS, CRGB::Black);
  }

  // Fade existing content for trail
  fadeToBlackBy(g_leds, NUM_LEDS, 40);

  // Draw the head
  g_leds[pos] = CRGB::Blue;

  // Step
  pos += dir;
  if (pos <= 0)       { pos = 0; dir = 1; }
  else if (pos >= NUM_LEDS - 1) { pos = NUM_LEDS - 1; dir = -1; }
}

// Dispatch table
typedef void (*AnimFn)(bool reset);
static AnimFn kAnims[NUM_LIGHT_ANIMATIONS] = { anim_candycane, anim_flames, anim_bounce };




static void LightTask(void*) {
  io_printf("[Light] Task started. num=%d, pin=%d, fps=%u, brightness=%u\n",
          NUM_LEDS, LED_PIN, (unsigned)g_targetFps, (unsigned)g_brightness);

  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / g_targetFps);
  TickType_t lastWake = xTaskGetTickCount();

  LightCmdQueueMsg msg{};

 for (;;) {
    // Drain any pending commands quickly (non-blocking)
    while (xQueueReceive(queueBus.lightCmdQueueHandle, &msg, 0) == pdPASS) {
      io_printf("[Light] Cmd=%u param=%u\n", (unsigned)msg.cmd, (unsigned)msg.param);
      switch (msg.cmd) {
        case LightQueueCmd::Play: {
          uint8_t idx = msg.param % NUM_LIGHT_ANIMATIONS;
          if (!g_playing || idx != g_animIndex) {
            g_animIndex = idx;
            g_animReset = true;
          }
          g_playing = true;
          break;
        }
        case LightQueueCmd::Stop: {
          g_playing = false;
          g_animReset = true;            // ensure clean start next time
          fill_solid(g_leds, NUM_LEDS, CRGB::Black);
          FastLED.show();
          break;
        }
        default:
          // Ignore other commands for now
          break;
      }
    }

    // Render one frame if playing
    if (g_playing) {
      if (g_animIndex < NUM_LIGHT_ANIMATIONS) {
        kAnims[g_animIndex](g_animReset);
        g_animReset = false;
      } else {
        // Safety: if index invalid, just clear
        fill_solid(g_leds, NUM_LEDS, CRGB::Black);
      }
      FastLED.show();
    }

    // Frame pacing
    vTaskDelayUntil(&lastWake, frameTicks);
  }
}


// ---------- Init/start ----------
static bool light_hw_init_once() {
  static bool inited = false;
  if (inited) return true;

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(g_leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setBrightness(g_brightness);
  // Optional: color correction to taste
  FastLED.setCorrection(TypicalLEDStrip);

  inited = true;
  return true;
}

bool light_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {

  // Initialize the hardware interface
  if (!light_hw_init_once()) return false;

  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      LightTask, "LightTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}