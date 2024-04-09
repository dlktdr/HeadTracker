#include "hexprint.h"

char *bytesToHex(const uint8_t *data, int len, char *buffer)
{
  char *bufptr = buffer;
  for (int i = 0; i < len; i++) {
    uint8_t nib1 = (*data >> 4) & 0x0F;
    uint8_t nib2 = *data++ & 0x0F;
    if (nib1 > 9)
      *bufptr++ = ((char)('A' + nib1 - 10));
    else
      *bufptr++ = ((char)('0' + nib1));
    if (nib2 > 9)
      *bufptr++ = ((char)('A' + nib2 - 10));
    else
      *bufptr++ = ((char)('0' + nib2));
  }
  *bufptr = 0;
  return buffer;
}