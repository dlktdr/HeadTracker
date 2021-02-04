#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>

#define STORAGE_PAGES 4
#define SECTOR_SIZE 0x1000

void flash_Init();
int writeFlash(char *data, int len);

extern const char flashSpace[];

#endif