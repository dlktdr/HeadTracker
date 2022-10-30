#include "crsfout.h"
#include "auxserial.h"
#include "crc8.h"

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
volatile crsfPayloadLinkstatistics_s CRSF::LinkStatistics;

void CRSF::Begin()
{
  AuxSerial_Close();
  AuxSerial_Open(BAUD420000, CONF8N1);
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
  uint8_t outBuffer[RCframeLength + 4] = {0};

  outBuffer[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
  outBuffer[1] = RCframeLength + 2;
  outBuffer[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;

  memcpy(outBuffer + 3, (void *)&PackedRCdataOut, RCframeLength);
  Crc8 _crc(0xd5);
  uint8_t crc = _crc.calc(&outBuffer[2], RCframeLength + 1);

  outBuffer[RCframeLength + 3] = crc;

  AuxSerial_Write(outBuffer, RCframeLength + 4);
}
