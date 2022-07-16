/**
 * Tiny and cross-device compatible CCITT CRC16 calculator library - uCRC16Lib
 *
 * @copyright Naguissa
 * @author Naguissa
 * @email naguissa@foroelectro.net
 * @version 2.0.0
 * @created 2018-04-21
 */
#include "ucrc16lib.h"

/**
 * Constructor
 *
 * Nothing to do here
 */
uCRC16Lib::uCRC16Lib() {}

/**
 * Calculate CRC16 function
 *
 * @param	data_p	*char	Pointer to data
 * @param	length	uint16_t	Length, in bytes, of data to calculate CRC16 of. Should be the
 * same or inferior to data pointer's length.
 */
uint16_t uCRC16Lib::calculate(char *data_p, uint16_t length)
{
  uint8_t i;
  uint16_t data;
  uint16_t crc = 0xffff;

  if (length == 0) {
    return (~crc);
  }

  do {
    for (i = 0, data = (uint16_t)0xff & *data_p++; i < 8; i++, data >>= 1) {
      if ((crc & 0x0001) ^ (data & 0x0001)) {
        crc = (crc >> 1) ^ uCRC16Lib_POLYNOMIAL;
      } else {
        crc >>= 1;
      }
    }
  } while (--length);
  crc = ~crc;
  // Byte swap only needed in certain cases (i.e.: line transmission), so don't perform it.
  // data = crc;
  // crc = (crc << 8) | (data >> 8 & 0xFF);
  return (crc);
}
