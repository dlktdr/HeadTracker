#include "defines.h"
#include "features.h"
#include "boardsdefs.h"

// Tells the GUI What Features are Available on this board
void getBoardFeatures(DynamicJsonDocument &json)
{
  JsonArray array = json.createNestedArray("FEAT");
  #if defined(HAS_LSM6DS3)
  array.add("IMU");
  #endif
  #if defined(HAS_MPU6886)
  array.add("IMU");
  #endif
  #if defined(HAS_QMC5883)
  array.add("MAG");
  #endif
  #if defined(HAS_BMM150)
  array.add("MAG");
  #endif
  #if defined(HAS_BMI270)
  array.add("IMU");
  #endif
  #if defined(HAS_MPU6886)
  array.add("IMU");
  #endif
  #if defined(HAS_LSM9DS1)
  array.add("IMU");
  array.add("MAG");
  #endif
  #if defined(HAS_APDS9960)
  array.add("PROXIMITY");
  #endif
  #if defined(HAS_CENTERBTN)
  array.add("CENTER");
  #endif
  // TODO add generic io pins that are selectable
  #if defined(HAS_PPMIN) || DT_NODE_EXISTS(DT_NODELABEL(ppm_input))
  array.add("PPMIN");
  #endif
  #if defined(HAS_PPMOUT) || DT_NODE_EXISTS(DT_NODELABEL(ppm_output))
  array.add("PPMOUT");
  #endif
  #if defined(HAS_1CH_PWM)
  array.add("PWM2CH");
  #endif
  #if defined(HAS_2CH_PWM)
  array.add("PWM2CH");
  #endif
  #if defined(HAS_2CH_PWM)
  array.add("PWM2CH");
  #endif
  #if defined(HAS_4CH_PWM)
  array.add("PWM4CH");
  #endif
  #if defined(HAS_1CH_ANALOG)
  array.add("AN1CH");
  #endif
  #if defined(HAS_2CH_ANALOG)
  array.add("AN2CH");
  #endif
  #if defined(HAS_3CH_ANALOG)
  array.add("AN3CH");
  #endif
  #if defined(HAS_4CH_ANALOG)
  array.add("AN4CH");
  #endif
  #if defined(HAS_AUXSERIAL) || DT_NODE_EXISTS(DT_ALIAS(auxserial))
  array.add("AUXSERIAL");
    #if defined(CONFIG_SOC_SERIES_NRF52) // Only the NRF52 is capable of inverting serial so far
    array.add("AUXINVERT");
    #endif
  #endif
  #if defined(HAS_NOTIFYLED)
  array.add("LED");
  #endif
  #if defined(HAS_3DIODE_RGB) || defined(HAS_WS2812)
  array.add("RGB");
  #endif
  #if defined(HAS_BUZZER)
  array.add("BUZZER");
  #endif
  #if defined(CONFIG_BT)
  array.add("BT");
    #if defined(CONFIG_BT_SETTINGS)
    array.add("BTJOYSTICK");
    #endif
  #endif
  #if defined(CONFIG_WIFI)
  array.add("WIFI");
  #endif
  #if defined(CONFIG_USB_DEVICE_HID)
  array.add("JOYSTICK");
  #endif
  #if defined(HAS_VOLTMON)
  array.add("BATTVOLT");
  #endif

  // Pin Mappings
  JsonObject pobj = json.createNestedObject("PINS");
  for(unsigned int i=0; i < sizeof(PinNumber)/sizeof(PinNumber[0]); i++) {
    pobj[StrPins[i]] = StrPinDescriptions[i];
  }
}

