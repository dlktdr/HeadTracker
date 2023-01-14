//==============================================================================================
// MahonyAHRS.c
//==============================================================================================
//
// Madgwick's implementation of Mayhony's AHRS algorithm.
// See: http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
//
// From the x-io website "Open-source resources available on this website are
// provided under the GNU General Public Licence unless an alternative licence
// is provided in source."
//
// Date			Author			Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
//
// Algorithm paper:
// http://ieeexplore.ieee.org/xpl/login.jsp?tp=&arnumber=4608934&url=http%3A%2F%2Fieeexplore.ieee.org%2Fstamp%2Fstamp.jsp%3Ftp%3D%26arnumber%3D4608934
//
//==============================================================================================

//----------------------------------------------------------------------------------------------

#include "imuread.h"

#ifdef USE_MAHONY_FUSION

//----------------------------------------------------------------------------------------------
// Definitions

#define twoKpDef	(2.0f * 0.02f)	// 2 * proportional gain
#define twoKiDef	(2.0f * 0.0f)	// 2 * integral gain

#define INV_SAMPLE_RATE  (1.0f / SENSORFS)

//----------------------------------------------------------------------------------------------
// Variable definitions

static float twoKp = twoKpDef;		// 2 * proportional gain (Kp)
static float twoKi = twoKiDef;		// 2 * integral gain (Ki)
static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; // quaternion of sensor frame relative to auxiliary frame
static float integralFBx = 0.0f,  integralFBy = 0.0f, integralFBz = 0.0f; // integral error terms scaled by Ki


//==============================================================================================
// Functions

static float invSqrt(float x);
static void mahony_init();
static void mahony_update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);
void mahony_updateIMU(float gx, float gy, float gz, float ax, float ay, float az);

static int reset_next_update=0;


void fusion_init(void)
{
    mahony_init();
}

void fusion_update(const AccelSensor_t *Accel, const MagSensor_t *Mag, const GyroSensor_t *Gyro,
    const MagCalibration_t *MagCal)
{
    Q_UNUSED(MagCal);
    int i;
    float ax, ay, az, gx, gy, gz, mx, my, mz;
    float factor = (float)(M_PI / 180.0);

    ax = Accel->Gp[0];
    ay = Accel->Gp[1];
    az = Accel->Gp[2];
    mx = Mag->Bc[0];
    my = Mag->Bc[1];
    mz = Mag->Bc[2];
    for (i=0; i < OVERSAMPLE_RATIO; i++) {
        gx = Gyro->YpFast[i][0];
        gy = Gyro->YpFast[i][1];
        gz = Gyro->YpFast[i][2];
        gx *= factor;
        gy *= factor;
        gz *= factor;
        mahony_update(gx, gy, gz, ax, ay, az, mx, my, mz);
    }
}

void fusion_read(Quaternion_t *q)
{
    q->q0 = q0;
    q->q1 = q1;
    q->q2 = q2;
    q->q3 = q3;
}


//----------------------------------------------------------------------------------------------
// AHRS algorithm update

static void mahony_init()
{
    static int first=1;

    twoKp = twoKpDef;	// 2 * proportional gain (Kp)
    twoKi = twoKiDef;	// 2 * integral gain (Ki)
    if (first) {
        q0 = 1.0f;
        q1 = 0.0f;	// TODO: set a flag to immediately capture
        q2 = 0.0f;	// magnetic orientation on next update
        q3 = 0.0f;
        first = 0;
    }
    reset_next_update = 1;
    integralFBx = 0.0f;
    integralFBy = 0.0f;
    integralFBz = 0.0f;
}

static void mahony_update(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz)
{
    float recipNorm;
    float q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;
    float hx, hy, bx, bz;
    float halfvx, halfvy, halfvz, halfwx, halfwy, halfwz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    // Use IMU algorithm if magnetometer measurement invalid
    // (avoids NaN in magnetometer normalisation)
    if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
        mahony_updateIMU(gx, gy, gz, ax, ay, az);
        return;
    }

    // Compute feedback only if accelerometer measurement valid
    // (avoids NaN in accelerometer normalisation)
    if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

        // Normalise accelerometer measurement
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Normalise magnetometer measurement
        recipNorm = invSqrt(mx * mx + my * my + mz * mz);
        mx *= recipNorm;
        my *= recipNorm;
        mz *= recipNorm;
#if 0
        // crazy experiement - no filter, just use magnetometer...
        q0 = 0;
        q1 = mx;
        q2 = my;
        q3 = mz;
        return;
