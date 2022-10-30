#pragma once

#include <stddef.h>

#include "crc8.h"
#include "auxserial.h"
#include "crsf_protocol.h"
#include "map.h"

enum eFailsafeAction { fsaNoPulses, fsaHold };


class CrsfSerial
{
public:
    // Packet timeout where buffer is flushed if no data is received in this time
    static const unsigned int CRSF_PACKET_TIMEOUT_MS = 100;
    static const unsigned int CRSF_FAILSAFE_STAGE1_MS = 300;

    CrsfSerial(uint32_t baud = CRSF_BAUDRATE);
    void loop();
    void write(uint8_t b);
    void write(const uint8_t *buf, size_t len);
    void queuePacket(uint8_t addr, uint8_t type, const void *payload, uint8_t len);

    // Return current channel value (1-based) in us
    int getChannel(unsigned int ch) const { return _channels[ch - 1]; }
    const crsfLinkStatistics_t *getLinkStatistics() const { return &_linkStatistics; }
    const crsf_sensor_gps_t *getGpsSensor() const { return &_gpsSensor; }
    bool isLinkUp() const { return _linkIsUp; }
    bool getPassthroughMode() const { return _passthroughMode; }
    void setPassthroughMode(bool val, unsigned int baud = 0);

    // Event Handlers
    void (*onLinkUp)();
    void (*onLinkDown)();
    void (*onPacketChannels)();
    void (*onShiftyByte)(uint8_t b);
    void (*onPacketLinkStatistics)(crsfLinkStatistics_t *ls);
    void (*onPacketGps)(crsf_sensor_gps_t *gpsSensor);

private:
    uint8_t _rxBuf[CRSF_MAX_PACKET_LEN+3];
    uint8_t _rxBufPos;
    Crc8 _crc;
    crsfLinkStatistics_t _linkStatistics;
    crsf_sensor_gps_t _gpsSensor;
    uint32_t _baud;
    uint32_t _lastReceive;
    uint32_t _lastChannelsPacket;
    bool _linkIsUp;
    bool _passthroughMode;
    int _channels[CRSF_NUM_CHANNELS];

    void handleSerialIn();
    void handleByteReceived();
    void shiftRxBuffer(uint8_t cnt);
    void processPacketIn(uint8_t len);
    void checkPacketTimeout();
    void checkLinkDown();

    // Packet Handlers
    void packetChannelsPacked(const crsf_header_t *p);
    void packetLinkStatistics(const crsf_header_t *p);
    void packetGps(const crsf_header_t *p);
};

void CrsfInInit();

extern CrsfSerial *crsfin;

