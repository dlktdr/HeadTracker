//-----------------------------------------------------------------------------
// Original project by Dennis Frie - 2012 - Dennis.frie@gmail.com
// Discussion: http://www.rcgroups.com/forums/showthread.php?t=1677559
//
// Other contributors to this code:
//  Mark Mansur (Mangus on rcgroups)
//  
// Version history:
// - 0.01 - 0.08 - Dennis Frie - preliminary releases
// - 1.01 - April 2013 - Mark Mansur - code clean-up and refactoring, comments
//      added. Added pause functionality, added settings retrieval commands.
//      Minor optimizations.
// - 2.00 - Nov 2020 - Cliff Blackburn - Modified for use with the Bosch BNO055 IC
//-----------------------------------------------------------------------------

#include <Wire.h>
#include "config.h"
#include "functions.h"
#include "sensors.h"
#include "uCRC16Lib.h"
#include <EEPROM.h>

#define uCRC16Lib_POLYNOMIAL 0x8408

/*
Channel mapping/config for PPM out:

1 = PPM CHANNEL 1
2 = PPM CHANNEL 2
3 = PPM CHANNEL 3
4 = PPM CHANNEL 4
5 = PPM CHANNEL 5
6 = PPM CHANNEL 6
7 = PPM CHANNEL 7
8 = PPM CHANNEL 8
9 = PPM CHANNEL 9
10 = PPM CHANNEL 10
11 = PPM CHANNEL 11
12 = PPM CHANNEL 12

Mapping example:
$123456789111CH
*/

// Local file variables
//
int frameNumber = 0;		    // Frame count since last debug serial output

char serial_data[SERIAL_BUFFER_SIZE+1];          // Array for serial-data 
unsigned char serial_index = 0; // How many bytes have been received?
char string_started = 0;        // Only saves data if string starts with right byte
unsigned char channel_mapping[13];

char outputMag = 0;             // Stream magnetometer data to host
char outputAcc = 0;             // Stream acelerometer data to host
char outputMagAcc = 0;          // Stream mag and accell data (for calibration on PC)
char outputTrack = 0;	        // Stream angle data to host

// Keep track of button press
char lastButtonState = 0;           // 0 is not pressed, 1 is pressed
unsigned long buttonDownTime = 0;   // the system time of the press
char pauseToggled = 0;              // Used to make sure we toggle pause only once per hold
char ht_paused = 0;

// CRC Check
uint16_t crc_calc = 0;
uint16_t crc_val = 0;

// External variables (defined in other files)
//
extern unsigned char PpmIn_PpmOut[13];
extern char read_sensors;
extern char resetValues;   
extern char tiltInverse;
extern char rollInverse;
extern char panInverse;

// Settings (Defined in sensors.cpp)
//
extern float tiltRollBeta;
extern float panBeta;
extern float gyroWeightTiltRoll;
extern float GyroWeightPan;
extern float tiltStart;        
extern float panStart;                
extern float rollStart; 
extern int servoPanCenter;
extern int servoTiltCenter;
extern int servoRollCenter;
extern int panMaxPulse;
extern int panMinPulse;
extern int tiltMaxPulse;
extern int tiltMinPulse;
extern int rollMaxPulse;
extern int rollMinPulse;
extern float panFactor;
extern float tiltFactor;  
extern float rollFactor;
extern unsigned char servoReverseMask;
extern unsigned char htChannels[];
extern unsigned char axisRemap;
extern unsigned char axisSign;
extern bool graphRaw;
extern int I2CPresent;
extern uint8_t sys, gyro, accel, mag;

// End settings   

