#ifndef CH_PPM_IN
#define CH_PPM_IN

#include "config.h"

using namespace mbed;

class PpmIn
{
    public:
        uint16_t period;
        uint16_t channels[MAX_PPM_CHANNELS+2]; 
        uint8_t nochannels;
        bool state;
        
        PpmIn(PinName pin, int channels=8);
        ~PpmIn();
        
        uint16_t* getPpm();
        void rise();
        void setInverted(bool inv);
        
     protected:     
        InterruptIn ppm;        
        Timer timer;
        uint8_t current_channel;
        bool inverted;
};
 
#endif