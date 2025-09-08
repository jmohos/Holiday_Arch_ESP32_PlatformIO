#include <Arduino.h>
#include <FastLED.h>

#include "CommandQueues.h"
#include "IoSync.h"
#include "Light.h"
#include "LightningBolt.h"
#include "Logging.h"
#include "Pins.h"

// Addressable LED light strip config.
#define LED_PIN RGB_LED_DATA_PIN
#define NUM_LEDS 300
#define NUM_LEDS_ARCH_R 150
#define NUM_LEDS_ARCH_L 150
#define ARCH_R_START 0
#define ARCH_R_END 149
#define ARCH_L_START 150
#define ARCH_L_END 299
#define LED_TYPE WS2815 /* WS2812B */ /* WS2811 */
#define COLOR_ORDER RGB /* GRB */     /* RGB */
#define LED_BRIGHTNESS_DEFAULT 128    /* 128  = nominal, 255 = max */
#define LIGHT_FPS_DEFAULT 60

// ---------- Module state ----------
static CRGB g_leds[NUM_LEDS];
static uint8_t g_brightness = LED_BRIGHTNESS_DEFAULT;
static uint16_t g_targetFps = LIGHT_FPS_DEFAULT;

static bool g_playing = false;
static uint8_t g_animIndex = static_cast<uint8_t>(LightAnim::BLANK);
static bool g_animReset = true; // set true on animation change

// ---------- Helpers ----------
static inline uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000ULL); }

void light_set_brightness(uint8_t b)
{
  g_brightness = b;
  FastLED.setBrightness(g_brightness);
}

void light_set_fps(uint16_t fps)
{
  g_targetFps = (fps == 0) ? 1 : fps;
}


// ---------- Lightning Bolt Animation ----------
LightningBolt<NUM_LEDS_ARCH_R> lightning;


// ---------- Flame Animation ----------

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING 85

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 75

// ---------- Blank Animation ----------

static void anim_blank(bool reset)
{
  // Regardless of reset, we always clear the LEDS.
  FastLED.clear(true);
}


