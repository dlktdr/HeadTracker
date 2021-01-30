// FROM https://os.mbed.com/users/edy05/code/PPM/

#include <Arduino.h>
#include <mbed.h>

#include "PPMIn.h"

// *** NOT TESTED.

using namespace std::chrono;
  
PpmIn::PpmIn(PinName pin, int channels): ppm(pin), nochannels(channels)
{
    current_channel = 0;
    state = false;
    timer.start();

    // Add invertered options here to watch on fall
    inverted = false;
    setInverted(inverted);    
}

PpmIn::~PpmIn()
{
    // Disable interrupts
    ppm.rise(0);
    ppm.fall(0);
}

void PpmIn::setInverted(bool inv)
{
    // Delete previous interrupt handlers
    ppm.rise(0);
    ppm.fall(0);
    if(!inv) 
        ppm.rise(callback(this, &PpmIn::rise));
    else
        ppm.fall(callback(this, &PpmIn::rise));
}

uint16_t* PpmIn::getPpm()
{
    return &channels[2];
}
            
void PpmIn::rise()
{
    
    //uint16_t time = timer.read_us();
    uint16_t time = duration_cast<microseconds>(timer.elapsed_time()).count();
    
    // we are in synchro zone
    if(time > 2500)
    {
       // *** Should reset all channels to 1500 incase they aren't being sent
       current_channel = 0;

       // return values 
       state = true;
    }
    else if(current_channel < MAX_PPM_CHANNELS+2)
    {
        channels[current_channel] = duration_cast<microseconds>(timer.elapsed_time()).count();
        current_channel += 1;     
    }
    
    timer.reset();
    
    //if (current_channel > (CHANNELS + 2 - 1)); //+frame and - 1 indexing of channels list
}