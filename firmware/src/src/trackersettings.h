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

  void getMagSiOff(float dest[9])
  {
    dest[0] = getso00();
    dest[1] = getso01();
    dest[2] = getso02();
    dest[3] = getso10();
    dest[4] = getso11();
    dest[5] = getso12();
    dest[6] = getso20();
    dest[7] = getso21();
    dest[8] = getso22();
  }

  void saveToEEPROM();
  void loadFromEEPROM();

  void setSettingModified() {settingsModified = true;}
  bool getSettingsModfied() {return settingsModified;}

 private:
  bool freshProgram;
  bool settingsModified;
};

extern TrackerSettings trkset;