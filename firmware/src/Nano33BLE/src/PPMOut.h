#ifndef PPM_OUT
#define PPM_OUT

#include <mbed.h>
#include <rtos.h>

using namespace mbed;
using namespace rtos;

class PpmOut{
    public:
        static const uint8_t MAX_CHANNELS = 8 ;
        static const uint16_t CHANNEL_SYNC = 300; // us
        static const uint16_t CHANNEL_PAD_SYNC = 1000 - CHANNEL_SYNC; // us
        static const uint16_t FRAME_SYNC = 5000; // us
        static const uint16_t FRAME_LEN = 20000; // us
        static const uint16_t MAX_CHANNEL_VALUE = 1980; // us
        static const uint16_t MIN_CHANNEL_VALUE = 1020; 
        static const uint16_t DOTS = MAX_CHANNELS*2+2; // two dots per channel + FRAME_SYNC
 
        // Will start the PPM output 
        PpmOut(PinName pin, uint8_t channel_number);
        ~PpmOut();

        // Values go from MIN_CHANNEL_VALUE to MAX_CHANNEL_VALUE 
        void setChannel(int channel_no, uint16_t value);
        void setAllChannels(uint16_t new_channels[MAX_CHANNELS], int channels);

 
    private:
        // These are the time dots where the signal changes the value 
         //   from 0 to 1 and in reverse 
        uint16_t dots[DOTS];
        Timeout timeout;
        DigitalOut ppm;
        uint8_t current_dot;
        uint8_t channel_number;
        uint16_t frame_length;
        uint16_t pulse_out;
                
        void attimeout();
        inline void resetChannels();
        inline void setFrameSync();
};

#endif