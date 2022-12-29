#include "opentxbt.h"

#include <zephyr.h>


constexpr uint8_t START_STOP = 0x7E;
constexpr uint8_t BYTE_STUFF = 0x7D;
constexpr uint8_t STUFF_MASK = 0x20;

uint8_t otxbuffer[BLUETOOTH_LINE_LENGTH + 1];
uint8_t otxbufferIndex = 0;
uint8_t crc;
uint16_t BtChannelsIn[TrackerSettings::BT_CHANNELS];

/* From OpenTX 2.3.1
 */

void appendTrainerByte(uint8_t data)
{
  if (otxbufferIndex < BLUETOOTH_LINE_LENGTH) {
    otxbuffer[otxbufferIndex++] = data;
    // we check for "DisConnected", but the first byte could be altered (if received in state
    // STATE_DATA_XOR)
    if (data == '\n') {
      otxbufferIndex = 0;
    }
  }
}

void processTrainerFrame(const uint8_t* otxbuffer)
{
  for (uint8_t channel = 0, i = 1; channel < TrackerSettings::BT_CHANNELS; channel += 2, i += 3) {
    // +-500 != 512, but close enough.
    BtChannelsIn[channel] = otxbuffer[i] + ((otxbuffer[i + 1] & 0xf0) << 4);
    BtChannelsIn[channel + 1] = ((otxbuffer[i + 1] & 0x0f) << 4) +
                                ((otxbuffer[i + 2] & 0xf0) >> 4) + ((otxbuffer[i + 2] & 0x0f) << 8);
  }
}

void processTrainerByte(uint8_t data)
{
  static uint8_t dataState = STATE_DATA_IDLE;

  switch (dataState) {
    case STATE_DATA_START:
      if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
      } else {
        appendTrainerByte(data);
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (data == BYTE_STUFF) {
        dataState = STATE_DATA_XOR;  // XOR next byte
      } else if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
      } else {
        appendTrainerByte(data);
      }
      break;

    case STATE_DATA_XOR:
      appendTrainerByte(data ^ STUFF_MASK);
      dataState = STATE_DATA_IN_FRAME;
      break;

    case STATE_DATA_IDLE:
      if (data == START_STOP) {
        otxbufferIndex = 0;
        dataState = STATE_DATA_START;
      } else {
        appendTrainerByte(data);
      }
      break;
  }

  if (otxbufferIndex >= BLUETOOTH_PACKET_SIZE) {
    uint8_t crc = 0x00;
    for (int i = 0; i < 13; i++) {
      crc ^= otxbuffer[i];
    }
    if (crc == otxbuffer[13]) {
      if (otxbuffer[0] == 0x80) {
        processTrainerFrame(otxbuffer);
      }
    }
    dataState = STATE_DATA_IDLE;
  }
}
