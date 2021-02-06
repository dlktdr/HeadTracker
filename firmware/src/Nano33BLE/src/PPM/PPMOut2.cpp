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

static uint16_t framesync = 20000;
static uint16_t sync = 300;
static uint32_t chsteps[34] {framesync,sync};
static uint16_t chstepcnt=1;
static uint16_t curstep=0;

void buildChannels() 
{    
    // Start high/low
    // Toggle first time after frame sync
    
    // 1) Toggle at first sync pulse
    // 2) Toggle at chvalue - sync pulse

    // Repeat 1+2 until end

    int ch=0;
    int i;
    uint32_t curtime=framesync;
    chsteps[0] = curtime;    
    Serial.print("Chan Timing: ");
    Serial.print(chsteps[0]); 
    for(i=1; i<ch_count*2+1;i+=2) {
        curtime += sync;
        chsteps[i] = curtime;
        curtime += ch_values[ch++]-sync;
        chsteps[i+1] = curtime;
        Serial.print(" ");
        Serial.print(chsteps[i]);
        Serial.print(" ");
        Serial.print(chsteps[i+1]);
    }
    chstepcnt = i;
    Serial.print(" S:");
    Serial.print(chstepcnt);
    
}

void resetChannels() 
{
    // Set all channels to center
    for(int i=0;i<32;i++)
        ch_values[i] = 1500;
}

extern "C" void Timer3ISR_Handler(void)
{
    if(NRF_TIMER3->EVENTS_COMPARE[0] == 1) {
        interrupt = true;
        // Clear event
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;
        
        // If step 0, reset counter to zero
        if(curstep == 0) {
            // Reset timer to zero, everything counts from this base 0.
            // The little delay from the isr call to this won't matter here
            NRF_TIMER3->TASKS_CLEAR = 1;
        }
            
        // Setup next capture event value
        // values should always be larger than last
        NRF_TIMER3->CC[0] = chsteps[curstep];
        curstep++;
        if(curstep > chstepcnt) {
            curstep = 0;

        }    
    }
}

// Set pin to -1 to disable

void PpmOut_setPin(int pinNum)
{
    // Same pin, just quit
    if(pinNum == setPin)
        return;

    // THIS MUST BE DEFINED SOMEWHERE... CAN'T FIND IT!
    int dpintopin[]  = {0,0,11,12,15,13,14,23,21,27,2,1,8,13};
    int dpintoport[] = {0,0,1 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1,1,1,0 };

    int pin;
    pin = dpintopin[pinNum];    
    int port;
    port = dpintoport[pinNum];    
    
    // Disabled and already Started - Shutdown
    if(pinNum < 0 && ppmoutstarted) {
        setPin = pinNum;    
        __disable_irq();
        ppmoutstarted = false;
        NRF_TIMER3->TASKS_STOP = 1;
        NRF_TIMER3->INTENCLR = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;      
        NVIC_SetVector(GPIOTE_IRQn,oldTimer3Interrupt); // Reset Orig Interupt Vector
        __enable_irq();        
        Serial.println("Stopped Interrupt");

    // Enabled OR Not Started
    } else {
        setPin = pinNum;
        __disable_irq();

        // Disable timer interrupt
        NRF_TIMER3->INTENCLR = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;      

        // Setup GPOITE[7] to toggle output on every timer capture, Start High
        if(!ppmoutinverted) {        
            NRF_GPIOTE->CONFIG[7] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                        (GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos) |
                        (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                        (port << GPIOTE_CONFIG_PORT_Pos);
        
        // Setup GPOITE[7] to toggle output on every timer capture, Start Low
        } else {        
            NRF_GPIOTE->CONFIG[7] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                        (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos) |
                        (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                        (port << GPIOTE_CONFIG_PORT_Pos);
        }

        // If not started
        if(!ppmoutstarted) {

            // Start Timers, they can stay running all the time.
            NRF_TIMER3->PRESCALER = 4; // 16Mhz/2^4 = 1Mhz = 1us Resolution, 1.048s Max@32bit
            NRF_TIMER3->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
            NRF_TIMER3->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;

            // Start timer
            NRF_TIMER3->CC[0] = chsteps[0]; // Frame sync - Step 0
            NRF_TIMER3->TASKS_START = 1;

            // On Compare equals Value, Toggle IO Pin
            NRF_PPI->CH[10].EEP = (uint32_t)&NRF_TIMER3->EVENTS_COMPARE[0];
            NRF_PPI->CH[10].TEP = (uint32_t)NRF_GPIOTE_TASKS_OUT_7;

            // Enable PPI 10
            NRF_PPI->CHEN |= (PPI_CHEN_CH10_Enabled << PPI_CHEN_CH10_Pos);

            // Change timer3 interrupt handler
            oldTimer3Interrupt = NVIC_GetVector(TIMER3_IRQn);
            NVIC_SetVector(TIMER3_IRQn,(uint32_t)&Timer3ISR_Handler);

            
            ppmoutstarted = true;
        }

        // Enable timer interrupt
        NRF_TIMER3->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
        NRF_TIMER3->EVENTS_COMPARE[0] = 0;   

        __enable_irq();
        Serial.println("Started Interrupt");
    }
}

void PpmOut_setInverted(bool inv)
{
    if(!ppmoutstarted)
        return;

    if(ppmoutinverted != inv) {
        int op = setPin;
        PpmOut_setInverted(-1);
        ppmoutinverted = inv;
        PpmOut_setInverted(op);
    }
}

void PpmOut_execute()
{
   /* static bool sentconn=false;
    // Start a timer
    runt.start();    

    // ISR should stop timer on frame start signal
    using namespace std::chrono;
    int micros = duration_cast<microseconds>(runt.elapsed_time()).count();
    if(micros > 60000) {
        if(sentconn == false) {
            serialWriteln("HT: PPM Input Data Lost");
            sentconn = true;
            ch_count = 0;
        }
    } else {
        if(sentconn == true && ch_count >= 4 && ch_count <= 16) {
            serialWriteln("HT: PPM Input Data Received");
            sentconn = false;
        } 
    }*/
}

void PpmOut_setChnCount(int chans)
{
    if(chans >= 4 && chans <=16) {
        ch_count = chans;
        resetChannels();
        Serial.print("Chans Set To "); Serial.println(ch_count);
    }
}

// Returns number of channels read
void PpmOut_setChannel(int chan, uint16_t val)
{
    if(chan >= 0 && chan <= ch_count &&
       val >= 1000 && val <= 2000) {
        ch_values[chan] = val;
    }
    buildChannels();
}