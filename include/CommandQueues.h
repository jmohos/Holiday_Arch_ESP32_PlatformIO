#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ===== Command enums for each queue type =====
enum class ShowCmd  : uint8_t { None=0, Start=1, Stop=2, Trigger=3 };
enum class AudioCmd : uint8_t { None=0, Play=1, Stop=2, Volume=3 };
enum class LightCmd : uint8_t { None=0, Play=1, Stop=2 };
enum class MotorCmd : uint8_t { None=0, Play=1, Stop=2, Home=3 };

// ===== Message payloads for each queue =====
struct ShowInputMsg  { ShowCmd  cmd; uint8_t param; };
struct NetSendMsg    { uint8_t  dest; uint8_t cmd; uint8_t param; };
struct AudioCmdMsg   { AudioCmd cmd; uint8_t param; };
struct LightCmdMsg   { LightCmd cmd; uint8_t param; };
struct MotorCmdMsg   { MotorCmd cmd; uint8_t param; };

// (sanity) validate sizes if you rely on tight wire formats
static_assert(sizeof(ShowInputMsg) == 2, "ShowInputMsg must be 2 bytes");
static_assert(sizeof(AudioCmdMsg)  == 2, "AudioCmdMsg must be 2 bytes");
static_assert(sizeof(LightCmdMsg)  == 2, "LightCmdMsg must be 2 bytes");
static_assert(sizeof(MotorCmdMsg)  == 2, "MotorCmdMsg must be 2 bytes");
static_assert(sizeof(NetSendMsg)   == 3, "NetSendMsg must be 3 bytes");

// ===== Centralized queue bus (global/shared) =====
struct QueueBus {
  QueueHandle_t showInput = nullptr;
  QueueHandle_t netSend   = nullptr;
  QueueHandle_t audioCmd  = nullptr;
  QueueHandle_t lightCmd  = nullptr;
  QueueHandle_t motorCmd  = nullptr;
};

// Global instance (defined in CommandQueues.cpp)
extern QueueBus queueBus;

// ===== Configuration (tune lengths as needed) =====
#ifndef Q_SHOW_INPUT_LEN
#define Q_SHOW_INPUT_LEN 8
#endif
#ifndef Q_NET_SEND_LEN
#define Q_NET_SEND_LEN   8
#endif
#ifndef Q_AUDIO_CMD_LEN
#define Q_AUDIO_CMD_LEN  8
#endif
#ifndef Q_LIGHT_CMD_LEN
#define Q_LIGHT_CMD_LEN  8
#endif
#ifndef Q_MOTOR_CMD_LEN
#define Q_MOTOR_CMD_LEN  8
#endif

// ===== Lifecycle =====
// Create all queues (returns true if all succeed)
bool QueueBus_Init(QueueBus& bus);

// ===== Generic helpers =====
inline bool qsend(QueueHandle_t q, const void* item, TickType_t to=0) {
  return xQueueSend(q, item, to) == pdPASS;
}
inline bool qrecv(QueueHandle_t q, void* item, TickType_t to=portMAX_DELAY) {
  return xQueueReceive(q, item, to) == pdPASS;
}
inline bool qpeek(QueueHandle_t q, void* item, TickType_t to=0) {
  return xQueuePeek(q, item, to) == pdPASS;
}
inline bool qsend_isr(QueueHandle_t q, const void* item, BaseType_t* hpw=nullptr) {
  return xQueueSendFromISR(q, item, hpw) == pdPASS;
}

// ===== Typed convenience wrappers (task context) =====
inline bool SendShow (const ShowInputMsg&  m, TickType_t to=0) { return qsend(queueBus.showInput, &m, to); }
inline bool SendNet  (const NetSendMsg&    m, TickType_t to=0) { return qsend(queueBus.netSend,   &m, to); }
inline bool SendAudio(const AudioCmdMsg&   m, TickType_t to=0) { return qsend(queueBus.audioCmd,  &m, to); }
inline bool SendLight(const LightCmdMsg&   m, TickType_t to=0) { return qsend(queueBus.lightCmd,  &m, to); }
inline bool SendMotor(const MotorCmdMsg&   m, TickType_t to=0) { return qsend(queueBus.motorCmd,  &m, to); }

// ===== Typed convenience wrappers (ISR context) =====
inline bool SendShowFromISR (const ShowInputMsg&  m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.showInput, &m, hpw); }
inline bool SendNetFromISR  (const NetSendMsg&    m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.netSend,   &m, hpw); }
inline bool SendAudioFromISR(const AudioCmdMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.audioCmd,  &m, hpw); }
inline bool SendLightFromISR(const LightCmdMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.lightCmd,  &m, hpw); }
inline bool SendMotorFromISR(const MotorCmdMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.motorCmd,  &m, hpw); }
