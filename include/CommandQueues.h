#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// ===== Command enums for each queue type =====
enum class ShowInputQueueCmd  : uint8_t { None=0, Start=1, Stop=2, TriggerLocal=3, TriggerPeer=4 };
enum class AudioQueueCmd : uint8_t { None=0, Play=1, Stop=2, Volume=3 };
enum class LightQueueCmd : uint8_t { None=0, Play=1, Stop=2 };
enum class MotorQueueCmd : uint8_t { None=0, Play=1, Stop=2, Home=3 };

// ===== Message payloads for each queue =====
struct ShowInputQueueMsg  { ShowInputQueueCmd cmd; uint8_t param; }; /* If TriggerPeer, param = peer station # */
struct NetSendQueueMsg    { uint8_t  dest; uint8_t cmd; uint8_t param; };
struct AudioCmdQueueMsg   { AudioQueueCmd cmd; uint8_t param; };
struct LightCmdQueueMsg   { LightQueueCmd cmd; uint8_t param; };
struct MotorCmdQueueMsg   { MotorQueueCmd cmd; uint8_t param; };

// (sanity) validate sizes if you rely on tight wire formats
static_assert(sizeof(ShowInputQueueMsg) == 2, "ShowInputQueueMsg must be 2 bytes");
static_assert(sizeof(AudioCmdQueueMsg)  == 2, "AudioCmdQueueMsg must be 2 bytes");
static_assert(sizeof(LightCmdQueueMsg)  == 2, "LightCmdQueueMsg must be 2 bytes");
static_assert(sizeof(MotorCmdQueueMsg)  == 2, "MotorCmdQueueMsg must be 2 bytes");
static_assert(sizeof(NetSendQueueMsg)   == 3, "NetSendQueueMsg must be 3 bytes");

// ===== Centralized queue bus (global/shared) =====
struct QueueBus {
  QueueHandle_t showInputQueueHandle = nullptr;
  QueueHandle_t netSendQueueHandle   = nullptr;
  QueueHandle_t audioCmdQueueHandle  = nullptr;
  QueueHandle_t lightCmdQueueHandle  = nullptr;
  QueueHandle_t motorCmdQueueHandle  = nullptr;
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
inline bool SendShowQueue (const ShowInputQueueMsg&  m, TickType_t to=0) { return qsend(queueBus.showInputQueueHandle, &m, to); }
inline bool SendNetQueue  (const NetSendQueueMsg&    m, TickType_t to=0) { return qsend(queueBus.netSendQueueHandle,   &m, to); }
inline bool SendAudioQueue(const AudioCmdQueueMsg&   m, TickType_t to=0) { return qsend(queueBus.audioCmdQueueHandle,  &m, to); }
inline bool SendLightQueue(const LightCmdQueueMsg&   m, TickType_t to=0) { return qsend(queueBus.lightCmdQueueHandle,  &m, to); }
inline bool SendMotorQueue(const MotorCmdQueueMsg&   m, TickType_t to=0) { return qsend(queueBus.motorCmdQueueHandle,  &m, to); }

// ===== Typed convenience wrappers (ISR context) =====
inline bool SendShowQueueFromISR (const ShowInputQueueMsg&  m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.showInputQueueHandle, &m, hpw); }
inline bool SendNetQueueFromISR  (const NetSendQueueMsg&    m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.netSendQueueHandle,   &m, hpw); }
inline bool SendAudioQueueFromISR(const AudioCmdQueueMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.audioCmdQueueHandle,  &m, hpw); }
inline bool SendLightQueueFromISR(const LightCmdQueueMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.lightCmdQueueHandle,  &m, hpw); }
inline bool SendMotorQueueFromISR(const MotorCmdQueueMsg&   m, BaseType_t* hpw=nullptr){ return qsend_isr(queueBus.motorCmdQueueHandle,  &m, hpw); }
