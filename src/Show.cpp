#include <Arduino.h>
#include "AnimationStep.h"
#include "Audio.h"
#include "CommandQueues.h"
#include "Light.h"
#include "Motor.h"
#include "Show.h"

#include "IoSync.h"
#include "Logging.h"

// AnimationStep x[] = {
//   { 1000, LightAnim::CandyCane, 0,0 },
//   { 1000, LightAnim::Flames, 0,0 }
// };


const AnimationStep idleShow[] = {
    { 10000, LightAnim::CandyCane, AudioAnim::Silence, MotorAnim::Still },
    { 5000, LightAnim::Flames,    AudioAnim::Bells,   MotorAnim::Jiggle }
};

const uint8_t IDLE_SHOW_LENGTH = sizeof(idleShow) / sizeof(AnimationStep);




static void ShowTask(void*) {
  ShowStates showState = ShowStates::SHOWSTATE_IDLE;
  uint32_t stepStartTime = 0;
  uint8_t currentStep = 0;
  const AnimationStep* currentShow = nullptr;
  uint8_t currentShowLength = 0;
  ShowInputQueueMsg in_msg{};

  for (;;)
  {

    if (xQueueReceive(queueBus.showInputQueueHandle, &in_msg, 0) == pdPASS) {
      io_printf("Received incoming command: %d, param: %d\n", in_msg.cmd, in_msg.param);

      switch(in_msg.cmd) {
        case ShowInputQueueCmd::TriggerLocal:
          SendLightQueue( LightCmdQueueMsg{ LightQueueCmd::Play, 1 } );
          SendAudioQueue( AudioCmdQueueMsg { AudioQueueCmd::Play, 1 });
        break;

        case ShowInputQueueCmd::TriggerPeer:
          SendLightQueue( LightCmdQueueMsg{ LightQueueCmd::Play, 2 } );
          SendAudioQueue( AudioCmdQueueMsg { AudioQueueCmd::Play, 30 });
        break;

        case ShowInputQueueCmd::Start:
          showState = ShowStates::SHOWSTATE_IDLE;
          io_printf("Starting show in idle...\n");
        break;

        case ShowInputQueueCmd::Stop:
          showState = ShowStates::SHOWSTATE_START_DISABLE;
          io_printf("Stopping show...\n");
        break;

        default:
        // Unsupported command!
        break;
      }
    }

  switch(showState) {
    case SHOWSTATE_START_DISABLE:
      currentShow = nullptr;
      // Disable the lights

      // Disable the audio

      // Disable the motor
      showState = SHOWSTATE_DISABLED;
    break;

    case SHOWSTATE_DISABLED:
      // Wait to be reenabled.
    break;

    case SHOWSTATE_IDLE:
      // Load the idle show for animation.
      currentShow = idleShow;
      currentShowLength = IDLE_SHOW_LENGTH;
      currentStep = 0;
      io_printf("Show starting idle loop...\n");
      showState = SHOWSTATE_IDLE_STEP;
    break;

    case SHOWSTATE_IDLE_STEP:
      // Activate all animated elements for this step.
      if (currentShow) {
        stepStartTime = millis();
        SendLightQueue( LightCmdQueueMsg{ LightQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].lightIndex) } );
        SendAudioQueue( AudioCmdQueueMsg{ AudioQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].audioIndex) } );
        SendMotorQueue( MotorCmdQueueMsg{ MotorQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].motorIndex) } );
        showState = SHOWSTATE_IDLE_WAIT;
      } else {
        showState = SHOWSTATE_DISABLED;
      }

    break;

    case SHOWSTATE_IDLE_WAIT:
      if (currentShow) {
        if (millis() - stepStartTime > currentShow[currentStep].duration_ms) {
          // Done with this line in the animation steps.
          currentStep++;
          if (currentStep >= currentShowLength) {
            // We have completed all rows, start again.
            currentShow = nullptr;
            showState = SHOWSTATE_IDLE;
          } else {
            // Do next row
            showState = SHOWSTATE_IDLE_STEP;
          }
        }
      } else {
        showState = SHOWSTATE_DISABLED;
      }
    break;

  }

  // 







    //ESP_LOGD(TAG_SHOW, "[core %d] doing periodic work...", core());
    //Serial.println("Show tick...");
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


bool show_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ShowTask, "ShowTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}