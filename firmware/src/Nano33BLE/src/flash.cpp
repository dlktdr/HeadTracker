// Writing to flash

 #include "flash.h"
 #include "main.h"
 #include "serial.h"

// Keep this storage space free for saving data later.
// This will be wiped every time the device is programmed tho
const char flashSpace[SECTOR_SIZE*STORAGE_PAGES] __attribute__ ((section("FLASH"), aligned (0x1000))) =\
     "{\"UUID\":837727}";

// A section of flash at the end that isn't used
void *flashSpace2;
char *fs2;

const int storageBytes = SECTOR_SIZE*STORAGE_PAGES;

void init_Flash() {
        flash.init();

        /* DON't Use, not sure where it's placing in memory
        // Mayoverwrite bootloader at end -- Will overrite
        uint32_t flash2start = flash.get_flash_size() - storageBytes;
        flash2start &= 0xFFFFFFFF|SECTOR_SIZE; // Align to sector
        flashSpace2 = (uintptr_t *)flash2start;
        fs2 = (char*)flashSpace2;
        */

        // Get Flash Length - Storage Space
        //flashSpace2 = (char *)();
        /*
        uint32_t sector_size = flash.get_sector_size((uint32_t)flashSpace);
        Serial.print("Page Size-"); Serial.println(page_size);
        Serial.print("Sector Size-"); Serial.println(sector_size);
        Serial.print("Flash Size-"); Serial.println(flash.get_flash_size());
        Serial.print("Flash Start-"); Serial.println(flash.get_flash_start());
        Serial.print("Flash Address-"); Serial.println((uint32_t)flashSpace);

        hexDumpMemory((uint8_t *)flashSpace,4096);
        writeFlash("\0\0\0NEW DATA",11);
        hexDumpMemory((uint8_t *)flashSpace,4096);
        */
}

int writeFlash(char *data, int len)
{
    // Don't let other flash get overwritten
    if(len > storageBytes)     
        return -1;

    // Pauses all threads. It's too slow here
    pauseForEEPROM = true;
    // Give time for the rest of the threads to go into sleep loops
    ThisThread::sleep_for(std::chrono::milliseconds(50)); 
    
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

        //serialWrite("Programming addr:"); serialWrite((int)(cuaddr-data)); serialWriteln("");
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
    pauseForEEPROM = false;
    return 0;
}

