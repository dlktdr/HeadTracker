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
    
    // Give time for the rest of the threads to go into sleep loops
    //ThisThread::sleep_for(std::chrono::milliseconds(50));
    
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
    pauseThreads = false;
    return 0;
}

