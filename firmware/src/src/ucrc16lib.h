/**
 * Tiny and cross-device compatible CCITT CRC16 calculator library.
 *
 * @copyright Naguissa
 * @author Naguissa
 * @email naguissa@foroelectro.net
 * @version 2.0.0
 * @created 2018-04-21
 */
#pragma once

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#define uCRC16Lib_POLYNOMIAL 0x8408

class uCRC16Lib
{
 public:
  static uint16_t calculate(char *, uint16_t);
  const static uint16_t crc_ok = 0x0F47;

 private:
  // Static library, no need to construct objects
  uCRC16Lib();
};
