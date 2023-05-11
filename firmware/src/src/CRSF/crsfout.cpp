#include "crsfout.h"
#include "auxserial.h"
#include "crc8.h"
#include "trackersettings.h"

CRSF crsfout;

void CrsfOutInit()
{
  crsfout.Begin();
}

volatile bool CRSF::ignoreSerialData = false;
volatile bool CRSF::CRSFframeActive = false; //since we get a copy of the serial data use this flag to know when to ignore it

void inline CRSF::nullCallback(void) {};

void (*CRSF::RCdataCallback1)() = &nullCallback; // function is called whenever there is new RC data.
void (*CRSF::RCdataCallback2)() = &nullCallback; // function is called whenever there is new RC data.

void (*CRSF::disconnected)() = &nullCallback; // called when CRSF stream is lost
void (*CRSF::connected)() = &nullCallback;    // called when CRSF stream is regained

void (*CRSF::RecvParameterUpdate)() = &nullCallback; // called when recv parameter update req, ie from LUA

bool CRSF::firstboot = true;

bool CRSF::CRSFstate = false;

volatile uint8_t CRSF::SerialInPacketLen = 0;                        // length of the CRSF packet as measured
volatile uint8_t CRSF::SerialInPacketPtr = 0;                        // index where we are reading/writing
volatile uint8_t CRSF::SerialInBuffer[100] = {0};                    // max 64 bytes for CRSF packet
volatile uint8_t CRSF::CRSFoutBuffer[CRSF_MAX_PACKET_LEN + 1] = {0}; // max 64 bytes for CRSF packet
volatile uint16_t CRSF::ChannelDataIn[16] = {0};
volatile uint16_t CRSF::ChannelDataInPrev[16] = {0};

volatile uint8_t CRSF::ParameterUpdateData[2] = {0};

volatile crsf_channels_s CRSF::PackedRCdataOut;
volatile crsf_attitude_s CRSF::AttitudeDataOut;
volatile crsfPayloadLinkstatistics_s CRSF::LinkStatistics;

void CRSF::Begin()
{
  AuxSerial_Close();
  AuxSerial_Open(BAUD400000, CONF8N1);
}

void CRSF::sendLinkStatisticsToFC()
{
  uint8_t outBuffer[LinkStatisticsFrameLength + 4] = {0};

  outBuffer[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  outBuffer[1] = LinkStatisticsFrameLength + 2;
  outBuffer[2] = CRSF_FRAMETYPE_LINK_STATISTICS;

  memcpy(outBuffer + 3, (void *)&LinkStatistics, LinkStatisticsFrameLength);

  Crc8 _crc(0xd5);
  uint8_t crc = _crc.calc(&outBuffer[2], LinkStatisticsFrameLength + 1);

  outBuffer[LinkStatisticsFrameLength + 3] = crc;

  AuxSerial_Write(outBuffer, LinkStatisticsFrameLength + 4);
}

void CRSF::sendRCFrameToFC()
{
  // Fix me properly, also merge CRSF better one day.

  // Check the inversion status of the TX pin
  static bool crsfoutinv = false;
  if (crsfoutinv != trkset.getCrsfTxInv()) {
    crsfoutinv = trkset.getCrsfTxInv();
    // Close and re-open port with new settings
    AuxSerial_Close();
    uint8_t inversion = 0;
    if (crsfoutinv) inversion |= CONFINV_TX;
    AuxSerial_Open(BAUD400000, CONF8N1, inversion);
  }

  uint8_t outBuffer[RCframeLength + 4] = {0};

  outBuffer[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  outBuffer[1] = RCframeLength + 2;
  outBuffer[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;

  memcpy(outBuffer + 3, (void *)&PackedRCdataOut, RCframeLength);
  Crc8 _crc(CRSF_CRC_POLY);
  uint8_t crc = _crc.calc(&outBuffer[2], RCframeLength + 1);

  outBuffer[RCframeLength + 3] = crc;

  AuxSerial_Write(outBuffer, RCframeLength + 4);
}

void CRSF::sendAttitideToFC()
{
  uint8_t outBuffer[CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE + 4] = {0};

  outBuffer[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER; // ??
  outBuffer[1] = CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE + 2;
  outBuffer[2] = CRSF_FRAMETYPE_ATTITUDE;

  memcpy(outBuffer + 3, (void *)&PackedRCdataOut, CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE);
  Crc8 _crc(CRSF_CRC_POLY);
  uint8_t crc = _crc.calc(&outBuffer[2], CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE + 1);

  outBuffer[CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE + 3] = crc;

  AuxSerial_Write(outBuffer, CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE + 4);
}
