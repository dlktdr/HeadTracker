#pragma once

#include "defines.h"
#include "ble.h"

void processTrainerByte(uint8_t data);
extern uint16_t BtChannelsIn[BT_CHANNELS];

enum {
  STATE_DATA_IDLE,
  STATE_DATA_START,
  STATE_DATA_XOR,
  STATE_DATA_IN_FRAME
};

#define BLUETOOTH_LINE_LENGTH           32
#define BLUETOOTH_PACKET_SIZE           14
