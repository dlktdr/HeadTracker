/* This code below is from CapnBry https://github.com/CapnBry/CRServoF
 *
 * It was modified for use in HT. Sep 25, 2022
 */

#include "crsfin.h"

#include <string.h>

#include "crsfout.h"
#include "defines.h"
#include "log.h"
#include "uart_mode.h"

#define CRSF_LOG_RATE 500 // 500mS

CrsfSerial *crsfin = nullptr;

static void crsfShiftyByte(uint8_t b)
{
  // LOGI("CRSF, shifty byte %c", b);
}

static void packetChannels() { PacketCount++; }

// V3.0 ELRS
static const char *getELRSMode(uint8_t mode)
{
  switch (mode) {
    case RATE_LORA_4HZ:
      return "LORA 4HZ";
    case RATE_LORA_25HZ:
      return "LORA 25HZ";
    case RATE_LORA_50HZ:
      return "LORA 50HZ";
    case RATE_LORA_100HZ:
      return "LORA 100HZ";
    case RATE_LORA_100HZ_8CH:
      return "LORA 100HZ_8CH";
    case RATE_LORA_150HZ:
      return "LORA 150HZ";
    case RATE_LORA_200HZ:
      return "LORA_200HZ";
    case RATE_LORA_250HZ:
      return "LORA_250HZ";
    case RATE_LORA_333HZ_8CH:
      return "LORA 333HZ_8CH";
    case RATE_LORA_500HZ:
      return "LORA 500HZ";
    case RATE_DVDA_250HZ:
      return "DVDA 250HZ";
    case RATE_DVDA_500HZ:
      return "DVDA 500HZ";
    case RATE_FLRC_500HZ:
      return "FLRC 500HZ";
    case RATE_FLRC_1000HZ:
      return "FLRC 1000HZ";
    default:
      return "UNKNOWN";
  }
}

static void packetLinkStatistics(crsfLinkStatistics_t *link)
{
  static int64_t mmic = millis64() + CRSF_LOG_RATE;
  if (mmic < millis64()) {
    LOGI("CRSF Qlty %d%%, Mode(%d) %s, Rssi %d, SNR %d", link->uplink_Link_quality, link->rf_Mode,
         getELRSMode(link->rf_Mode), link->uplink_RSSI_1, link->uplink_SNR);
    mmic = millis64() + CRSF_LOG_RATE;
  }
}

void crsfLinkUp() {}

void crsfLinkDown() {}

void CrsfInInit()
{
  crsfin = new CrsfSerial;
  crsfin->onLinkUp = &crsfLinkUp;
  crsfin->onLinkDown = &crsfLinkDown;
  crsfin->onShiftyByte = &crsfShiftyByte;
  crsfin->onPacketChannels = &packetChannels;
  crsfin->onPacketLinkStatistics = &packetLinkStatistics;
}

CrsfSerial::CrsfSerial(uint32_t baud) :
    _crc(0xd5),
    _baud(baud),
    _lastReceive(0),
    _lastChannelsPacket(0),
    _linkIsUp(false),
    _passthroughMode(false)
{
  // Crsf serial is 420000 baud for V2
  AuxSerial_Close();
  AuxSerial_Open(_baud, CONF8N1);
}

// Call from main loop to update
void CrsfSerial::loop() { handleSerialIn(); }

void CrsfSerial::handleSerialIn()
{
  while (AuxSerial_Available()) {
    uint8_t b;
    if (AuxSerial_Read(&b, 1) != 1) continue;
    _lastReceive = millis();

    if (_passthroughMode) {
      if (onShiftyByte) onShiftyByte(b);
      continue;
    }

    _rxBuf[_rxBufPos++] = b;
    handleByteReceived();

    if (_rxBufPos == (sizeof(_rxBuf) / sizeof(_rxBuf[0]))) {
      // Packet buffer filled and no valid packet found, dump the whole thing
      _rxBufPos = 0;
    }
  }

  checkPacketTimeout();
  checkLinkDown();
}

void CrsfSerial::handleByteReceived()
{
  bool reprocess;
  do {
    reprocess = false;
    if (_rxBufPos > 1) {
      uint8_t len = _rxBuf[1];
      // Sanity check the declared length, can't be shorter than Type, X, CRC
      if (len < 3 || len > CRSF_MAX_PACKET_LEN) {
        shiftRxBuffer(1);
        reprocess = true;
      }

      else if (_rxBufPos >= (len + 2)) {
        uint8_t inCrc = _rxBuf[2 + len - 1];
        uint8_t crc = _crc.calc(&_rxBuf[2], len - 1);
        if (crc == inCrc) {
          processPacketIn(len);
          shiftRxBuffer(len + 2);
          reprocess = true;
        } else {
          shiftRxBuffer(1);
          reprocess = true;
        }
      }  // if complete packet
    }    // if pos > 1
  } while (reprocess);
}

