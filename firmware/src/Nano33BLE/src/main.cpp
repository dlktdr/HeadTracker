#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <platform/Callback.h>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include <chrono>
#include "PPMOut.h"
#include "dataparser.h"
#include "trackersettings.h"
#include "Wire.h"


using namespace rtos;
using namespace mbed;

Thread btThread;
Thread dataThread;
Ticker ioTick;
Thread senseThread;

// PPM Output
PpmOut ppmout(PinName(D11), 8);

void bt_Thread() {
  while(true) {
    
  }
};

void sense_Init()
{
  
 /* if(!IMU.begin()) {
    Serial.println("Failed to initalize IMU");    
  }*/

}

// Read all IMU data and do the calculations
void sense_Thread()
{
  //Madgwick filter;
  unsigned long microsPerReading, microsPrevious;
  float accx=0,accy=0,accz=0;
  float magx=0,magy=0,magz=0;
  float gyrx=0,gyry=0,gyrz=0;
  float pitch=0,roll=0,yaw=0;     

  int count=0;
  while(1) {
    // Update Sensors
    /*if(IMU.accelerationAvailable()) {
      IMU.readAcceleration(accx, accy, accz);
    }
    if(IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gyrx,gyry,gyrz);
      gyrx *= DEG_TO_RAD; // ?? CHECK IF REQUIRED
      gyry *= DEG_TO_RAD;
      gyrz *= DEG_TO_RAD;
    }
    if(IMU.magneticFieldAvailable()) {
      IMU.readMagneticField(magx,magy,magz);
    }
      
    // Period Between Samples
    float deltat = fusion.deltatUpdate();  

    // Do the Calculation
    fusion.MadgwickUpdate(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate

    pitch = fusion.getPitch();
    roll = fusion.getRoll();
    yaw = fusion.getYaw();
    */

    if(count == 60) {
      /*Serial.print("Accel X "); 
      Serial.print(accx);
      Serial.print(" Y "); 
      Serial.print(accy);
      Serial.print(" Z "); 
      Serial.println(accz);
      
      Serial.print("Mag X "); 
      Serial.print(magx);
      Serial.print(" Y "); 
      Serial.print(magy);
      Serial.print(" Z "); 
      Serial.println(magz);
      
      Serial.print("Gyro X "); 
      Serial.print(gyrx);
      Serial.print(" Y "); 
      Serial.print(gyry);
      Serial.print(" Z "); 
      Serial.println(gyrz);

      Serial.print("Pitch ");
      Serial.print(pitch);
      Serial.print(" Roll ");
      Serial.print(roll);
      Serial.print(" Yaw ");
      Serial.println(yaw);*/

      count = 0;
      }
    count++;

    ThisThread::sleep_for(std::chrono::milliseconds(20));
  }
}

// Any IO Related Tasks, buttons, etc.
void io_Task()
{
  static int i =0;
  
  if(i==100) {
    digitalWrite(LED_BUILTIN, HIGH);
  } 
  if(i==200) {
    digitalWrite(LED_BUILTIN, LOW);
    i=0;
  }
  i++;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR,HIGH);
  digitalWrite(LEDG,HIGH);
  digitalWrite(LEDB,HIGH);

  Serial.begin(1000000); // 1 Megabaud
  
  // Start the Data Thread
  dataThread.start(mbed::callback(data_Thread));
  dataThread.set_priority(osPriorityNormal);

  // Serial Read Ready Interrupt
  Serial.attach(&serialrx_Int);

  // Start the BT Thread, Higher Prority than data.
  btThread.start(mbed::callback(bt_Thread)); 
  btThread.set_priority(osPriorityNormal);

  // Start the IO task at 1khz, Realtime priority
  ioTick.attach(mbed::callback(io_Task),std::chrono::milliseconds(1));
  
  // Start the sensor thread, Realtime priority
  sense_Init();
  senseThread.start(mbed::callback(sense_Thread));
  senseThread.set_priority(osPriorityRealtime); 
}

void loop() {}