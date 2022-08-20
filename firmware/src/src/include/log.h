#ifndef LOGGIT_H
#define LOGGIT_H

// TODO transition this to local logger -- common to both modules
#include <logging/log.h>
#include <usb/usb_device.h>

#include "defines.h"

LOG_MODULE_DECLARE(cdc_acm_composite, LOG_LEVEL_ERR);

typedef enum { ALL = 0, TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF = 100 } log_level;

extern log_level global_log_level;

void logger_init();

// TODO implement per module
#define DEFAULT_LOG_CONTEXT "main"
#define LOG_BUFFER1_SIZE 400
#define LOG_BUFFER2_SIZE 2000
#define MAX_MUTEX_WAIT_DELAY 10 // (ms)
#define LOGE(...) HT_LOG(ERROR, __VA_ARGS__)
#define LOGW(...) HT_LOG(WARN, __VA_ARGS__)
#define LOGI(...) HT_LOG(INFO, __VA_ARGS__)
#define LOGD(...) HT_LOG(DEBUG, __VA_ARGS__)
#define LOGT(...) HT_LOG(TRACE, __VA_ARGS__)

extern int ht_serial_logger(const log_level level, ...);
extern char* bytesToHex(const uint8_t* data, int len, char* buffer);

#define HT_LOG(LEVEL, ...) ht_serial_logger(LEVEL, __VA_ARGS__)

#endif
