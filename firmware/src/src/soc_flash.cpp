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

#include "soc_flash.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_SOC_ESP32) || defined(CONFIG_SOC_ESP32C3) || defined(CONFIG_SOC_ESP32S3)
#include <spi_flash_mmap.h>
#include <soc.h>
static bool isFlashMemMapped = false;
spi_flash_mmap_handle_t handle;
#endif

#include "defines.h"

LOG_MODULE_REGISTER(soc_flash);

#define FLASH_PARTITION	ht_data_partition
#define FLASH_OFFSET FIXED_PARTITION_OFFSET(FLASH_PARTITION)
#define FLASH_SIZE FIXED_PARTITION_SIZE(FLASH_PARTITION)
#define FLASH_DEVICE FIXED_PARTITION_DEVICE(FLASH_PARTITION)

// Pause for flash write to complete
K_SEM_DEFINE(flashWriteSemaphore, 0, 1);

// Use a pointer to flash on these architectures
#if defined(CONFIG_SOC_ESP32) ||\
    defined(CONFIG_SOC_ESP32C3) ||\
    defined(CONFIG_SOC_ESP32S3) ||\
    defined(CONFIG_SOC_SERIES_NRF52X)
const uint8_t *mem_ptr = NULL;
#else
// Architecture does not support memory mapping, use a buffer
uint8_t mem_ptr[FLASH_SIZE];
#endif

int socReadFlash(uint8_t dataout[FLASH_SIZE]);

/* Get the flash space as a memory mapped pointer
 *   on ESP it must call spi_flash_mmap to map the flash space
 *   on nRF it just returns the address of the flash space
 *
 *   Returns null on failure
 */

const uint8_t *socGetMMFlashPtr()
{
  #if defined(CONFIG_SOC_ESP32) || defined(CONFIG_SOC_ESP32C3) || defined(CONFIG_SOC_ESP32S3)
  if(!isFlashMemMapped) {
    const struct device *flash_device = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    if (!device_is_ready(flash_device)) {
      LOG_ERR("%s: device not ready.\n", flash_device->name);
      return NULL;
    }

    /* map selected region */
    spi_flash_mmap(FLASH_OFFSET, FLASH_SIZE, SPI_FLASH_MMAP_DATA, (const void **)&mem_ptr, &handle);
    isFlashMemMapped = true;
  }
  #elif defined(CONFIG_SOC_SERIES_NRF52X)
  mem_ptr = (const uint8_t *)FLASH_OFFSET;
  #else
  socReadFlash(mem_ptr);
  #endif
  // Is flash memory blank. Return a indicator there is nothing to be read
  if(*mem_ptr == 0xFF) {
    return (uint8_t *)"{\"UUID\":837727,\"TEST\":343}";
  }
  return mem_ptr;
}

int socReadFlash(uint8_t dataout[FLASH_SIZE])
{
  const struct device *flash_device = FLASH_DEVICE;
	if (!device_is_ready(flash_device)) {
		LOG_ERR("%s: device not ready.\n", flash_device->name);
		return -1;
	}
  memset(dataout, 0, FLASH_SIZE);
	flash_read(flash_device, FLASH_OFFSET, dataout, FLASH_SIZE);
  return 0;
}

void socClearFlash()
{
  const struct device *flash_device = FLASH_DEVICE;
  if (!flash_device) {
    LOG_ERR("Flash Device Not Found!");
    return;
  }

  LOG_INF("Erasing Flash at Offset: %d", FLASH_OFFSET);
  LOG_INF("Flash Size: %d", FLASH_SIZE);

  k_sem_give(&flashWriteSemaphore);

  k_msleep(PAUSE_BEFORE_FLASH);

  if (flash_erase(flash_device, FLASH_OFFSET, FLASH_SIZE) != 0) {
    LOG_ERR("Flash erase Failure");

    return;
  }
  k_sem_take(&flashWriteSemaphore, K_NO_WAIT);
  LOG_INF("Flash erase succeeded");
}

int socWriteFlash(const uint8_t *datain, int len)
{
  const struct device *flash_dev;

  flash_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

  if (!flash_dev) {
    LOG_ERR("Flash driver was not found!");
    return -1;
  }

  k_sem_give(&flashWriteSemaphore);
  k_msleep(PAUSE_BEFORE_FLASH);

  LOG_INF("Erasing Flash at Offset: %d", FLASH_OFFSET);
  LOG_INF("Flash Size: %d", FLASH_SIZE);

  if (flash_erase(flash_dev, FLASH_OFFSET, FLASH_SIZE) != 0) {
    LOG_ERR("Flash erase Failure");
    k_sem_take(&flashWriteSemaphore, K_NO_WAIT);
    return -1;
  }

  LOG_INF("Flash erase succeeded");
  k_msleep(PAUSE_BEFORE_FLASH);

  if (flash_write(flash_dev, FLASH_OFFSET, (const void *)datain, len) != 0) {
    LOG_ERR("Flash write failed!");
    k_sem_take(&flashWriteSemaphore, K_NO_WAIT);
    return -1;
  }

  LOG_INF("Flash write succeeded, %d%% used", (len * 100) / FLASH_SIZE);
  k_sem_take(&flashWriteSemaphore, K_NO_WAIT);

  return 0;
}
