/* From https://os.mbed.com/users/edy05/code/PPM/
*/

#include <mbed.h>
#include <chrono>

#include "PPMOut.h"
 
using namespace mbed;
using namespace rtos;
using namespace std::chrono;

 
PpmOut::PpmOut(PinName pin, uint8_t channel_number): ppm(pin) {
    if(channel_number > MAX_PPM_CHANNELS){
        this->channel_number = MAX_PPM_CHANNELS;
    }
    this->channel_number = channel_number;
    invertoutput = false;
    resetChannels();
    pulse_out = 1;
    ppm = pulse_out;
    current_dot = 0;
    
    timeout.attach_us(callback(this, &PpmOut::attimeout), (us_timestamp_t)(FRAME_LEN));    
    
    // Start a timer to measure actual time.
    timer.start();
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
    
    // We should have fired early. Find out by how much
    //uint64_t ex_wait = (dots[current_dot] * 1000) - duration_cast<nanoseconds>(timer.elapsed_time()).count();
    
    int64_t ex_wait = static_cast<int64_t>(dots[current_dot]) - static_cast<int64_t>(duration_cast<microseconds>(timer.elapsed_time()).count());
    timer.reset();
    
    // Start measuring for next pulse
    
    

    digitalWrite(A1,HIGH); // Measure how much time wasted here
    
    // Pause only when output high, sync pulse can vary without
    /*if(ex_wait < 100)                
        wait_us(ex_wait);*/

    digitalWrite(A1,LOW);

    pulse_out = !pulse_out;
    ppm = pulse_out;
       
    // Setup next interrupt, fire it early to adjust out some jitter
    timeout.attach_us(callback(this, &PpmOut::attimeout), dots[current_dot] - JITTER_TIME * 2);
    
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