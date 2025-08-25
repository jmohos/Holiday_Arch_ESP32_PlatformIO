#include "CommandQueues.h"

QueueBus queueBus;

bool QueueBus_Init(QueueBus& bus) {
  // Create queues with configured depths and typed element sizes
  bus.showInputQueueHandle = xQueueCreate(Q_SHOW_INPUT_LEN, sizeof(ShowInputQueueMsg));
  bus.netSendQueueHandle   = xQueueCreate(Q_NET_SEND_LEN,   sizeof(NetSendQueueMsg));
  bus.audioCmdQueueHandle  = xQueueCreate(Q_AUDIO_CMD_LEN,  sizeof(AudioCmdQueueMsg));
  bus.lightCmdQueueHandle  = xQueueCreate(Q_LIGHT_CMD_LEN,  sizeof(LightCmdQueueMsg));
  bus.motorCmdQueueHandle  = xQueueCreate(Q_MOTOR_CMD_LEN,  sizeof(MotorCmdQueueMsg));

  const bool ok = bus.showInputQueueHandle && bus.netSendQueueHandle && bus.audioCmdQueueHandle && bus.lightCmdQueueHandle && bus.motorCmdQueueHandle;

  return ok;
}