//--------------------------------------------------------------------------------------
// Func: setup
// Desc: Called by Arduino framework at initialization time. This sets up pins for I/O,
//       initializes sensors, etc.
//--------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(SERIAL_BAUD);

    pinMode(9,OUTPUT);
    digitalWrite(2,HIGH);
    digitalWrite(3,HIGH);  
  
    // Set all other pins to input, for safety.
    pinMode(0,INPUT);
    pinMode(1,INPUT);
    pinMode(2,INPUT);
    pinMode(3,INPUT);
    pinMode(6,INPUT);
    pinMode(7,INPUT);  
    pinMode(8,INPUT);    

    // Set button pin to input:
    pinMode(BUTTON_INPUT,INPUT);
  
    // Set internal pull-up resistor. 
    digitalWrite(BUTTON_INPUT,HIGH);
  
    digitalWrite(0,LOW); // pull-down resistor
    digitalWrite(1,LOW);
    digitalWrite(2,HIGH);
    digitalWrite(3,HIGH);  
  
    pinMode(ARDUINO_LED,OUTPUT);    // Arduino LED
    digitalWrite(ARDUINO_LED, HIGH);

#if FATSHARK_HT_MODULE
    pinMode(BUZZER,OUTPUT);         // Buzzer
#endif

    BEEP_ON();

    // Give it time to be noticed, then turn it off
    delay(200); // Note: only use delay here. This won't work when Timer0 is repurposed later.
    digitalWrite(ARDUINO_LED, LOW);

    BEEP_OFF();

    InitPWMInterrupt();         // Start PWM interrupt  
    Wire.begin();               // Start I2C
    delay(200);                 // Short delay before I2C check
    CheckI2CPresent();

    // If the device have just been programmed, write initial config/values to EEPROM:
    if (EEPROM.read(0) != EEPROM_MAGIC_NUMBER)
    {

        Serial.println("New board - saving default values!");
    
        InitSensors();
        SaveSettings();
        }
 
    GetSettings();                // Get settings saved in EEPROM
    InitSensors();                // Initialize I2C sensors    
    ResetCenter();
    RemapAxes();
    InitTimerInterrupt();         // Start timer interrupt (for sensors)  
}

