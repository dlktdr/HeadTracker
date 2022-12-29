#pragma once

#include <stdint.h>

#define PACKED __attribute__((packed))

#define CRSF_CRC_POLY 0xd5

#define CRSF_BAUDRATE BAUD420000

#define CRSF_NUM_CHANNELS 16
#define CRSF_CHANNEL_VALUE_MIN  172 // 987us - actual CRSF min is 0 with E.Limits on
#define CRSF_CHANNEL_VALUE_1000 191
#define CRSF_CHANNEL_VALUE_MID  992
#define CRSF_CHANNEL_VALUE_2000 1792
#define CRSF_CHANNEL_VALUE_MAX  1811 // 2012us - actual CRSF max is 1984 with E.Limits on
#define CRSF_MAX_PACKET_LEN 64

#define CRSF_SYNC_BYTE 0xC8

#define RCframeLength 22             // length of the RC data packed bytes frame. 16 channels in 11 bits each.
#define LinkStatisticsFrameLength 10 //
#define OpenTXsyncFrameLength 11     //
#define BattSensorFrameLength 8      //
#define VTXcontrolFrameLength 12     //

#define CRSF_PAYLOAD_SIZE_MAX 62
#define CRSF_FRAME_NOT_COUNTED_BYTES 2
#define CRSF_FRAME_SIZE(payload_size) ((payload_size) + 2) // See crsf_header_t.frame_size
#define CRSF_EXT_FRAME_SIZE(payload_size) (CRSF_FRAME_SIZE(payload_size) + 2)
#define CRSF_FRAME_SIZE_MAX (CRSF_PAYLOAD_SIZE_MAX + CRSF_FRAME_NOT_COUNTED_BYTES)
#define CRSF_FRAME_CRC_SIZE 1
#define CRSF_FRAME_LENGTH_EXT_TYPE_CRC 4 // length of Extended Dest/Origin, TYPE and CRC fields combined

#define CRSF_TELEMETRY_LENGTH_INDEX 1
#define CRSF_TELEMETRY_TYPE_INDEX 2
#define CRSF_TELEMETRY_FIELD_ID_INDEX 5
#define CRSF_TELEMETRY_FIELD_CHUNK_INDEX 6
#define CRSF_TELEMETRY_CRC_LENGTH 1
#define CRSF_TELEMETRY_TOTAL_SIZE(x) (x + CRSF_FRAME_LENGTH_EXT_TYPE_CRC)

#define AUX1 4
#define AUX2 5
#define AUX3 6
#define AUX4 7
#define AUX5 8
#define AUX6 9
#define AUX7 10
#define AUX8 11
#define AUX9 12
#define AUX10 13
#define AUX11 14
#define AUX12 15

//////////////////////////////////////////////////////////////

#define CRSF_MSP_REQ_PAYLOAD_SIZE 8
#define CRSF_MSP_RESP_PAYLOAD_SIZE 58
#define CRSF_MSP_MAX_PAYLOAD_SIZE (CRSF_MSP_REQ_PAYLOAD_SIZE > CRSF_MSP_RESP_PAYLOAD_SIZE ? CRSF_MSP_REQ_PAYLOAD_SIZE : CRSF_MSP_RESP_PAYLOAD_SIZE)

enum {
  CRSF_FRAME_LENGTH_ADDRESS = 1,      // length of ADDRESS field
  CRSF_FRAME_LENGTH_FRAMELENGTH = 1,  // length of FRAMELENGTH field
  CRSF_FRAME_LENGTH_TYPE = 1,         // length of TYPE field
  CRSF_FRAME_LENGTH_CRC = 1,          // length of CRC field
  CRSF_FRAME_LENGTH_TYPE_CRC = 2,     // length of TYPE and CRC fields combined
  CRSF_FRAME_LENGTH_NON_PAYLOAD = 4,  // combined length of all fields except payload
};

enum {
    CRSF_FRAME_TX_MSP_FRAME_SIZE = 58,
    CRSF_FRAME_RX_MSP_FRAME_SIZE = 8,
    CRSF_FRAME_ORIGIN_DEST_SIZE = 2,
};

typedef enum {
    CRSF_FRAMETYPE_GPS = 0x02,
    CRSF_FRAMETYPE_BATTERY_SENSOR = 0x08,
    CRSF_FRAMETYPE_HEARTBEAT = 0x0B,
    CRSF_FRAMETYPE_LINK_STATISTICS = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
    CRSF_FRAMETYPE_SUBSET_RC_CHANNELS_PACKED = 0x17,
    CRSF_FRAMETYPE_LINK_STATISTICS_RX = 0x1C,
    CRSF_FRAMETYPE_LINK_STATISTICS_TX = 0x1D,
    CRSF_FRAMETYPE_ATTITUDE = 0x1E,
    CRSF_FRAMETYPE_FLIGHT_MODE = 0x21,
    // Extended Header Frames, range: 0x28 to 0x96
    CRSF_FRAMETYPE_DEVICE_PING = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO = 0x29,
    CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY = 0x2B,
    CRSF_FRAMETYPE_PARAMETER_READ = 0x2C,
    CRSF_FRAMETYPE_PARAMETER_WRITE = 0x2D,
    CRSF_FRAMETYPE_COMMAND = 0x32,
    // MSP commands
    CRSF_FRAMETYPE_MSP_REQ = 0x7A,   // response request using msp sequence as command
    CRSF_FRAMETYPE_MSP_RESP = 0x7B,  // reply with 58 byte chunked binary
    CRSF_FRAMETYPE_MSP_WRITE = 0x7C,  // write with 8 byte chunked binary (OpenTX outbound telemetry buffer limit)
    CRSF_FRAMETYPE_DISPLAYPORT_CMD = 0x7D, // displayport control command
} crsfFrameType_e;

