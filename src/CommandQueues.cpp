#include "CommandQueues.h"

QueueBus queueBus;

bool QueueBus_Init(QueueBus& bus) {
  // Create queues with configured depths and typed element sizes
  bus.showInput = xQueueCreate(Q_SHOW_INPUT_LEN, sizeof(ShowInputMsg));
  bus.netSend   = xQueueCreate(Q_NET_SEND_LEN,   sizeof(NetSendMsg));
  bus.audioCmd  = xQueueCreate(Q_AUDIO_CMD_LEN,  sizeof(AudioCmdMsg));
  bus.lightCmd  = xQueueCreate(Q_LIGHT_CMD_LEN,  sizeof(LightCmdMsg));
  bus.motorCmd  = xQueueCreate(Q_MOTOR_CMD_LEN,  sizeof(MotorCmdMsg));

  const bool ok = bus.showInput && bus.netSend && bus.audioCmd && bus.lightCmd && bus.motorCmd;

  return ok;
}
