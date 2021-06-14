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
 #include "flash.h"
 #include "main.h"
 #include "serial.h"

// Keep this storage space free for saving data later.
// This will be wiped every time the device is programmed tho
const char flashSpace[SECTOR_SIZE*STORAGE_PAGES] __attribute__ ((section("FLASH"), aligned (0x1000))) =\
     "{\"UUID\":837727}";

const int storageBytes = SECTOR_SIZE*STORAGE_PAGES;

FlashIAP flash;

void flash_Init() {
        flash.init();
}

int writeFlash(char *data, int len)
{
    // Don't let other flash get overwritten
    if(len > storageBytes)
        return -1;

    // Pauses all threads. It's too slow here
    pauseThreads = true;

    // Determine where we can write that won't overwite this program
    uint32_t addr = (uint32_t)flashSpace;
    uint32_t next_sector = addr + flash.get_sector_size(addr);
    char *cuaddr = data; // Incrementing pointer by page size as we go
    bool sector_erased = false;
    const uint32_t page_size = flash.get_page_size();

    while(cuaddr-data < len) {
        // Erase this page if it hasn't been erased
        if (!sector_erased) {
            flash.erase(addr, flash.get_sector_size(addr));
            sector_erased = true;
         //   serialWriteln("Erasing Page");
        }

        // Program page
        flash.program(cuaddr, addr, page_size);

        // Move to next page to write to
        addr += page_size;
        cuaddr += page_size;
        if (addr >= next_sector) {
            next_sector = addr + flash.get_sector_size(addr);
            sector_erased = false;
        }
    }
    pauseThreads = false;
    return 0;
}