//--------------------------------------------------------------------------------------
// Func: loop
// Desc: Called by the Arduino framework once per frame. Represents main program loop.
//--------------------------------------------------------------------------------------
void loop()
{  
    // Check input button for reset/pause request
    char buttonPressed = (digitalRead(BUTTON_INPUT) == 0);

    if ( buttonPressed && lastButtonState == 0)
    {
        resetValues = 1; 
        buttonDownTime = 0;
        lastButtonState = 1;
    }
    
    if ( buttonPressed )
    {
        if ( !pauseToggled && (buttonDownTime > BUTTON_HOLD_PAUSE_THRESH) )
        {
            // Pause/unpause
            ht_paused = !ht_paused;
            resetValues = 1;
            pauseToggled = 1;
        }
    }
    else
    {
        pauseToggled = 0;
        lastButtonState = 0;
    }
    
    // All this is used for communication with GUI 
    //
    if (Serial.available())
    {
        // If no data received, wipe the buffer clean
        if(serial_index == 0)
          memset(serial_data,0,SERIAL_BUFFER_SIZE);
      
        if (string_started == 1)
        {
            // Read incoming byte
            serial_data[serial_index++] = Serial.read();
           
            // If string ends with "CH" it's channel configuration, that have been received.
            // String must always be 12 chars/bytes and ending with CH to be valid. 
            if (serial_index == 14 &&
                serial_data[serial_index-2] == 'C' &&
                serial_data[serial_index-1] == 'H' )
            {
                // To keep it simple, we will not let the channels be 0-initialized, but
                // start from 1 to match actual channels. 
                for (unsigned char i = 0; i < 13; i++)
                {
                    channel_mapping[i + 1] = serial_data[i] - 48;
                  
                    // Update the dedicated PPM-in -> PPM-out array for faster performance.
                    if ((serial_data[i] - 48) < 14)
                    {
                        PpmIn_PpmOut[serial_data[i]-48] = i + 1;
                    }
                }
               
                Serial.println("Channel mapping received");
               
               // Reset serial_index and serial_started
               
               // Clear buffer
               serial_index = 0;
               string_started = 0;
            }
            
            // Configure headtracker
            else if (IsCommand("HE"))
            {
                // HT parameters are passed in from the PC in this order:
                //
                // 0 tiltRollBeta      
                // 1 panBeta       
                // 2 gyroWeightTiltRoll    
                // 3 GyroWeightPan 
                // 4 tiltFactor        
                // 5 panFactor          
                // 6 rollFactor
                // 7 servoReverseMask
                // 8 servoPanCenter
                // 9 panMinPulse 
                // 10 panMaxPulse
                // 11 servoTiltCenter
                // 12 tiltMinPulse
                // 13 tiltMaxPulse
                // 14 servoRollCenter
                // 15 rollMinPulse
                // 16 rollMaxPulse
                // 17 htChannels[0]  // pan            
                // 18 htChannels[1]  // tilt 
                // 19 htChannels[2]  // roll         
             
                // Parameters from the PC client need to be scaled to match our local
                // expectations
               
                // Do a CRC Check on the data to insure it's valid data
                if(serial_index > 4)  {
                  crc_calc = uCRC16Lib::calculate(serial_data, serial_index-4); // Calculate the CRC of the data                  
                  memcpy(&crc_val,serial_data + serial_index - 4, sizeof(uint16_t));
                }
                else {
                  crc_calc = 0;
                }

                if(crc_val != crc_calc) {
                    Serial.println("$CRCERR");
                    return;
                } else {                    
                    Serial.println("$CRCOK\r\nHT: Settings received from GUI!");
                }
           
                int valuesReceived[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                int comma_index = 0;

                for (unsigned char k = 0; k < (serial_index - 4); k++)
                {
                    // Looking for comma
                    if (serial_data[k] == ',')
                    {
                        comma_index++;
                    }
                    else
                    {
                        valuesReceived[comma_index] = valuesReceived[comma_index] * 10 + (serial_data[k] - '0');
                    }
             
#if (DEBUG)
                    Serial.print(serial_data[k]);
#endif
                }

#if (DEBUG)
                Serial.println();
                for (unsigned char k = 0; k < comma_index+1; k++)
                {
                    Serial.print(valuesReceived[k]); 
                    Serial.print(",");           
                }
                Serial.println();
#endif

                tiltRollBeta  = (float)valuesReceived[0] / 100;  
                panBeta       = (float)valuesReceived[1] / 100;
                gyroWeightTiltRoll = (float)valuesReceived[2] / 100;
                GyroWeightPan = (float)valuesReceived[3] / 100;
                tiltFactor    = (float)valuesReceived[4] / 10;         
                panFactor     = (float)valuesReceived[5] / 10;          
                rollFactor    = (float)valuesReceived[6] / 10;   

                servoReverseMask = (unsigned char)valuesReceived[7];

                tiltInverse = 1;
                rollInverse = 1;
                panInverse = 1;           
                
                if ((servoReverseMask & HT_PAN_REVERSE_BIT) != 0)
                {
                    panInverse = -1;
                }
                if ((servoReverseMask & HT_ROLL_REVERSE_BIT) != 0)
                {
                    rollInverse = -1; 
                }
                if ((servoReverseMask & HT_TILT_REVERSE_BIT) != 0)
                {
                    tiltInverse = -1;
                }
                servoPanCenter = (valuesReceived[8] - 400) * 2 ;
                panMinPulse = (valuesReceived[9] - 400) * 2;
                panMaxPulse = (valuesReceived[10] - 400) * 2;
         
                servoTiltCenter = (valuesReceived[11] - 400) * 2;
                tiltMinPulse = (valuesReceived[12] - 400) * 2;
                tiltMaxPulse = (valuesReceived[13] - 400) * 2;

                servoRollCenter = (valuesReceived[14] - 400) * 2;
                rollMinPulse = (valuesReceived[15] - 400) * 2;
                rollMaxPulse = (valuesReceived[16] - 400) * 2;   
                    
                htChannels[0] = valuesReceived[17];                   
                htChannels[1] = valuesReceived[18];              
                htChannels[2] = valuesReceived[19];            

                axisRemap = valuesReceived[20];     
                axisSign = valuesReceived[21];

                // Update the BNO055 axis mapping
                RemapAxes();   

                // Auto Save to EEPROM
                SaveSettings();

                serial_index = 0;
                string_started = 0;
            } // end configure headtracker
          
            // Debug info
            else if (IsCommand("DEBUG"))
            {  
                DebugOutput();
                serial_index = 0;
                string_started = 0; 
            }
            
            // Firmware version requested
            else if (IsCommand("VERS")) {
                Serial.print("$VERS");
                Serial.println(FIRMWARE_VERSION_FLOAT, 2);
                serial_index = 0;
                string_started = 0;
            }

            // Hardmware version requested
            else if (IsCommand("HARD")) {
                Serial.print("$HARD");
                Serial.println(SENSOR_NAME);
                serial_index = 0;
                string_started = 0;
            }

            // Reset Button Via Software
            else if (IsCommand("RST"))
            {
                Serial.println("HT: Resetting center");
                resetValues = 1;
                serial_index = 0;
                string_started = 0;
            }
            
            // Clear Offsets via Software
            else if (IsCommand("STO"))
            {
                Serial.println("HT: Calibration saved when all sensors at maximum");
                doCalibrate = true;
                serial_index = 0;
                string_started = 0;
            }

            // Clear Offsets via Software
            else if (IsCommand("CLR"))
            {
                Serial.println("HT: Clearing Offsets");
                tiltStart = 0;        
                panStart = 0;                
                rollStart = 0; 
                serial_index = 0;
                string_started = 0;
            }

            // Graph raw sensor data
            else if (IsCommand("GRAW"))
            {
                Serial.println("HT: Showing Raw Sensor Data");
                graphRaw = 1;        
                serial_index = 0;
                string_started = 0;
            }

            // Graph offset sensor data
            else if (IsCommand("GOFF"))
            {
                Serial.println("HT: Showing Offset Sensor Data");
                graphRaw = 0;        
                serial_index = 0;
                string_started = 0;
            }
          
            // Stop any data stream commands (leave all aliases for compatibility
            else if (IsCommand("CMAE") || IsCommand("CAEN") || IsCommand("GREN") || IsCommand("PLEN") ) {
                outputMagAcc = 0;
                outputMag = 0;
                outputTrack = 0;
                outputAcc = 0;
                serial_index = 0;
                string_started = 0;
            }
           
            // Start tracking data stream
            else if (IsCommand("PLST"))
            {  
                outputAcc = 0;
                outputMagAcc = 0;
                outputMag = 0;
                outputTrack = 1;
                serial_index = 0;
                string_started = 0;
            }        

            // Save RAM settings to EEPROM
            else if (IsCommand("SAVE"))
            {  
                SaveSettings();     
                serial_index = 0;
                string_started = 0; 
            }          
          
            // Retrieve settings
            else if(IsCommand("GSET"))
            {
                // Get Settings. Scale our local values to real-world values usable on the PC side.
                Serial.print("$SET$"); // something recognizable in the stream

                SendSettings();

                Serial.println("HT: Settings sent to GUI");
                
                serial_index = 0;
                string_started = 0;
            } else if (serial_index > SERIAL_BUFFER_SIZE)
            {
                // If more than 100 bytes have been received, the string is not valid.
                // Reset and "try again" (wait for $ to indicate start of new string). 
                serial_index = 0;
                string_started = 0;
            }
        }
        else if (Serial.read() == '$')
        {
            string_started = 1;
        }
    } // serial port input

    // if "read_sensors" flag is set high, read sensors and update
    if (read_sensors == 1 && ht_paused == 0)
    {
        UpdateSensors();
        FilterSensorData();    
               
        // Only output this data every X frames.
        if (frameNumber++ >= SERIAL_OUTPUT_FRAME_INTERVAL)
        {
            if (outputTrack == 1)
            {
                trackerOutput();
            }
            frameNumber = 0; 
        }

        // Will first update read_sensors when everything is done.  
        read_sensors = 0;
    }
}

//--------------------------------------------------------------------------------------
// Check if current line ends with specified command 
//--------------------------------------------------------------------------------------
bool IsCommand(String command) {
    // Ensure we read enough characters
    int cLength = command.length();
    if( cLength > serial_index ) return false;
    // ..and then check characters one by one
    for (int i = 0; i < cLength; i++) {
         if (serial_data[serial_index - cLength + i ] != command.charAt(i)) return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------
// Send Settings
//--------------------------------------------------------------------------------------

void SendSettings() {
  Serial.print(tiltRollBeta * 100);
  Serial.print(",");   
  Serial.print(panBeta * 100);
  Serial.print(",");
  Serial.print(gyroWeightTiltRoll * 100);  
  Serial.print(",");
  Serial.print(GyroWeightPan * 100);
  Serial.print(",");
  Serial.print(tiltFactor * 10);
  Serial.print(",");
  Serial.print(panFactor * 10);
  Serial.print(",");
  Serial.print(rollFactor * 10);
  Serial.print(",");
  Serial.print(servoReverseMask);
  Serial.print(",");
  Serial.print(servoPanCenter / 2 + 400);
  Serial.print(",");
  Serial.print(panMinPulse / 2 + 400);
  Serial.print(",");
  Serial.print(panMaxPulse / 2 + 400);
  Serial.print(",");
  Serial.print(servoTiltCenter / 2 + 400);
  Serial.print(",");
  Serial.print(tiltMinPulse / 2 + 400);
  Serial.print(",");
  Serial.print(tiltMaxPulse / 2 + 400);
  Serial.print(",");
  Serial.print(servoRollCenter / 2 + 400);
  Serial.print(",");
  Serial.print(rollMinPulse / 2 + 400);
  Serial.print(",");
  Serial.print(rollMaxPulse / 2 + 400);
  Serial.print(",");
  Serial.print(htChannels[0]);
  Serial.print(",");
  Serial.print(htChannels[1]);
  Serial.print(",");
  Serial.print(htChannels[2]);
  Serial.print(",");
  Serial.print(axisRemap);
  Serial.print(",");
  Serial.println(axisSign);  
}

//--------------------------------------------------------------------------------------
// Func: SaveSettings
// Desc: Saves device settings to EEPROM for retrieval at boot-up.
//--------------------------------------------------------------------------------------
void SaveSettings()
{  
    EEPROM.write(1, (unsigned char)(tiltRollBeta * 100));
    EEPROM.write(2, (unsigned char)(panBeta * 100));    
    EEPROM.write(5, (unsigned char)servoReverseMask);
    
    // 6 unused
  
    EEPROM.write(7, (unsigned char)servoPanCenter);
    EEPROM.write(8, (unsigned char)(servoPanCenter >> 8));  
  
    EEPROM.write(9, (unsigned char)(tiltFactor * 10));
    EEPROM.write(10, (int)((tiltFactor * 10)) >> 8);  

    EEPROM.write(11, (unsigned char) (panFactor * 10));
    EEPROM.write(12, (int)((panFactor * 10)) >> 8);  

    EEPROM.write(13, (unsigned char) (rollFactor * 10));
    EEPROM.write(14, (int)((rollFactor * 10)) >> 8);  

    // 15 unused

    EEPROM.write(16, (unsigned char)servoTiltCenter);
    EEPROM.write(17, (unsigned char)(servoTiltCenter >> 8));  

    EEPROM.write(18, (unsigned char)servoRollCenter);
    EEPROM.write(19, (unsigned char)(servoRollCenter >> 8));  


    EEPROM.write(20, (unsigned char)panMaxPulse);
    EEPROM.write(21, (unsigned char)(panMaxPulse >> 8));  
  
    EEPROM.write(22, (unsigned char)panMinPulse);
    EEPROM.write(23, (unsigned char)(panMinPulse >> 8));    

    EEPROM.write(24, (unsigned char)tiltMaxPulse);
    EEPROM.write(25, (unsigned char)(tiltMaxPulse >> 8));    

    EEPROM.write(26, (unsigned char)tiltMinPulse);
    EEPROM.write(27, (unsigned char)(tiltMinPulse >> 8));

    EEPROM.write(28, (unsigned char)rollMaxPulse);
    EEPROM.write(29, (unsigned char)(rollMaxPulse >> 8));    

    EEPROM.write(30, (unsigned char)rollMinPulse);
    EEPROM.write(31, (unsigned char)(rollMinPulse >> 8)); 
  
    EEPROM.write(32, (unsigned char)htChannels[0]);
    EEPROM.write(33, (unsigned char)htChannels[1]);
    EEPROM.write(34, (unsigned char)htChannels[2]);
    
    EEPROM.write(35, (unsigned char)axisRemap);
    EEPROM.write(36, (unsigned char)axisSign);
   
    
    // Mark the memory to indicate that it has been
    // written. Used to determine if board is newly flashed
    // or not.
  
    EEPROM.write(0,EEPROM_MAGIC_NUMBER); 

    Serial.println("HT: Settings saved to EEPROM!");
}

//--------------------------------------------------------------------------------------
// Func: GetSettings
// Desc: Retrieves device settings from EEPROM.
//--------------------------------------------------------------------------------------
void GetSettings()
{  
    tiltRollBeta    = (float)EEPROM.read(1) / 100;
    panBeta         = (float)EEPROM.read(2) / 100;
  
    tiltInverse = 1;
    rollInverse = 1;
    panInverse = 1;

    unsigned char temp = EEPROM.read(5);
    if ( temp & HT_TILT_REVERSE_BIT )
    {
        tiltInverse = -1;
    }  
    if ( temp & HT_ROLL_REVERSE_BIT )
    {
        rollInverse = -1;
    }
    if ( temp & HT_PAN_REVERSE_BIT )
    {
        panInverse = -1;
    }

    // 6 unused

    servoPanCenter  = EEPROM.read(7) + (EEPROM.read(8) << 8);
    tiltFactor      = (float)(EEPROM.read(9) + (EEPROM.read(10) << 8)) / 10;
    panFactor       = (float)(EEPROM.read(11) + (EEPROM.read(12) << 8)) / 10;
    rollFactor       = (float)(EEPROM.read(13) + (EEPROM.read(14) << 8)) / 10;  

    // 15 unused

    servoTiltCenter = EEPROM.read(16) + (EEPROM.read(17) << 8);
    servoRollCenter = EEPROM.read(18) + (EEPROM.read(19) << 8);  
  
    panMaxPulse   = EEPROM.read(20) + (EEPROM.read(21) << 8);  
    panMinPulse   = EEPROM.read(22) + (EEPROM.read(23) << 8);    
  
    tiltMaxPulse  = EEPROM.read(24) + (EEPROM.read(25) << 8);  
    tiltMinPulse  = EEPROM.read(26) + (EEPROM.read(27) << 8);      
  
    rollMaxPulse  = EEPROM.read(28) + (EEPROM.read(29) << 8);  
    rollMinPulse  = EEPROM.read(30) + (EEPROM.read(31) << 8);        
  
    htChannels[0] = EEPROM.read(32);  
    htChannels[1] = EEPROM.read(33);  
    htChannels[2] = EEPROM.read(34);    

    axisRemap = EEPROM.read(35);
    axisSign = EEPROM.read(36);

#if (DEBUG)
    DebugOutput();
#endif
}

//--------------------------------------------------------------------------------------
// Func: DebugOutput
// Desc: Outputs useful device/debug information to the serial port.
//--------------------------------------------------------------------------------------
void DebugOutput()
{
    Serial.println();  
    Serial.println();
    Serial.println();
    Serial.println("HT: ------ Debug info------");

    Serial.print("FW Version: ");
    Serial.println(FIRMWARE_VERSION_FLOAT, 2);
    
    Serial.print("tiltRollBeta: ");
    Serial.println(tiltRollBeta); 

    Serial.print("panBeta: ");
    Serial.println(panBeta); 
 
    Serial.print("servoPanCenter: ");
    Serial.println(servoPanCenter); 
 
    Serial.print("servoTiltCenter: ");
    Serial.println(servoTiltCenter); 

    Serial.print("servoRollCenter: ");
    Serial.println(servoRollCenter); 

    Serial.print("tiltFactor: ");
    Serial.println(tiltFactor); 

    Serial.print("panFactor: ");
    Serial.println(panFactor); 

    Serial.print("axisRemap: ");
    Serial.println(axisRemap); 

}