enum {
    CRSF_FRAME_GPS_PAYLOAD_SIZE = 15,
    CRSF_FRAME_BATTERY_SENSOR_PAYLOAD_SIZE = 8,
    CRSF_FRAME_HEARTBEAT_PAYLOAD_SIZE = 2,
    CRSF_FRAME_LINK_STATISTICS_PAYLOAD_SIZE = 10,
    CRSF_FRAME_LINK_STATISTICS_TX_PAYLOAD_SIZE = 6,
    CRSF_FRAME_RC_CHANNELS_PAYLOAD_SIZE = 22, // 11 bits per channel * 16 channels = 22 bytes.
    CRSF_FRAME_ATTITUDE_PAYLOAD_SIZE = 6,
};

typedef enum
{
    CRSF_ADDRESS_BROADCAST = 0x00,
    CRSF_ADDRESS_USB = 0x10,
    CRSF_ADDRESS_TBS_CORE_PNP_PRO = 0x80,
    CRSF_ADDRESS_RESERVED1 = 0x8A,
    CRSF_ADDRESS_CURRENT_SENSOR = 0xC0,
    CRSF_ADDRESS_GPS = 0xC2,
    CRSF_ADDRESS_TBS_BLACKBOX = 0xC4,
    CRSF_ADDRESS_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDRESS_RESERVED2 = 0xCA,
    CRSF_ADDRESS_RACE_TAG = 0xCC,
    CRSF_ADDRESS_RADIO_TRANSMITTER = 0xEA,
    CRSF_ADDRESS_CRSF_RECEIVER = 0xEC,
    CRSF_ADDRESS_CRSF_TRANSMITTER = 0xEE,
    CRSF_ADDRESS_ELRS_LUA = 0xEF
} crsf_addr_e;

// Express LRS 3.0
typedef enum : uint8_t
{
    RATE_LORA_4HZ = 0,
    RATE_LORA_25HZ,
    RATE_LORA_50HZ,
    RATE_LORA_100HZ,
    RATE_LORA_100HZ_8CH,
    RATE_LORA_150HZ,
    RATE_LORA_200HZ,
    RATE_LORA_250HZ,
    RATE_LORA_333HZ_8CH,
    RATE_LORA_500HZ,
    RATE_DVDA_250HZ,
    RATE_DVDA_500HZ,
    RATE_FLRC_500HZ,
    RATE_FLRC_1000HZ,
} expresslrs_RFrates_e;

typedef struct crsf_header_s {
  uint8_t device_addr;  // from crsf_addr_e
  uint8_t
      frame_size;  // counts size after this byte, so it must be the payload size + 2 (type and crc)
  uint8_t type;    // from crsf_frame_type_e
  uint8_t data[0];
} PACKED crsf_header_t;

typedef struct crsf_channels_s {
  unsigned ch0 : 11;
  unsigned ch1 : 11;
  unsigned ch2 : 11;
  unsigned ch3 : 11;
  unsigned ch4 : 11;
  unsigned ch5 : 11;
  unsigned ch6 : 11;
  unsigned ch7 : 11;
  unsigned ch8 : 11;
  unsigned ch9 : 11;
  unsigned ch10 : 11;
  unsigned ch11 : 11;
  unsigned ch12 : 11;
  unsigned ch13 : 11;
  unsigned ch14 : 11;
  unsigned ch15 : 11;
} PACKED crsf_channels_t;

typedef struct crsf_attitude_s {
  uint16_t pitch;
  uint16_t roll;
  uint16_t yaw;
} crsf_attitude_t;


typedef struct crsfPayloadLinkstatistics_s {
  uint8_t uplink_RSSI_1;
  uint8_t uplink_RSSI_2;
  uint8_t uplink_Link_quality;
  int8_t uplink_SNR;
  uint8_t active_antenna;
  uint8_t rf_Mode;
  uint8_t uplink_TX_Power;
  uint8_t downlink_RSSI;
  uint8_t downlink_Link_quality;
  int8_t downlink_SNR;
} crsfLinkStatistics_t;

typedef struct crsf_sensor_battery_s {
  unsigned voltage : 16;   // V * 10 big endian
  unsigned current : 16;   // A * 10 big endian
  unsigned capacity : 24;  // mah big endian
  unsigned remaining : 8;  // %
} PACKED crsf_sensor_battery_t;

typedef struct crsf_sensor_gps_s {
  int32_t latitude;      // degree / 10,000,000 big endian
  int32_t longitude;     // degree / 10,000,000 big endian
  uint16_t groundspeed;  // km/h / 10 big endian
  uint16_t heading;      // GPS heading, degree/100 big endian
  uint16_t altitude;     // meters, +1000m big endian
  uint8_t satellites;    // satellites
} PACKED crsf_sensor_gps_t;

#if !defined(__linux__)
static inline uint16_t htobe16(uint16_t val)
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return val;
#else
  return __builtin_bswap16(val);
#endif
}

static inline uint16_t be16toh(uint16_t val)
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return val;
#else
  return __builtin_bswap16(val);
#endif
}

static inline uint32_t htobe32(uint32_t val)
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return val;
#else
  return __builtin_bswap32(val);
#endif
}

static inline uint32_t be32toh(uint32_t val)
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return val;
#else
  return __builtin_bswap32(val);
#endif
}
#endif