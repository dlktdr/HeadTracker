#pragma once

#include "stdint.h"
#include "crsf_protocol.h"
#include <math.h>
#include "map.h"
#include <string.h>

#define PACKED __attribute__((packed))

// Macros for big-endian (assume little endian host for now) etc
#define CRSF_DEC_U16(x) ((uint16_t)__builtin_bswap16(x))
#define CRSF_DEC_I16(x) ((int16_t)CRSF_DEC_U16(x))
#define CRSF_DEC_U24(x) (CRSF_DEC_U32((uint32_t)x << 8))
#define CRSF_DEC_U32(x) ((uint32_t)__builtin_bswap32(x))
#define CRSF_DEC_I32(x) ((int32_t)CRSF_DEC_U32(x))

void CrsfOutInit();

typedef enum
{
    CRSF_UINT8 = 0,
    CRSF_INT8 = 1,
    CRSF_UINT16 = 2,
    CRSF_INT16 = 3,
    CRSF_UINT32 = 4,
    CRSF_INT32 = 5,
    CRSF_UINT64 = 6,
    CRSF_INT64 = 7,
    CRSF_FLOAT = 8,
    CRSF_TEXT_SELECTION = 9,
    CRSF_STRING = 10,
    CRSF_FOLDER = 11,
    CRSF_INFO = 12,
    CRSF_COMMAND = 13,
    CRSF_VTX = 15,
    CRSF_OUT_OF_RANGE = 127,
} crsf_value_type_e;

// Used by extended header frames (type in range 0x28 to 0x96)
typedef struct crsf_ext_header_s
{
    // Common header fields, see crsf_header_t
    uint8_t device_addr;
    uint8_t frame_size;
    uint8_t type;
    // Extended fields
    uint8_t dest_addr;
    uint8_t orig_addr;
} PACKED crsf_ext_header_t;


typedef struct crsf_channels_s crsf_channels_t;

/*
 * 0x14 Link statistics
 * Payload:
 *
 * uint8_t Uplink RSSI Ant. 1 ( dBm * -1 )
 * uint8_t Uplink RSSI Ant. 2 ( dBm * -1 )
 * uint8_t Uplink Package success rate / Link quality ( % )
 * int8_t Uplink SNR ( db )
 * uint8_t Diversity active antenna ( enum ant. 1 = 0, ant. 2 )
 * uint8_t RF Mode ( enum 4fps = 0 , 50fps, 150hz)
 * uint8_t Uplink TX Power ( enum 0mW = 0, 10mW, 25 mW, 100 mW, 500 mW, 1000 mW, 2000mW )
 * uint8_t Downlink RSSI ( dBm * -1 )
 * uint8_t Downlink package success rate / Link quality ( % )
 * int8_t Downlink SNR ( db )
 * Uplink is the connection from the ground to the UAV and downlink the opposite direction.
 */

typedef struct crsfPayloadLinkstatistics_s crsfLinkStatistics_t;

/////inline and utility functions//////

static inline uint16_t US_to_CRSF(uint16_t Val) { return round(fmap(Val, 988.0, 2012.0, 172.0, 1811.0)); };
static inline uint16_t CRSF_to_US(uint16_t Val) { return round(fmap(Val, 172.0, 1811.0, 988.0, 2012.0)); };
static inline uint16_t UINT10_to_CRSF(uint16_t Val) { return round(fmap(Val, 0.0, 1024.0, 172.0, 1811.0)); };

static inline uint16_t SWITCH3b_to_CRSF(uint16_t Val) { return round(map(Val, 0, 7, 188, 1795)); };

static inline uint8_t CRSF_to_BIT(uint16_t Val)
{
    if (Val > 1000)
        return 1;
    else
        return 0;
};
static inline uint16_t BIT_to_CRSF(uint8_t Val)
{
    if (Val)
        return 1795;
    else
        return 188;
};

static inline uint16_t CRSF_to_UINT10(uint16_t Val) { return round(fmap(Val, 172.0, 1811.0, 0.0, 1023.0)); };

class CRSF
{

public:
    static volatile uint16_t ChannelDataIn[16];
    static volatile uint16_t ChannelDataInPrev[16]; // Contains the previous RC channel data
    static volatile uint16_t ChannelDataOut[16];

    static void (*RCdataCallback1)(); //function pointer for new RC data callback
    static void (*RCdataCallback2)(); //function pointer for new RC data callback

    static void (*disconnected)();
    static void (*connected)();

    static void (*RecvParameterUpdate)();

    static volatile uint8_t ParameterUpdateData[2];

    static bool firstboot;

    static bool CRSFstate;
    static bool CRSFstatePrev;

    static uint8_t CSFR_TXpin_Module;
    static uint8_t CSFR_RXpin_Module;

    static uint8_t CSFR_TXpin_Recv;
    static uint8_t CSFR_RXpin_Recv;

    /////Variables/////

    static volatile crsf_channels_s PackedRCdataOut;            // RC data in packed format for output.
    static volatile crsf_attitude_s AttitudeDataOut;
    static volatile crsfPayloadLinkstatistics_s LinkStatistics; // Link Statisitics Stored as Struct

    static void Begin(); //setup timers etc

    static void duplex_set_RX();
    static void duplex_set_TX();
    static void duplex_set_HIGHZ();
    static void GetSYNC(); //SYNC to incomming data

#ifdef PLATFORM_ESP32
    static void ESP32uartTask(void *pvParameters);
#else
    static void ESP8266ReadUart();
#endif

    void sendRCFrameToFC();
    void sendLinkStatisticsToFC();
    void sendLinkStatisticsToTX();
    void sendAttitideToFC();

    //static void BuildRCPacket(crsf_addr_e addr = CRSF_ADDRESS_FLIGHT_CONTROLLER); //build packet to send to the FC

    static void SerialISR();
    static void ProcessPacket();
    static void GetChannelDataIn();

    static void inline nullCallback(void);

    //static uint16_t CRSF_to_US(uint16_t Datain) { return (0.62477120195241f * Datain) + 881; };

private:
    static volatile uint8_t SerialInPacketLen;   // length of the CRSF packet as measured
    static volatile uint8_t SerialInPacketPtr;   // index where we are reading/writing
    static volatile uint8_t SerialInBuffer[100]; // max 64 bytes for CRSF packet serial buffer

    static volatile uint8_t CRSFoutBuffer[CRSF_MAX_PACKET_LEN + 1]; //index 0 hold the length of the datapacket

    static volatile bool ignoreSerialData; //since we get a copy of the serial data use this flag to know when to ignore it
    static volatile bool CRSFframeActive;  //since we get a copy of the serial data use this flag to know when to ignore it
};

extern CRSF crsfout;