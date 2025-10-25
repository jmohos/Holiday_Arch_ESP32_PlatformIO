#include <Arduino.h>
#include "AnimationStep.h"
#include "Audio.h"
#include "CommandQueues.h"
#include "elapsedMillis.h"
#include "Light.h"
#include "main.h"
#include "Motor.h"
#include "Show.h"

#include "IoSync.h"
#include "Logging.h"
#include "Protocol.h"
#include "SettingsStore.h"


// Animation Profiles
const AnimationStep idleShow[] = {
    { 22000, LightAnim::PORTAL_HALLOWEEN, AudioAnim::THERAMIN, MotorAnim::HOME},
    { 30000, LightAnim::FLAMES, AudioAnim::FIRE, MotorAnim::HOME },
};
const uint8_t IDLE_SHOW_LENGTH = sizeof(idleShow) / sizeof(AnimationStep);

const AnimationStep localShow[] = {
    { 6000, LightAnim::LIGHTNING,   AudioAnim::THUNDER,  MotorAnim::JIGGLE },
};
const uint8_t LOCAL_SHOW_LENGTH = sizeof(localShow) / sizeof(AnimationStep);

const AnimationStep remoteShow[] = {
    { 6000, LightAnim::LIGHTNING,    AudioAnim::THUNDER, MotorAnim::HAMMER },
//    { 1000, LightAnim::BOUNCE,    AudioAnim::FIVE,    MotorAnim::HAMMER },
//    { 1000, LightAnim::BOUNCE,    AudioAnim::FOUR,    MotorAnim::HAMMER },
//    { 1000, LightAnim::BOUNCE,    AudioAnim::THREE,   MotorAnim::HAMMER },
//    { 1000, LightAnim::BOUNCE,    AudioAnim::TWO,     MotorAnim::HAMMER },
};
const uint8_t REMOTE_SHOW_LENGTH = sizeof(remoteShow) / sizeof(AnimationStep);

#define START_IDLE_ANIM() do {currentShow = idleShow; currentShowLength = IDLE_SHOW_LENGTH;} while(0)
#define START_LOCAL_ANIM() do {currentShow = localShow; currentShowLength = LOCAL_SHOW_LENGTH;} while(0)
#define START_REMOTE_ANIM() do {currentShow = remoteShow; currentShowLength = REMOTE_SHOW_LENGTH;} while(0)

// Minimum time between local triggers.
#define MIN_LOCAL_TRIGGER_TURNAROUND_MSEC 10 * 1000

// 
bool send_trigger(uint8_t animId) {
  // Trigger message has one data byte after header.
  uint8_t frame[Proto::HDR_SIZE + 1];
  size_t len = Proto::buildTriggerAnim(
      frame,
      sizeof(frame),
      Proto::BROADCAST,                // send to all peers
      settingsConfig.deviceId(),       // my node id as source
      animId);
  if (len == 0) {
    ESP_LOGE("NET", "Failed to pack trigger message!");
    return false;
  }
  ESP_LOGI("NET", "Sending TRIGGER_ANIM %u...", animId);
  int sent = networkService->send(frame, len);
  if (sent == (int)len) return true;
  ESP_LOGE("NET", "Trigger send failed! rc=%d", sent);
  return false;
}



static void ShowTask(void*) {
  ShowStates showState = ShowStates::SHOWSTATE_START_TABLE;
  uint32_t stepStartTime = 0;
  uint8_t currentStep = 0;
  const AnimationStep* currentShow = nullptr;
  uint8_t currentShowLength = 0;
  ShowInputQueueMsg in_msg{};
  elapsedMillis time_since_last_local_trigger = 0; 

  for (;;)
  {

    if (xQueueReceive(queueBus.showInputQueueHandle, &in_msg, 0) == pdPASS) {
      io_printf("Received incoming command: %d, param: %d\n", in_msg.cmd, in_msg.param);

      switch(in_msg.cmd) {
        case ShowInputQueueCmd::TriggerLocal:

          // Check if its been too soon since the last trigger to prevent
          // annoying back to back triggering.
          if (time_since_last_local_trigger >= MIN_LOCAL_TRIGGER_TURNAROUND_MSEC) {
            time_since_last_local_trigger = 0;

            START_LOCAL_ANIM();

            // Send out to peers a remote trigger messsage
            send_trigger(1);

            showState = SHOWSTATE_START_TABLE;
          } else {
            io_printf("Rejecting trigger, too soon!\n");
          }

        break;

        case ShowInputQueueCmd::TriggerPeer:
          START_REMOTE_ANIM();
          showState = SHOWSTATE_START_TABLE;
        break;

        case ShowInputQueueCmd::Start:
          START_IDLE_ANIM();
          showState = SHOWSTATE_START_TABLE;
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

  //
  // The currentShow and currentShowLength must be selected prior to
  // this point, otherwise it will default to the idle show.
  //
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

    case SHOWSTATE_START_TABLE:
      if (!currentShow) {
        START_IDLE_ANIM();
        //io_printf("Show starting idle loop...\n");
      }
      //io_printf("SHOW - START NEW TABLE\n");
      currentStep = 0;
      showState = SHOWSTATE_TABLE_STEP;
    break;

    case SHOWSTATE_TABLE_STEP:
      // Activate all animated elements for this step.
      if (currentShow) {
        stepStartTime = millis();
        SendLightQueue( LightCmdQueueMsg{ LightQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].lightIndex) } );
        SendAudioQueue( AudioCmdQueueMsg{ AudioQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].audioIndex) } );
        SendMotorQueue( MotorCmdQueueMsg{ MotorQueueCmd::Play,  static_cast<uint8_t>(currentShow[currentStep].motorIndex) } );
        showState = SHOWSTATE_STEP_WAIT;
        // io_printf("SHOW - STEP: %d  QUEUED UP LIGHT:%d AUDIO:%d MOTOR:%d\n",
        //   currentStep,
        //   static_cast<uint8_t>(currentShow[currentStep].lightIndex),
        //   static_cast<uint8_t>(currentShow[currentStep].audioIndex),
        //   static_cast<uint8_t>(currentShow[currentStep].motorIndex));
      } else {
        showState = SHOWSTATE_DISABLED;
      }

    break;

    case SHOWSTATE_STEP_WAIT:
      if (currentShow) {
        if (millis() - stepStartTime > currentShow[currentStep].duration_ms) {
          // Done with this line in the animation steps.
          currentStep++;
          if (currentStep >= currentShowLength) {
            // We have completed all rows, return to Idle show.
            START_IDLE_ANIM();
            showState = SHOWSTATE_START_TABLE;
            //io_printf("SHOW - RETURN TO IDLE\n");
          } else {
            // Do next row
            showState = SHOWSTATE_TABLE_STEP;
            //io_printf("SHOW - START NEXT STEP num %d of %d\n", currentStep, currentShowLength);
          }
        }
      } else {
        showState = SHOWSTATE_DISABLED;
      }
    break;

  }

  // 
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


bool show_start(UBaseType_t priority, uint32_t stack_bytes, BaseType_t core) {
  TaskHandle_t h = nullptr;
  BaseType_t ok = xTaskCreatePinnedToCore(
      ShowTask, "ShowTask", stack_bytes, nullptr, priority, &h, core);
  return ok == pdPASS;
}