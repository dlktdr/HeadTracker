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
#include <stdio.h>
#include <kernel.h>
#include "log.h"
#include "serial.h"

// protect the preallocated buffer
K_MUTEX_DEFINE(log_buffer_mutex);
char log_buffer1[LOG_BUFFER1_SIZE];
char log_buffer2[LOG_BUFFER2_SIZE];

log_level global_log_level = DEFAULT_LOG_LEVEL;

char log_level_to_char(log_level level) {
    switch (level) {
        case TRACE: return 't';
        case DEBUG: return 'd';
        case INFO: return 'i';
        case WARN: return 'w';
        case ERROR: return 'e';
        case FATAL: return 'f';
        default: return '?';
    }
}

int ht_serial_logger(const log_level level, ...) {
    int len1 = 0;
    int len2 = 0;

    // level filter -- only render if needed
    if (global_log_level >= level) {
        k_mutex_lock(&log_buffer_mutex, K_FOREVER);

        va_list vArg;
        va_start(vArg, level);
        char *format = va_arg(vArg, char*);

        // generate the prefix for format string
        int now = k_uptime_get_32();
        // TODO remove the 'HT:' when the coloring scheme in GUI can interpret log messages
        len1 = snprintf(log_buffer1, LOG_BUFFER1_SIZE, "HT: %c (%d.%d) %s: %s\r\n",
            log_level_to_char(level),
            now / 1000, now % 1000,
            DEFAULT_LOG_CONTEXT,
            format);

        if (len1 < LOG_BUFFER1_SIZE) {
            len2 = vsnprintf(log_buffer2, LOG_BUFFER2_SIZE, log_buffer1, vArg);

            if (len2 >= LOG_BUFFER2_SIZE) {
                // TODO print overflow
            }
        } else {
            // TODO print overflow
        }
        va_end(vArg);

        serialWrite(log_buffer2, len2);
        k_mutex_unlock(&log_buffer_mutex);
    }
    return len2;
}

void logger_init() {
    k_mutex_init(&log_buffer_mutex);
    LOGI("logger initialized");
}