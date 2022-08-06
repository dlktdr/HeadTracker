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

// Writing to flash

#include "soc_flash.h"

#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <zephyr.h>

#include "defines.h"
#include "log.h"

#define FLASH_OFFSET FLASH_AREA_OFFSET(datapt)
#define FLASH_PAGE_SIZE 4096  // Can grow up to 0x4000

volatile bool pauseForFlash = false;

const char *get_flashSpace()
{
  const char *addr = (const char *)FLASH_OFFSET;
  if (*addr == 0xFF)  // Blank Flash
    return "{\"UUID\":837727}";
  return addr;
}

void socClearFlash()
{
  const struct device *flash_dev;

  flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);

  if (!flash_dev) {
    LOGE("Nordic nRF5 flash driver was not found!");
    return;
  }

  pauseForFlash = true;
  rt_sleep_ms(PAUSE_BEFORE_FLASH);

  if (flash_erase(flash_dev, FLASH_OFFSET, FLASH_PAGE_SIZE) != 0) {
    LOGE("Flash erase Failure");
    pauseForFlash = false;
    return;
  }
  pauseForFlash = false;

  LOGI("Flash erase succeeded");
}

int socWriteFlash(const char *datain, int len)
{
  const struct device *flash_dev;

  flash_dev = device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);

  if (!flash_dev) {
    LOGE("Nordic nRF5 flash driver was not found!");
    return -1;
  }

  pauseForFlash = true;
  rt_sleep_ms(PAUSE_BEFORE_FLASH);

  if (flash_erase(flash_dev, FLASH_OFFSET, FLASH_PAGE_SIZE) != 0) {
    LOGE("Flash erase Failure");
    pauseForFlash = false;
    return -1;
  }

  LOGI("Flash erase succeeded");

  if (flash_write(flash_dev, FLASH_OFFSET, (const void *)datain, FLASH_PAGE_SIZE) != 0) {
    LOGE("   Flash write failed!");
    pauseForFlash = false;
    return -1;
  }

  LOGI("Flash write succeeded");

  pauseForFlash = false;

  return 0;
}
