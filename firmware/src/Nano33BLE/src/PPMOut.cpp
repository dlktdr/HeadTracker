/* From https://os.mbed.com/users/edy05/code/PPM/
*/

#include <mbed.h>
#include "PPMOut.h"
#include <chrono>

 
using namespace mbed;
using namespace rtos;
using namespace std;

 
PpmOut::PpmOut(PinName pin, uint8_t channel_number): ppm(pin) {
    if(channel_number > MAX_CHANNELS){
        this->channel_number = MAX_CHANNELS;
    }
    this->channel_number = channel_number;
    invertoutput = false;
    resetChannels();
    pulse_out = 1;
    ppm = pulse_out;
    current_dot = 0;

    timeout.attach_us(callback(this, &PpmOut::attimeout), (us_timestamp_t)(FRAME_LEN));    
}

PpmOut::~PpmOut()
{
    timeout.detach();
}
 
void PpmOut::setChannel(int channel_no, uint16_t value) {   
    // Channels are set starting at 1, not zero
    channel_no--;
    if(channel_no < 0 ||
        channel_no > channel_number-1){
        return;
    }

    // Limit to min/max
    value = MIN(MAX(value,MIN_CHANNEL_VALUE),MAX_CHANNEL_VALUE);
    
    dots[channel_no*2] = CHANNEL_SYNC;
    dots[channel_no*2+1] = value - CHANNEL_SYNC;
 
    setFrameSync();    
}
 
void PpmOut::setFrameSync(){
    uint16_t sum_channels = 0;
    for(uint8_t channel = 0; channel < channel_number; channel++) {
        sum_channels += dots[channel*2+1];
    }
    sum_channels += channel_number*CHANNEL_SYNC;
    dots[channel_number*2] = CHANNEL_SYNC;
    dots[channel_number*2+1] = FRAME_LEN - CHANNEL_SYNC - sum_channels;
}
 
// Added Inverted PPM output
bool PpmOut::isInverted()
{
    return invertoutput;
}

void PpmOut::setInverted(bool inv)
{
    if(inv != invertoutput) {
        invertoutput = inv;
        pulse_out = !pulse_out; // Flip the output
    }
}

void PpmOut::attimeout() {
    pulse_out = !pulse_out;
    ppm = pulse_out;
    
    timeout.attach_us(callback(this, &PpmOut::attimeout), dots[current_dot]);
    current_dot++;
 
    if(current_dot == channel_number*2+2) { // 2 for FRAME_SYNC
        current_dot = 0;
    }
}
 
void PpmOut::resetChannels() {
    int8_t channel;
    
    current_dot = 0;
    //memset(dots, 0x00, DOTS * sizeof(dots[0]));
    for(channel = 0; channel < channel_number; channel++) {
        dots[channel*2] = CHANNEL_SYNC;
        dots[channel*2+1] = CHANNEL_PAD_SYNC;
    }
    setFrameSync();
}