//=============================================================================================
// MadgwickAHRS.h
//=============================================================================================
//
// Implementation of Madgwick's IMU and AHRS algorithms.
// See: http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
//
// From the x-io website "Open-source resources available on this website are
// provided under the GNU General Public Licence unless an alternative licence
// is provided in source."
//
// Date			Author          Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
//
//=============================================================================================
#pragma once

#include <math.h>
#include <string.h>
#include <zephyr.h>

#include "defines.h"


//--------------------------------------------------------------------------------------------
// Variable declaration
class Madgwick
{
 private:
  static float invSqrt(float x);
  static float dot(float ax, float ay, float az, float bx, float by, float bz);
  static void cross(float ax, float ay, float az, float bx, float by, float bz, float &cx,
                    float &cy, float &cz);
  static void norm(float &ax, float &ay, float &az);
  float beta;  // algorithm gain
  float q0;
  float q1;
  float q2;
  float q3;  // quaternion of sensor frame relative to auxiliary frame
  float roll;
  float pitch;
  float yaw;
  float deltat;
  float lastUpdate;
  char anglesComputed;
  float _copyQuat[4];  // copy buffer to protect the quaternion values since getters!=setters
  float Now;
  void computeAngles();
  void align(float ax, float ay, float az, float bx, float by, float bz);
  void combine(float p0, float p1, float p2, float p3);
  void rotate(float &ax, float &ay, float &az);

  //-------------------------------------------------------------------------------------------
  // Function declarations
 public:
  Madgwick(void);
  void begin(float pitch, float roll, float yaw);
  void begin(float ax, float ay, float az, float mx, float my, float mz);
  void setGain(float gain) { beta = gain; }
  void update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my,
              float mz, float deltat);
  void updateIMU(float gx, float gy, float gz, float ax, float ay, float az, float deltat);

  float getRoll()
  {
    if (!anglesComputed) computeAngles();
    return roll * 57.29578f;
  }
  float getPitch()
  {
    if (!anglesComputed) computeAngles();
    return pitch * 57.29578f;
  }
  float getYaw()
  {
    if (!anglesComputed) computeAngles();
    return yaw * 57.29578f;
  }
  float getRollRadians()
  {
    if (!anglesComputed) computeAngles();
    return roll;
  }
  float getPitchRadians()
  {
    if (!anglesComputed) computeAngles();
    return pitch;
  }
  float getYawRadians()
  {
    if (!anglesComputed) computeAngles();
    return yaw;
  }

  float *getQuat()
  {
    memcpy(_copyQuat, &q0, sizeof(float) * 4);
    return _copyQuat;
  }

  float deltatUpdate()
  {
    Now = micros64();
    deltat = ((Now - lastUpdate) /
              1000000.0f);  // set integration time by time elapsed since last filter update
    lastUpdate = Now;
    return deltat;
  }
};
