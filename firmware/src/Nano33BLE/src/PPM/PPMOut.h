#ifndef PPM_OUT
#define PPM_OUT

#include <rtos.h>

#include "config.h"

using namespace mbed;
using namespace rtos;

class PpmOut{
    public:
        static const uint16_t CHANNEL_SYNC = 300; // us
        static const uint16_t PPM_CENTER = 1500;
        static const uint16_t CHANNEL_PAD_SYNC = PPM_CENTER - CHANNEL_SYNC; // us
        static const uint16_t FRAME_SYNC = 5000; // us
        static const uint16_t FRAME_LEN = 25000; // us = 50Hz
        static const uint16_t MAX_CHANNEL_VALUE = 2000;
        static const uint16_t MIN_CHANNEL_VALUE = 1000;
        static const uint16_t DOTS = MAX_PPM_CHANNELS*2+2; // two dots per channel + FRAME_SYNC
        
        // Added this to fire all interrupts 30us early, to remove most jitter on ppm channels
        static const uint16_t JITTER_TIME = 30; // Fire all interrupts 30us Early
 
        // Will start the PPM output 
        PpmOut(PinName pin, uint8_t channel_number);
        ~PpmOut();

        // Values go from MIN_CHANNEL_VALUE to MAX_CHANNEL_VALUE 
        void setChannel(int channel_no, uint16_t value);
        void setAllChannels(uint16_t new_channels[MAX_PPM_CHANNELS], int channels);
        bool isInverted();
        void setInverted(bool inv);
        
 
    private:
        // These are the time dots where the signal changes the value 
         //   from 0 to 1 and in reverse 
        uint16_t dots[DOTS];
        Timeout timeout;
        Timer timer;           
        uint64_t lasttime;     
        DigitalOut ppm;
        uint8_t current_dot;
        uint8_t last_dot;
        uint8_t channel_number;
        uint16_t frame_length;
        uint16_t pulse_out;
        bool invertoutput;
        
                
        void attimeout();
        inline void resetChannels();
        inline void setFrameSync();
};

#endif