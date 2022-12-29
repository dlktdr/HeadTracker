#ifndef NANO33BLE_HH
#define NANO33BLE_HH

#ifdef __cplusplus
extern "C" {
#endif

void start(void);

#include "zephyr.h"

extern struct k_sem saveToFlash_sem;

#ifdef __cplusplus
}
#endif

#endif
