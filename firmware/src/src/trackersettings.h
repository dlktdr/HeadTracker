#pragma once

#include <zephyr.h>

#include "PPMIn.h"
#include "PPMOut.h"
#include "arduinojsonwrp.h"
#include "btparahead.h"
#include "btpararmt.h"
#include "serial.h"
#include "basetrackersettings.h"

class TrackerSettings : public BaseTrackerSettings
{
 public:
  enum {
    AUX_DISABLE = -1,
    AUX_GYRX,     // 0
    AUX_GYRY,     // 1
    AUX_GYRZ,     // 2
    AUX_ACCELX,   // 3
    AUX_ACCELY,   // 4
    AUX_ACCELZ,   // 5
    AUX_ACCELZO,  // 6
    BT_RSSI,      // 7
    AUX_CNT = AUX_FUNCTIONS + 1,
  };

  TrackerSettings() {}

  void resetFusion() override;
  void pinsChanged() override;

  void setRollReversed(bool value);
  void setPanReversed(bool Value);
  void setTiltReversed(bool Value);
  bool isRollReversed();
  bool isTiltReversed();
  bool isPanReversed();

  void saveToEEPROM();
  void loadFromEEPROM();

  // Setting of data to be returned to the GUI
  /*void setRawGyro(float x, float y, float z);
  void setRawAccel(float x, float y, float z);
  void setRawMag(float x, float y, float z);
  void setRawOrient(float t, float r, float p);
  void setOffGyro(float x, float y, float z);
  void setOffAccel(float x, float y, float z);
  void setOffMag(float x, float y, float z);
  void setOffOrient(float t, float r, float p);
  void setPPMOut(uint16_t t, uint16_t r, uint16_t p);*/

 private:
  bool freshProgram;

};

extern TrackerSettings trkset;