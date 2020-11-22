//-----------------------------------------------------------------------------
// File: Functions.cpp
// Desc: Implementations of PPM-related functions for the project.
//-----------------------------------------------------------------------------
#include "config.h"
#include "Arduino.h"
#include "functions.h"
#include "sensors.h"
#include <Wire.h>

extern long channel_value[];

// Local variables
unsigned int pulseTime = 0; 
unsigned int lastTime = 0; 
unsigned int timeRead; 
int channelsDetected = 0;
char channel = 0;
int channelValues[20]; 
char state = 0; // PPM signal high or Low?
char read_sensors = 0;

// external variables
extern unsigned long buttonDownTime;
extern unsigned char htChannels[];

// Sensor_board,   x,y,z
int acc_raw[3]  = {1,2,3};  
int gyro_raw[3] = {4,5,6};
int mag_raw[3]  = {7,8,9};

unsigned char PpmIn_PpmOut[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
long channel_value[13] = {2100,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100,2100};

unsigned char channel_number = 1;
char shift = 0;
char time_out = 0;

//--------------------------------------------------------------------------------------
// Func: PrintPPM
// Desc: Prints the channel value represented in the stream. Debugging assistant. 
//--------------------------------------------------------------------------------------
void PrintPPM()
{
  for (char j = 1; j < 13; j++)
  {
      Serial.print(channel_value[j]);
      Serial.print(",");
  }
  Serial.println();
  
}

//--------------------------------------------------------------------------------------
// Func: InitPWMInterrupt
// Desc: 
//--------------------------------------------------------------------------------------
void InitPWMInterrupt()
{
#if (DEBUG)    
    Serial.println("PWM interrupt initialized");
#endif
  
    TCCR1A = 
       (0 << WGM10) |
       (0 << WGM11) |
       (0 << COM1A1) |
       (1 << COM1A0) | // Toggle pin om compare-match
       (0 << COM1B1) |
       (0 << COM1B0);  
  
    TCCR1B =
        (1 << ICNC1)| // Input capture noise canceler - set to active 
        (1 << ICES1)| // Input capture edge select. 1 = rising, 0 = falling. We will toggle this, doesn't matter what it starts at        
        (0 << CS10) | // Prescale 8  
        (1 << CS11) | // Prescale 8  
        (0 << CS12) | // Prescale 8
        (0 << WGM13)|    
        (1 << WGM12); // CTC mode (Clear timer on compare match) with ICR1 as top.           
    
    // Not used in this case:
    TCCR1C =
        (0 << FOC1A)| // No force output compare (A)
        (0 << FOC1B); // No force output compare (B)
        
    
    TIMSK1 = 
        (PPM_IN << ICIE1) | // Enable input capture interrupt    
        (1 << OCIE1A) | // Interrupt on compare A
        (0 << OCIE1B) | // Disable interrupt on compare B    
        (0 << TOIE1);          

    // OCR1A is used to generate PPM signal and later reset counter (to control frame-length)
    OCR1A = DEAD_TIME;    
}

//--------------------------------------------------------------------------------------
// Func: InitTimerInterrupt
// Desc: Initializes timer interrupts.
//--------------------------------------------------------------------------------------
void InitTimerInterrupt()
{  
#if (DEBUG)    
    Serial.println("Timer interrupt initialized");
#endif
  
    TCCR0A = 
       (0 << WGM00) |
       (1 << WGM01) |
       (0 << COM0A1) |
       (0 << COM0A0) |
       (0 << COM0B1) |
       (0 << COM0B0);  
   
    // 61 hz update-rate:
    TCCR0B =
        (0 << FOC0A)| // 
        (0 << FOC0B)| // 
        (1 << CS00) | // Prescale 1024 
        (0 << CS01) | // Prescale 1024  
        (1 << CS02) | // Prescale 1024
        (0 << WGM02);
  
    TIMSK0 = 
        (0 << OCIE0B) |
        (1 << OCIE0A) |
        (1 << TOIE0);       

    OCR0B = 64 * 2; 
    OCR0A = 64 * 2;
}

//--------------------------------------------------------------------------------------
// Func: TIMER1_OVF_vect
// Desc: Timer 1 overflow vector - only here for debugging/testing, as it should always
//      be reset/cleared before overflow. 
//--------------------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{
    Serial.print("Timer 1 OVF");
}

