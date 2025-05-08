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
#include <zephyr/kernel.h>

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
  uint32_t lastUpdate;
  char anglesComputed;
  float _copyQuat[4];  // copy buffer to protect the quaternion values since getters!=setters
  void computeAngles();
  void align(float ax, float ay, float az, float bx, float by, float bz);
  void combine(float p0, float p1, float p2, float p3);
  void rotate(float &ax, float &ay, float &az);
  void normalizeQuat(float &q0, float &q1, float &q2, float &q3);
  float qrot[4];

  //-------------------------------------------------------------------------------------------
  // Function declarations
 public:
  Madgwick(void);
  void begin(float pitch, float roll, float yaw);
  void begin(float ax, float ay, float az, float mx, float my, float mz);
  void alignToAccelVect(float ax, float ay, float az);
  void setRotQuat(float qw, float qx, float qy, float qz)
  {
    qrot[0] = qw;
    qrot[1] = qx;
    qrot[2] = qy;
    qrot[3] = qz;
    normalizeQuat(qrot[0], qrot[1], qrot[2], qrot[3]);
  }
  void setGain(float gain) { beta = gain; }
  void update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my,
              float mz, float deltat);
  void updateIMU(float gx, float gy, float gz, float ax, float ay, float az, float deltat);

  float getRoll()
  {
    if (!anglesComputed) computeAngles();
    return roll * RAD_TO_DEG;
  }
  float getPitch()
  {
    if (!anglesComputed) computeAngles();
    return pitch * RAD_TO_DEG;
  }
  float getYaw()
  {
    if (!anglesComputed) computeAngles();
    return yaw * RAD_TO_DEG;
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

  void getQuat(float dest[4])
  {
    float a2 = q0;
    float b2 = q1;
    float c2 = q2;
    float d2 = q3;
    float a1 =  qrot[0];
    float b1 = -qrot[1];
    float c1 = -qrot[2];
    float d1 = -qrot[3];
    dest[0] = a1 * a2 - b1 * b2 - c1 * c2 - d1 * d2;
    dest[1] = a1 * b2 + b1 * a2 - c1 * d2 + d1 * c2;
    dest[2] = a1 * c2 + c1 * a2 - d1 * b2 + b1 * d2;
    dest[3] = a1 * d2 + d1 * a2 - b1 * c2 + c1 * b2;
  }

  float deltatUpdate()
  {
    uint32_t now = micros();
    uint32_t dt = now - lastUpdate;
    lastUpdate = now;
    deltat = ((float)(dt) / 1000000.0f);
    return deltat;
  }
};
