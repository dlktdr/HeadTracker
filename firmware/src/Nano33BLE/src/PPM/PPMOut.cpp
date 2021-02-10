#include <Arduino.h>


#include <nrfx_ppi.h>
#include <nrfx_gpiote.h>

#include "serial.h"

static uint32_t oldTimer3Interrupt=0;

volatile bool interrupt=false;

static bool ppmoutstarted=false;
static bool ppmoutinverted=false;
static int setPin=-1;

// Used in ISR

// Used to read data at once, read with isr disabled
static uint16_t ch_values[16];
static int ch_count=0;

static uint16_t framesync = 4000; // Minimum Frame Sync Pulse
static int framelength = 20000; // Ideal frame length
static uint16_t sync = 300; // Sync Pulse Length

// Local data - Only build with interrupts disabled
static uint32_t chsteps[35] {framesync,sync};

// ISR Values - Values from chsteps are copied here on step 0.
// prevent an update from happening mid stream.
static uint32_t isrchsteps[35] {framesync,sync};
static uint16_t chstepcnt=1;
static uint16_t curstep=0;
volatile bool buildingdata=false;

/* Builds an array with all the transition times
 */
void buildChannels() 
{        
    buildingdata = true; // Prevent a read happing while this is building

    int ch=0;
    int i;
    uint32_t curtime=framesync;
    chsteps[0] = curtime;    
    for(i=1; i<ch_count*2+1;i+=2) {
        curtime += sync;
        chsteps[i] = curtime;
        curtime += (ch_values[ch++]-sync);
        chsteps[i+1] = curtime;
    }
    // Add Final Sync
    curtime += sync;
    chsteps[i++] = curtime;
    chstepcnt = i;
    // Now we know how long the train is. Try to make the entire frame == framelength
    // If possible it will add this to the frame sync pulse
    int ft = framelength-curtime;
    if(ft < 0) // Not possible, no time left
        ft = 0;
    chsteps[i] = ft; // Store at end of sequence
    buildingdata = false;
}

void resetChannels() 
{
    // Set all channels to center
    for(int i=0;i<16;i++)
        ch_values[i] = 1500;
}

extern "C" void Timer3ISR_Handler(void)
{
    digitalWrite(A1,HIGH);
    if(NRF_TIMER3->EVENTS_COMPARE[0] == 1) {
        // Clear event
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;

        // Reset, don't get stuck in the wrong signal level
        if(curstep == 0) {
            if(ppmoutinverted)
                NRF_GPIOTE->TASKS_SET[7] = 1;
            else
                NRF_GPIOTE->TASKS_CLR[7] = 1;            
        }

        curstep++;
        // Loop
        if(curstep >= chstepcnt) {
            if(!buildingdata)
                memcpy(isrchsteps,chsteps,sizeof(uint32_t)*35);
            NRF_TIMER3->TASKS_CLEAR = 1;            
            curstep = 0;
        }          
        
        // Setup next capture event value     
        NRF_TIMER3->CC[0] = isrchsteps[curstep] + isrchsteps[chstepcnt]; // Offset by the extra time required to make frame length right
    }
    digitalWrite(A1,LOW);
}

// Set pin to -1 to disable