// ---------- Flame Animation ----------
//
// FLAMES: classic FastLED fire (1D).
// Based on Fire2012 in Fastled library.
// Flames are animated twice, once for each half of the arch.
// Right side is pixels 0+.
// Left side is revesed with the base as the last pixel in the strip.
static void anim_flames(bool reset)
{
  static uint8_t heat[NUM_LEDS];

  if (reset)
  {
    memset(heat, 0, sizeof(heat));
    fill_solid(g_leds, NUM_LEDS, CRGB::Black);
  }

  // Two flame patterns split half way between strip.
  // First half is 0-NUM_LEDS/2 in normal order.
  // Second half is NUM_LEDS/2 to NUM_LEDS in reversed order.

  // Step 1A.  Cool down every Right cell a little
  for (int i = ARCH_R_START; i <= ARCH_R_END; i++)
  {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS_ARCH_R) + 2));
  }

  // Step 1B.  Cool down every Left cell a little
  for (int i = ARCH_L_START; i <= ARCH_L_END; i++)
  {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS_ARCH_L) + 2));
  }

  // Step 2A.  Right arch heat drift and diffusion
  for (int k = ARCH_R_END; k >= ARCH_R_START + 2; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 2B.  Left arch heat drift and diffusion
  for (int k = ARCH_L_END; k >= ARCH_L_START + 2; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3A.  Right random 'spark' in first 7 positions.
  if (random8() < SPARKING)
  {
    int y = random8(7);
    heat[ARCH_R_START + y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 3B.  Left random 'spark' in first 7 positions.
  if (random8() < SPARKING)
  {
    int y = random8(7);
    heat[ARCH_L_START + y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4A.  Map Right segment in normal order to heat colors.
  for (int j = 0; j < NUM_LEDS_ARCH_R; j++)
  {
    CRGB color = HeatColor(heat[ARCH_R_START + j]);
    int pixelnumber;
    // Map in normal order.
    pixelnumber = ARCH_R_START + j;
    g_leds[pixelnumber] = color;
  }

  // Step 4B.  Map Left segment in normal order to heat colors.
  for (int j = 0; j < NUM_LEDS_ARCH_L; j++)
  {
    CRGB color = HeatColor(heat[ARCH_L_START + j]);
    int pixelnumber;
    // Map in reverse order.
    pixelnumber = ARCH_L_END - j;
    g_leds[pixelnumber] = color;
  }
}


// ---------- Lightning Bolt Animation ----------
//
// Lightning effect (1D vertical strip: index 0 = ground, NUM_LEDS-1 = sky)
// We are going to duplicate the effect on both halves of the arch.

static void anim_lightning_bolts(bool reset)
{
  lightning.update(reset);     // render one arch's worth
  lightning.blitTo(g_leds, NUM_LEDS_ARCH_R); // copy into the right arch

  // Copy the right arch into the left
  for (int j = 0; j < NUM_LEDS_ARCH_R; j++)
  {
    g_leds[ARCH_L_END - j] = g_leds[j];
  }

}



// CANDYCANE: moving red/white stripes.
static void anim_candycane(bool reset)
{
  static uint32_t t0 = 0;
  if (reset)
  {
    t0 = now_ms();
    fill_solid(g_leds, NUM_LEDS, CRGB::Black);
  }

  const uint32_t t = now_ms() - t0;
  const uint8_t stripeWidth = 3;      // pixels per color block
  const uint8_t speedMsPerShift = 90; // lower = faster
  const int offset = (t / speedMsPerShift) % (stripeWidth * 2);

  for (int i = 0; i < NUM_LEDS; ++i)
  {
    const int band = ((i + offset) / stripeWidth) & 1;
    g_leds[i] = band ? CRGB(220, 0, 0) : CRGB::White;
  }
}


// BOUNCE: single bright pixel bouncing back & forth with fading trail.
static void anim_bounce(bool reset)
{
  static int pos = 0;
  static int dir = 1;
  if (reset)
  {
    pos = 0;
    dir = 1;
    fill_solid(g_leds, NUM_LEDS, CRGB::Black);
  }

  // Fade existing content for trail
  fadeToBlackBy(g_leds, NUM_LEDS, 40);

  // Draw the head
  g_leds[pos] = CRGB::Blue;

  // Step
  pos += dir;
  if (pos <= 0)
  {
    pos = 0;
    dir = 1;
  }
  else if (pos >= NUM_LEDS - 1)
  {
    pos = NUM_LEDS - 1;
    dir = -1;
  }
}

// Dispatch table, must match mapping in Light.h for animations.
typedef void (*AnimFn)(bool reset);
static AnimFn kAnims[NUM_LIGHT_ANIMATIONS] = {anim_blank, anim_candycane, anim_flames, anim_bounce, anim_lightning_bolts};

// Master LightTask
//
static void LightTask(void *)
{
  io_printf("[Light] Task started. num=%d, pin=%d, fps=%u, brightness=%u\n",
            NUM_LEDS, LED_PIN, (unsigned)g_targetFps, (unsigned)g_brightness);

  const TickType_t frameTicks = pdMS_TO_TICKS(1000UL / g_targetFps);
  TickType_t lastWake = xTaskGetTickCount();

  LightCmdQueueMsg msg{};

  for (;;)
  {
    // Drain any pending commands quickly (non-blocking)
    while (xQueueReceive(queueBus.lightCmdQueueHandle, &msg, 0) == pdPASS)
    {
      // io_printf("[Light] Cmd=%u param=%u\n", (unsigned)msg.cmd, (unsigned)msg.param);

      // Process the incoming command.
      switch (msg.cmd)
      {
      case LightQueueCmd::Play:
      {
        if (msg.param >= NUM_LIGHT_ANIMATIONS)
        {
          io_printf("Invalid light animation index: %d\n", msg.param);
        }
        else
        {
          if (!g_playing || msg.param != g_animIndex)
          {
            // Start a new light animation.
            g_animIndex = msg.param;
            g_animReset = true;
          }
          g_playing = true;
        }
        break;
      }

      case LightQueueCmd::Stop:
      {
        g_playing = false;
        g_animReset = true; // ensure clean start next time
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
    if (g_playing)
    {
      if (g_animIndex < NUM_LIGHT_ANIMATIONS)
      {
        // Call the selected animation function.
        kAnims[g_animIndex](g_animReset);
        g_animReset = false;
      }
      else
      {
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
static bool light_hw_init_once()
{
  static bool inited = false;
  if (inited)
    return true;

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(g_leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setBrightness(g_brightness);
  // Optional: color correction to taste
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.show();

  inited = true;
  return true;
}

bool light_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core)
{

  // Initialize the hardware interface
  if (!light_hw_init_once())
    return false;

  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      LightTask, "LightTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}