void CrsfSerial::checkPacketTimeout()
{
  // If we haven't received data in a long time, flush the buffer a byte at a time (to trigger
  // shiftyByte)
  if (_rxBufPos > 0 && millis() - _lastReceive > CRSF_PACKET_TIMEOUT_MS)
    while (_rxBufPos) shiftRxBuffer(1);
}

void CrsfSerial::checkLinkDown()
{
  if (_linkIsUp && millis() - _lastChannelsPacket > CRSF_FAILSAFE_STAGE1_MS) {
    if (onLinkDown) onLinkDown();
    _linkIsUp = false;
  }
}

void CrsfSerial::processPacketIn(uint8_t len)
{
  const crsf_header_t *hdr = (crsf_header_t *)_rxBuf;
  if (hdr->device_addr == CRSF_ADDRESS_FLIGHT_CONTROLLER) {
    switch (hdr->type) {
      case CRSF_FRAMETYPE_GPS:
        packetGps(hdr);
        break;
      case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
        packetChannelsPacked(hdr);
        break;
      case CRSF_FRAMETYPE_LINK_STATISTICS:
        packetLinkStatistics(hdr);
        break;
    }
  }  // CRSF_ADDRESS_FLIGHT_CONTROLLER
}

// Shift the bytes in the RxBuf down by cnt bytes
void CrsfSerial::shiftRxBuffer(uint8_t cnt)
{
  // If removing the whole thing, just set pos to 0
  if (cnt >= _rxBufPos) {
    _rxBufPos = 0;
    return;
  }

  if (cnt == 1 && onShiftyByte) onShiftyByte(_rxBuf[0]);

  // Otherwise do the slow shift down
  uint8_t *src = &_rxBuf[cnt];
  uint8_t *dst = &_rxBuf[0];
  _rxBufPos -= cnt;
  uint8_t left = _rxBufPos;
  while (left--) *dst++ = *src++;
}

void CrsfSerial::packetChannelsPacked(const crsf_header_t *p)
{
  crsf_channels_t *ch = (crsf_channels_t *)&p->data;
  _channels[0] = ch->ch0;
  _channels[1] = ch->ch1;
  _channels[2] = ch->ch2;
  _channels[3] = ch->ch3;
  _channels[4] = ch->ch4;
  _channels[5] = ch->ch5;
  _channels[6] = ch->ch6;
  _channels[7] = ch->ch7;
  _channels[8] = ch->ch8;
  _channels[9] = ch->ch9;
  _channels[10] = ch->ch10;
  _channels[11] = ch->ch11;
  _channels[12] = ch->ch12;
  _channels[13] = ch->ch13;
  _channels[14] = ch->ch14;
  _channels[15] = ch->ch15;

  for (unsigned int i = 0; i < CRSF_NUM_CHANNELS; ++i) {
    _channels[i] = fmap(_channels[i], CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
  }

  if (!_linkIsUp && onLinkUp) onLinkUp();
  _linkIsUp = true;
  _lastChannelsPacket = millis();

  if (onPacketChannels) onPacketChannels();
}

void CrsfSerial::packetLinkStatistics(const crsf_header_t *p)
{
  const crsfLinkStatistics_t *link = (crsfLinkStatistics_t *)p->data;
  memcpy(&_linkStatistics, link, sizeof(_linkStatistics));

  if (onPacketLinkStatistics) onPacketLinkStatistics(&_linkStatistics);
}

void CrsfSerial::packetGps(const crsf_header_t *p)
{
  const crsf_sensor_gps_t *gps = (crsf_sensor_gps_t *)p->data;
  _gpsSensor.latitude = be32toh(gps->latitude);
  _gpsSensor.longitude = be32toh(gps->longitude);
  _gpsSensor.groundspeed = be16toh(gps->groundspeed);
  _gpsSensor.heading = be16toh(gps->heading);
  _gpsSensor.altitude = be16toh(gps->altitude);
  _gpsSensor.satellites = gps->satellites;

  if (onPacketGps) onPacketGps(&_gpsSensor);
}

void CrsfSerial::write(uint8_t b) { AuxSerial_Write(&b, 1); }

void CrsfSerial::write(const uint8_t *buf, size_t len) { AuxSerial_Write(buf, len); }

void CrsfSerial::queuePacket(uint8_t addr, uint8_t type, const void *payload, uint8_t len)
{
  if (!_linkIsUp) return;
  if (_passthroughMode) return;
  if (len > CRSF_MAX_PACKET_LEN) return;

  uint8_t buf[CRSF_MAX_PACKET_LEN + 4];
  buf[0] = addr;
  buf[1] = len + 2;  // type + payload + crc
  buf[2] = type;
  memcpy(&buf[3], payload, len);
  buf[len + 3] = _crc.calc(&buf[2], len + 1);

  // Busywait until the serial port seems free
  // while (millis() - _lastReceive < 2)
  //    loop();
  write(buf, len + 4);
}

void CrsfSerial::setPassthroughMode(bool val, unsigned int baud)
{
  _passthroughMode = val;
  if (baud == 0)
    AuxSerial_Open(_baud, CONF8N1);
  else
    AuxSerial_Open(baud, CONF8N1);
}