void PpmOut_setPin(int pinNum)
{
    // Same pin, just quit
    if(pinNum == setPin)
        return;

    // THIS MUST BE DEFINED SOMEWHERE... I COULDN'T FIND IT!
    int dpintopin[]  = {0,0,11,12,15,13,14,23,21,27,2,1,8,13};
    int dpintoport[] = {0,0,1 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1,1,1,0 };

    int pin;
    pin = dpintopin[pinNum];    
    int port;
    port = dpintoport[pinNum];    
    
    // Disabled and already Started - Shutdown
    if(pinNum < 0 && ppmoutstarted) {        
        __disable_irq();
        
        // Stop Timer
        NRF_TIMER3->TASKS_STOP = 1;
        
        // Stop Interrupt
        NRF_TIMER3->INTENCLR = TIMER_INTENSET_COMPARE0_Msk;

        // Clear interrupt flag
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;      
        NRF_GPIOTE->CONFIG[7] = 0; // Disable Config
        
        // Was there a previous handler?
        if(oldTimer3Interrupt != 0) {
            NVIC_DisableIRQ(TIMER3_IRQn);
            NVIC_SetVector(TIMER3_IRQn,oldTimer3Interrupt); // Reset Orig Interupt Vector
            NVIC_EnableIRQ(TIMER3_IRQn);
            oldTimer3Interrupt = 0;
        // No - just stop the interrupt them
        } else {
            NVIC_DisableIRQ(TIMER3_IRQn);
        }

        __enable_irq();                

        setPin = pinNum;
        ppmoutstarted = false;

    // Enabled OR Not Started
    } else {
        setPin = pinNum;
        __disable_irq();

        // Disable timer interrupt
        NRF_TIMER3->INTENCLR = TIMER_INTENSET_COMPARE0_Msk;
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;      

        // Setup GPOITE[7] to toggle output on every timer capture, Start High
        if(!ppmoutinverted) {        
            NRF_GPIOTE->CONFIG[7] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                        (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                        (port << GPIOTE_CONFIG_PORT_Pos);
        
        // Setup GPOITE[7] to toggle output on every timer capture, Start Low
        } else {        
            NRF_GPIOTE->CONFIG[7] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                        (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                        (port << GPIOTE_CONFIG_PORT_Pos);
        }

        // If not started
        if(!ppmoutstarted) {

            // Start Timers, they can stay running all the time.
            NRF_TIMER3->PRESCALER = 4; // 16Mhz/2^(4) = 1Mhz = 1us Resolution, 1.048s Max@32bit
            NRF_TIMER3->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
            NRF_TIMER3->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
            
            // On Compare equals Value, Toggle IO Pin
            NRF_PPI->CH[10].EEP = (uint32_t)&NRF_TIMER3->EVENTS_COMPARE[0];
            NRF_PPI->CH[10].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[7];

            // Enable PPI 10
            NRF_PPI->CHEN |= (PPI_CHEN_CH10_Enabled << PPI_CHEN_CH10_Pos);

            // Change timer3 interrupt handler, if enabled
            if(NVIC_GetEnableIRQ(TIMER3_IRQn)) {
                oldTimer3Interrupt = NVIC_GetVector(TIMER3_IRQn);
                NVIC_DisableIRQ(TIMER3_IRQn);                
            } else
                oldTimer3Interrupt = 0;

            NVIC_SetVector(TIMER3_IRQn,(uint32_t)&Timer3ISR_Handler);
            NVIC_EnableIRQ(TIMER3_IRQn);

            // Start timer
            memcpy(isrchsteps,chsteps,sizeof(uint32_t)*32);
            NRF_TIMER3->CC[0] = framesync;
            curstep = 0;
            NRF_TIMER3->TASKS_CLEAR = 1;
            NRF_TIMER3->TASKS_START = 1;
            
            ppmoutstarted = true;
        }

        // Enable timer interrupt
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;
        NRF_TIMER3->INTENSET |= TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        
        __enable_irq();
    }
}

void PpmOut_setInverted(bool inv)
{
    /// Should cause the change on the next frame in the ISR
    ppmoutinverted = inv;
}

void PpmOut_execute()
{

}

void PpmOut_setChnCount(int chans)
{
    if(chans >= 4 && chans <=16) {
        ch_count = chans;
        resetChannels();        
    }
    buildChannels();
}

void PpmOut_setChannel(int chan, uint16_t val)
{
    if(chan >= 0 && chan <= ch_count &&
       val >= 1000 && val <= 2000) {
        ch_values[chan] = val;
    }
    buildChannels();
}

int PpmOut_getChnCount()
{
    return ch_count;
}