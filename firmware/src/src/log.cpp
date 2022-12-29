/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "log.h"

#include <kernel.h>
#include <stdio.h>

#include "serial.h"

// protect the preallocated buffer
K_MUTEX_DEFINE(log_buffer_mutex);
char log_buffer1[LOG_BUFFER1_SIZE];
char log_buffer2[LOG_BUFFER2_SIZE];

log_level global_log_level = DEFAULT_LOG_LEVEL;

char log_level_to_char(log_level level)
{
  switch (level) {
    case TRACE:
      return 't';
    case DEBUG:
      return 'd';
    case INFO:
      return 'i';
    case WARN:
      return 'w';
    case ERROR:
      return 'e';
    case FATAL:
      return 'f';
    default:
      return '?';
  }
}

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

int ht_serial_logger(const log_level level, ...)
{
  int len1 = 0;
  int len2 = 0;

  // level filter -- only render if needed
  if (global_log_level <= level) {
    // Don't allow hang in logging, skip message
    if (k_mutex_lock(&log_buffer_mutex, K_MSEC(MAX_MUTEX_WAIT_DELAY)) == -EAGAIN) return 0;

    va_list vArg;
    va_start(vArg, level);
    char *format = va_arg(vArg, char *);

    // generate the prefix for format string
    int now = k_uptime_get_32();
    // TODO remove the 'HT:' when the coloring scheme in GUI can interpret log messages
    len1 = snprintf(log_buffer1, LOG_BUFFER1_SIZE, "HT: %c (%d.%d) %s: %s\r\n",
                    log_level_to_char(level), now / 1000, now % 1000, DEFAULT_LOG_CONTEXT, format);

    if (len1 < LOG_BUFFER1_SIZE) {
      len2 = vsnprintf(log_buffer2, LOG_BUFFER2_SIZE, log_buffer1, vArg);

      if (len2 >= LOG_BUFFER2_SIZE) {
        // formatted string too long -- truncate
        log_buffer2[LOG_BUFFER2_SIZE - 2] = '>';
        log_buffer2[LOG_BUFFER2_SIZE - 1] = 0;
        len2 = LOG_BUFFER2_SIZE;
      }
    } else {
      // syntheszed log formatter string too long -- we're stuck
      len2 = snprintf(log_buffer2, LOG_BUFFER2_SIZE,
                      "ERROR: log formatter line too long [%d] for buffer [%d]\r\n", len1,
                      LOG_BUFFER1_SIZE);
    }
    va_end(vArg);

    serialWrite(log_buffer2, len2);
    k_mutex_unlock(&log_buffer_mutex);
  }
  return len2;
}

void logger_init()
{
  k_mutex_init(&log_buffer_mutex);
  LOGI("logger initialized");
}