//--------------------------------------------------------------------------------------
// Func: TIMER1_COMPA_vect
// Desc: Timer 1 compare A vector
//--------------------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect)
{
    if (OCR1A == FRAME_LENGTH)
    {
        TCCR1A = 
            (0 << WGM10) |
            (0 << WGM11) |
            (1 << COM1A1) |
            (1 << COM1A0) |
            (0 << COM1B1) |
            (0 << COM1B0);   
  
        channel_number = 1;
        OCR1A = DEAD_TIME;
  
        TCCR1B &= ~(1 << WGM12);
    }
    else
    {
        if (channel_number == 1)
        {
            // After first time, when pin have been set high, we toggle the pin at each interrupt
            TCCR1A = 
                (0 << WGM10) |
                (0 << WGM11) |
                (0 << COM1A1) |
                (POSITIVE_SHIFT_PPM << COM1A0) |
                (0 << COM1B1) |
                (0 << COM1B0);   
        }
                  
        if ((channel_number - 1) < NUMBER_OF_CHANNELS * 2)
        {
            if ((channel_number-1) % 2 == 1)
            {
                OCR1A += DEAD_TIME; 
            }
            else
            {
                OCR1A += channel_value[(channel_number + 1) / 2];
            }
            channel_number++;
        }
        else
        {
            // We have to use OCR1A as top too, as ICR1 is used for input capture and OCR1B can't be
            // used as top. 
            OCR1A = FRAME_LENGTH;
            TCCR1B |= (1 << WGM12);
        }
    }
}  

//--------------------------------------------------------------------------------------
// Func: TIMER0_COMPA_vect
// Desc: Timer 0 compare A vector Sensor-interrupt. We query sensors on a timer, not
//          during every loop.
//--------------------------------------------------------------------------------------
ISR(TIMER0_COMPA_vect)
{
    // Reset counter - should be changed to CTC timer mode. 
    TCNT0 = 0;

/*
    // Used to check timing - have the previous calculations been done?
    // Will always show in start, but should stop when initialized. 
    if (shift == 1)
    {
        //digitalWrite(7,HIGH); 
        shift = 0;
    }
    else
    {
        //digitalWrite(7,LOW);
        shift = 1; 
    }
 */

#if (DEBUG == 1)  
    if (read_sensors == 1)
    {
        time_out++;
        if (time_out > 10)
        {
            //Serial.println("Timing problem!!!"); 
            time_out = 0;  
        }
    }
#endif
    read_sensors = 1;
    buttonDownTime += 16; // every 16 milliseconds, at 61 hz.
}

//--------------------------------------------------------------------------------------
// Func: DetectPPM
// Desc: 
//--------------------------------------------------------------------------------------
void DetectPPM()
{  
    // If a new frame is detected
    if (pulseTime > 5500)
    {
        // Save total channels detected
        channelsDetected = channel; 
     
        // Reset channel-count
        channel = 0; 
    }
    else if (channel < 20 && pulseTime > PPM_IN_MIN && pulseTime < PPM_IN_MAX)
    {
        // If the pulse is recognized as servo-pulse
        if ( (channel + 1) != htChannels[0] &&
             (channel + 1) != htChannels[1] &&
             (channel + 1) != htChannels[2] )
        {
            channelValues[channel++] = pulseTime;
            channel_value[PpmIn_PpmOut[channel]] = pulseTime;
        }
    }  
    else if (pulseTime > PPM_IN_MIN)
    {
        channel++;
    }
}  

//--------------------------------------------------------------------------------------
// Func: TIMER1_CAPT_vect
// Desc: The interrupt vector used when an edge is detected Interrupt vector, see page
//      57 Interrupt for input capture
//--------------------------------------------------------------------------------------
ISR(TIMER1_CAPT_vect)
{
    // Disable interrupt first, to avoid multiple interrupts causing hanging/restart,
    // or just weird behavior:
    TIMSK1 &= ~(1 << ICIE1);
    
    state = TCCR1B & (1 << ICES1);
    
    // Toggle interrupt to detect falling/rising edge:
    TCCR1B ^= (1<<ICES1);
    
    // Read the time-value stored in ICR1 register (will be the time copied from TCNT1 at input-capture event). 
    timeRead = ICR1;    
    
    pulseTime = timeRead; 
    
    // Check if the timer have reached top/started over:
    if (lastTime > pulseTime)
    {
        pulseTime += (TOP - lastTime); 
    }
    else
    {
        // Subtract last time to get the time:
        pulseTime -= lastTime;
    }
    
    // Save current timer-value to be used in next interrupt:
    lastTime = timeRead;
    
    // If we are detecting a PPM input signal
    DetectPPM(); 
    
    // Enable interrupt again:
    TIMSK1 |= (1 << ICIE1); 
}