#endif
        // Auxiliary variables to avoid repeated arithmetic
        q0q0 = q0 * q0;
        q0q1 = q0 * q1;
        q0q2 = q0 * q2;
        q0q3 = q0 * q3;
        q1q1 = q1 * q1;
        q1q2 = q1 * q2;
        q1q3 = q1 * q3;
        q2q2 = q2 * q2;
        q2q3 = q2 * q3;
        q3q3 = q3 * q3;

        // Reference direction of Earth's magnetic field
        hx = 2.0f * (mx * (0.5f - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2));
        hy = 2.0f * (mx * (q1q2 + q0q3) + my * (0.5f - q1q1 - q3q3) + mz * (q2q3 - q0q1));
        bx = sqrtf(hx * hx + hy * hy);
        bz = 2.0f * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5f - q1q1 - q2q2));

        // Estimated direction of gravity and magnetic field
        halfvx = q1q3 - q0q2;
        halfvy = q0q1 + q2q3;
        halfvz = q0q0 - 0.5f + q3q3;
        halfwx = bx * (0.5f - q2q2 - q3q3) + bz * (q1q3 - q0q2);
        halfwy = bx * (q1q2 - q0q3) + bz * (q0q1 + q2q3);
        halfwz = bx * (q0q2 + q1q3) + bz * (0.5f - q1q1 - q2q2);

        // Error is sum of cross product between estimated direction
        // and measured direction of field vectors
        halfex = (ay * halfvz - az * halfvy) + (my * halfwz - mz * halfwy);
        halfey = (az * halfvx - ax * halfvz) + (mz * halfwx - mx * halfwz);
        halfez = (ax * halfvy - ay * halfvx) + (mx * halfwy - my * halfwx);

        // Compute and apply integral feedback if enabled
        if(twoKi > 0.0f) {
            // integral error scaled by Ki
            integralFBx += twoKi * halfex * INV_SAMPLE_RATE;
            integralFBy += twoKi * halfey * INV_SAMPLE_RATE;
            integralFBz += twoKi * halfez * INV_SAMPLE_RATE;
            gx += integralFBx;	// apply integral feedback
            gy += integralFBy;
            gz += integralFBz;
        } else {
            integralFBx = 0.0f;	// prevent integral windup
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        }

        //printf("err =  %.3f, %.3f, %.3f\n", halfex, halfey, halfez);

        // Apply proportional feedback
        if (reset_next_update) {
            gx += 2.0f * halfex;
            gy += 2.0f * halfey;
            gz += 2.0f * halfez;
            reset_next_update = 0;
        } else {
            gx += twoKp * halfex;
            gy += twoKp * halfey;
            gz += twoKp * halfez;
        }
    }

    // Integrate rate of change of quaternion
    gx *= (0.5f * INV_SAMPLE_RATE);		// pre-multiply common factors
    gy *= (0.5f * INV_SAMPLE_RATE);
    gz *= (0.5f * INV_SAMPLE_RATE);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    // Normalise quaternion
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

//---------------------------------------------------------------------------------------------
// IMU algorithm update

void mahony_updateIMU(float gx, float gy, float gz, float ax, float ay, float az)
{
    float recipNorm;
    float halfvx, halfvy, halfvz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    // Compute feedback only if accelerometer measurement valid
    // (avoids NaN in accelerometer normalisation)
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

        // Normalise accelerometer measurement
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Estimated direction of gravity and vector perpendicular to magnetic flux
        halfvx = q1 * q3 - q0 * q2;
        halfvy = q0 * q1 + q2 * q3;
        halfvz = q0 * q0 - 0.5f + q3 * q3;

        // Error is sum of cross product between estimated and measured direction of gravity
        halfex = (ay * halfvz - az * halfvy);
        halfey = (az * halfvx - ax * halfvz);
        halfez = (ax * halfvy - ay * halfvx);

        // Compute and apply integral feedback if enabled
        if(twoKi > 0.0f) {
            // integral error scaled by Ki
            integralFBx += twoKi * halfex * INV_SAMPLE_RATE;
            integralFBy += twoKi * halfey * INV_SAMPLE_RATE;
            integralFBz += twoKi * halfez * INV_SAMPLE_RATE;
            gx += integralFBx;	// apply integral feedback
            gy += integralFBy;
            gz += integralFBz;
        } else {
            integralFBx = 0.0f;	// prevent integral windup
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        }

        // Apply proportional feedback
        gx += twoKp * halfex;
        gy += twoKp * halfey;
        gz += twoKp * halfez;
    }

    // Integrate rate of change of quaternion
    gx *= (0.5f * INV_SAMPLE_RATE);		// pre-multiply common factors
    gy *= (0.5f * INV_SAMPLE_RATE);
    gz *= (0.5f * INV_SAMPLE_RATE);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    // Normalise quaternion
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

//---------------------------------------------------------------------------------------------
// Fast inverse square-root
// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root

static float invSqrt(float x) {
    union {
        float f;
        int32_t i;
    } y;
    float halfx = 0.5f * x;

    y.f = x;
    y.i = 0x5f375a86 - (y.i >> 1);
    y.f = y.f * (1.5f - (halfx * y.f * y.f));
    y.f = y.f * (1.5f - (halfx * y.f * y.f));
    y.f = y.f * (1.5f - (halfx * y.f * y.f));
    return y.f;
}

//==============================================================================================
// END OF CODE
//==============================================================================================

#endif // USE_MAHONY_FUSION
