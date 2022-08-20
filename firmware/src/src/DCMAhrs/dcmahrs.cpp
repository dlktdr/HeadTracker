/* This Algorithm provided by Paul_BB on Rc Groups.
 *   Details to be filled in here! TODO
 */

#include "dcmahrs.h"

#include <math.h>
#include <zephyr.h>

#include "defines.h"
#include "sense.h"
#include "trackersettings.h"

#define R2D RAD_TO_DEG  // Radians to degrees
#define D2R DEG_TO_RAD  // Degrees to radians

bool DCMInitFlag = true;
float tilt, roll, pan;
float rzero[9];
float rmat[9];
float rup[9];
float errorRP[3];
float omegacorrPAcc[3];
float omegacorrPMag[3];
float biasGyros[3];
float dirovergndHB[3];
float dirovergndHMag[3];
float errorYawground[3];
float errorYawplane[3];

float omegatotal[3], theta[3];
float MagvecEarth[3];
float V_buff[3];
float r_buff[9];
float f_buff;

//----------------------------------------------------------------------
// Calculations and Main Channel Thread
//----------------------------------------------------------------------

/*============================================================================*/
/*============================================================================*/
/*                             Math routines                                  */
/*============================================================================*/
/*============================================================================*/
/*----------------------------------------------------------------------------*/
static void MatrixMultiply(float *M1, float *M2)
/*----------------------------------------------------------------------------*/
/* M1 = M1*M2                                                                 */
/*----------------------------------------------------------------------------*/
{
  /* Declarations */
  float M_buff[9];

  for (int i = 0; i < 9; i++) M_buff[i] = M1[i];

  M1[0] = M_buff[0] * M2[0] + M_buff[1] * M2[3] + M_buff[2] * M2[6];
  M1[3] = M_buff[3] * M2[0] + M_buff[4] * M2[3] + M_buff[5] * M2[6];
  M1[6] = M_buff[6] * M2[0] + M_buff[7] * M2[3] + M_buff[8] * M2[6];

  M1[1] = M_buff[0] * M2[1] + M_buff[1] * M2[4] + M_buff[2] * M2[7];
  M1[4] = M_buff[3] * M2[1] + M_buff[4] * M2[4] + M_buff[5] * M2[7];
  M1[7] = M_buff[6] * M2[1] + M_buff[7] * M2[4] + M_buff[8] * M2[7];

  M1[2] = M_buff[0] * M2[2] + M_buff[1] * M2[5] + M_buff[2] * M2[8];
  M1[5] = M_buff[3] * M2[2] + M_buff[4] * M2[5] + M_buff[5] * M2[8];
  M1[8] = M_buff[6] * M2[2] + M_buff[7] * M2[5] + M_buff[8] * M2[8];
}

/*----------------------------------------------------------------------------*/
static void TransposeMatrix(float *M1, float *M2)
/*----------------------------------------------------------------------------*/
/* M2 = Transpose M1                                                          */
/*----------------------------------------------------------------------------*/
{
  M2[0] = M1[0];
  M2[1] = M1[3];
  M2[2] = M1[6];
  M2[3] = M1[1];
  M2[4] = M1[4];
  M2[5] = M1[7];
  M2[6] = M1[2];
  M2[7] = M1[5];
  M2[8] = M1[8];
}

/*----------------------------------------------------------------------------*/
static void MatrixVector(float *M, float *V1, float *V2)
/*----------------------------------------------------------------------------*/
/* V2 = M*V1                                                                  */
/*----------------------------------------------------------------------------*/
{
  V2[0] = M[0] * V1[0] + M[1] * V1[1] + M[2] * V1[2];
  V2[1] = M[3] * V1[0] + M[4] * V1[1] + M[5] * V1[2];
  V2[2] = M[6] * V1[0] + M[7] * V1[1] + M[8] * V1[2];
}

/*----------------------------------------------------------------------------*/
static void VectorDotProduct(float *V1, float *V2, float *Scalar_Product)
/*----------------------------------------------------------------------------*/
/* Scalar_Product = V1.V2                                                     */
/*----------------------------------------------------------------------------*/
{
  *Scalar_Product = 0.;
  for (int i = 0; i < 3; i++) *Scalar_Product += V1[i] * V2[i];
}

/*----------------------------------------------------------------------------*/
static void VectorCross(float *V1, float *V2, float *V3)
/*----------------------------------------------------------------------------*/
/* V3 = V1xV2                                                                 */
/*----------------------------------------------------------------------------*/
{
  V3[0] = V1[1] * V2[2] - V1[2] * V2[1];
  V3[1] = V1[2] * V2[0] - V1[0] * V2[2];
  V3[2] = V1[0] * V2[1] - V1[1] * V2[0];
}

/*----------------------------------------------------------------------------*/
static void VectorAdd(float *V1, float *V2, float *V3)
/*----------------------------------------------------------------------------*/
/* V3 = V1+V2                                                                 */
/*----------------------------------------------------------------------------*/
{
  /* Declarations */
  for (int i = 0; i < 3; i++) V3[i] = V1[i] + V2[i];
}

static void MatrixMultiply2(float *M1, float *M2, float *M3)
/*----------------------------------------------------------------------------*/
/* M3 = M1*M2                                                                 */
/*----------------------------------------------------------------------------*/
{
  M3[0] = M1[0] * M2[0] + M1[1] * M2[3] + M1[2] * M2[6];
  M3[3] = M1[3] * M2[0] + M1[4] * M2[3] + M1[5] * M2[6];
  M3[6] = M1[6] * M2[0] + M1[7] * M2[3] + M1[8] * M2[6];

  M3[1] = M1[0] * M2[1] + M1[1] * M2[4] + M1[2] * M2[7];
  M3[4] = M1[3] * M2[1] + M1[4] * M2[4] + M1[5] * M2[7];
  M3[7] = M1[6] * M2[1] + M1[7] * M2[4] + M1[8] * M2[7];

  M3[2] = M1[0] * M2[2] + M1[1] * M2[5] + M1[2] * M2[8];
  M3[5] = M1[3] * M2[2] + M1[4] * M2[5] + M1[5] * M2[8];
  M3[8] = M1[6] * M2[2] + M1[7] * M2[5] + M1[8] * M2[8];
}

/*----------------------------------------------------------------------------*/
void DcmAHRSInitialize(float u0[3], float u1[3], float u2[3])
/*----------------------------------------------------------------------------*/
/* This routine computes the initial tilt roll pan                            */
/* and initializes the DCM algorithm                                          */
/* The Rzero matrix is set to the DCM matrix so that tilt roll pan are equal  */
/* to zero after boot                                                         */
/*----------------------------------------------------------------------------*/
{
  /* Declarations */
  float tilt0, roll0, pan0;
  float BeX, BeY;
  float Ct, St, Cr, Sr, Cp, Sp;

  /* Compute initial orientation relatively to Earth NED frame */
  /* X local Magnetic North                                    */
  /* Y east                                                    */
  /* Z down                                                    */
  /* ========================================================= */
  /* ========================================================= */
  /* Compute tilt and roll with gravity vector */
  /* ========================================= */
  /* Read body acceleration (x y z accelerometers) */
  for (int i = 0; i < 3; i++) V_buff[i] = u1[i];

  /* Normalize body acceleration vector */
  VectorDotProduct(V_buff, V_buff, &f_buff);
  f_buff = 1. / sqrt(f_buff);
  if (f_buff > 0.001)
    for (int i = 0; i < 3; i++) V_buff[i] *= f_buff;

  /* Tilt and roll in radians */
  tilt0 = -asin(V_buff[0]);
  roll0 = atan2(V_buff[1], V_buff[2]);

  /* Compute pan with Magnetic North vector */
  /* ====================================== */
  /* Straighten Magnetometer x and y (remove tilt and roll) */
  Ct = cos(tilt0);
  St = sin(tilt0);
  Cr = cos(roll0);
  Sr = sin(roll0);

  BeX = Ct * u2[0] + St * Sr * u2[1] + St * Cr * u2[2];
  BeY = -Cr * u2[1] + Sr * u2[2];

  /* pan in radians */
  /* No need to normalize because atan2 is a ratio */
  pan0 = atan2(BeY, BeX);

  /* Initialize DCM algorithm */
  /* ======================== */
  /* ======================== */
  /* Initialize DCM */
  /* ============== */
  Cp = cos(pan0);
  Sp = sin(pan0);

  /* xe in body frame */
  rmat[0] = Cp * Ct;
  rmat[1] = -Sp * Cr + Cp * St * Sr;
  rmat[2] = Sp * Sr + Cp * St * Cr;
  /* ye in body frame */
  rmat[3] = Sp * Ct;
  rmat[4] = Cp * Cr + Sp * St * Sr;
  rmat[5] = -Cp * Sr + Sp * St * Cr;
  /* ze in body frame */
  rmat[6] = -St;
  rmat[7] = Ct * Sr;
  rmat[8] = Ct * Cr;

  /* Update matrix rup = identity */
  /* ============================ */
  rup[0] = 1.;
  rup[4] = 1.;
  rup[8] = 1.;

  /* Zero matrix */
  /* Thus at the first DcmCalculate call tilt = roll = pan = 0 */
  /* ========================================================= */
  for (int i = 0; i < 9; i++) rzero[i] = rmat[i];

  /* tilt-roll correction */
  /* ===================== */
  for (int i = 0; i < 3; i++) omegacorrPAcc[i] = 0.;

  /* pan correction */
  /* ============== */
  for (int i = 0; i < 3; i++) omegacorrPMag[i] = 0.;

  /* Gyros bias */
  /* ========== */
  for (int i = 0; i < 3; i++) biasGyros[i] = -u0[i];
}

/*----------------------------------------------------------------------------*/

void DCMnormalization_Thread()
{
  while (1) {
    rt_sleep_ms(1000);
    // Use a mutex so sensor data can't be updated part way
    k_mutex_lock(&sensor_mutex, K_FOREVER);

    /* Orthogonalization */
    /* ================= */
    /* ================= */
    /* (U,V) */
    VectorDotProduct(&rmat[0], &rmat[3], &f_buff);
    f_buff /= 2.;

    /* U = U - 0.5*V(U,V) */
    for (int i = 0; i < 3; i++) V_buff[i] = rmat[i];
    for (int i = 0; i < 3; i++) rmat[i] -= rmat[3 + i] * f_buff;

    /* V = V - 0.5*U(U,V) */
    for (int i = 0; i < 3; i++) rmat[3 + i] -= V_buff[i] * f_buff;

    /* W = UxV */
    VectorCross(&rmat[0], &rmat[3], &rmat[6]);

    /* U scaling */
    VectorDotProduct(&rmat[0], &rmat[0], &f_buff);
    f_buff = 1. / sqrt(f_buff);
    for (int i = 0; i < 3; i++) rmat[i] = rmat[i] * f_buff;

    /* V scaling */
    VectorDotProduct(&rmat[3], &rmat[3], &f_buff);
    f_buff = 1. / sqrt(f_buff);
    for (int i = 0; i < 3; i++) rmat[3 + i] = rmat[3 + i] * f_buff;

    /* W scaling */
    VectorDotProduct(&rmat[6], &rmat[6], &f_buff);
    f_buff = 1. / sqrt(f_buff);
    for (int i = 0; i < 3; i++) rmat[6 + i] = rmat[6 + i] * f_buff;

    // Free Mutex Lock, Allow sensor updates
    k_mutex_unlock(&sensor_mutex);
  }
}

void DcmCalculate(float u0[3], float u1[3], float u2[3], float deltat)
{
  float kp = trkset.Kp();
  float ki = trkset.Ki();
  // Don't do calulations if no accel or mag data, will result in Nan
  if (fabs(u1[0] + u1[1] + u1[2]) < 0.0001) return;
  if (fabs(u2[0] + u2[1] + u2[2]) < 0.0001) return;

  /* R matrix initialization */
  /* ======================= */
  if (DCMInitFlag == true) {
    DcmAHRSInitialize(u0, u1, u2);
    DCMInitFlag = false;
  }

  /* R matrix update */
  /* =============== */
  /* =============== */
  /* Read body angular velocity (p q r gyrometers) */
  for (int i = 0; i < 3; i++) omegatotal[i] = u0[i];

  /* Add pitch-roll and yaw corrections */
  VectorAdd(omegatotal, omegacorrPAcc, omegatotal);
  VectorAdd(omegatotal, omegacorrPMag, omegatotal);
  VectorAdd(omegatotal, biasGyros, omegatotal);

  /* Integrate angular velocity over the 25ms time step */
  for (int i = 0; i < 3; i++) theta[i] = omegatotal[i] * deltat;

  /* Assemble equivalent small rotation matrix (update matrix) */
  rup[1] = -theta[2];
  rup[2] = theta[1];
  rup[3] = theta[2];
  rup[5] = -theta[0];
  rup[6] = -theta[1];
  rup[7] = theta[0];

  /* Rotate body frame */
  MatrixMultiply(rmat, rup);

  /* roll-tilt correction */
  /* ===================== */
  /* ===================== */
  /* Read body acceleration (x y z accelerometers) */
  for (int i = 0; i < 3; i++) V_buff[i] = u1[i];

  /* Normalize body acceleration vector */
  /* Note: if f_buff close to zero then errorRP will be close to zero */
  VectorDotProduct(V_buff, V_buff, &f_buff);
  f_buff = 1. / sqrt(f_buff);
  if (f_buff > 0.001)
    for (int i = 0; i < 3; i++) V_buff[i] *= f_buff;

  /* Compute the roll-pitch error vector: cross product of measured */
  /* earth Z vector with estimated earth vector expressed in body   */
  /* frame (3rd row of rmat)                                        */
  VectorCross(V_buff, &rmat[6], errorRP);

  /* Compute pitch-roll correction proportional term */
  for (int i = 0; i < 3; i++) omegacorrPAcc[i] = errorRP[i] * kp;

  /* Add pitch-roll error to integrator */
  for (int i = 0; i < 3; i++) biasGyros[i] += errorRP[i] * ki * deltat;

  /* pan correction */
  /* ============== */
  /* ============== */
  /* Read the magnetometer vector */
  for (int i = 0; i < 3; i++) V_buff[i] = u2[i];

  /* Express the magnetometer vector in the Earth frame */
  MatrixVector(rmat, V_buff, MagvecEarth);

  /* Horizontal component of the magnetometer vector in the Earth frame */
  for (int i = 0; i < 2; i++) dirovergndHMag[i] = MagvecEarth[i];
  dirovergndHMag[2] = 0.;

  /* Normalization */
  f_buff = dirovergndHMag[0] * dirovergndHMag[0] + dirovergndHMag[1] * dirovergndHMag[1];
  f_buff = 1. / sqrt(f_buff);
  for (int i = 0; i < 2; i++) dirovergndHMag[i] *= f_buff;

  /* Horizontal component of Earth magnetic vector */
  /* The Earth magnetic horizontal vector is the reference for yaw=0 */
  dirovergndHB[0] = 1.;
  dirovergndHB[1] = 0.;
  dirovergndHB[2] = 0.;

  /* Compute the yaw error vector expressed in earth frame */
  VectorCross(dirovergndHMag, dirovergndHB, errorYawground);

  /* Express the yaw error vector in the body frame */
  TransposeMatrix(rmat, r_buff);
  MatrixVector(r_buff, errorYawground, errorYawplane);

  /* Compute yaw correction proportional term */
  for (int i = 0; i < 3; i++) omegacorrPMag[i] = errorYawplane[i] * kp;

  /* Update gyros bias */
  for (int i = 0; i < 3; i++) biasGyros[i] += errorYawplane[i] * ki * deltat;

  /* Transpose rzero matrix */
  TransposeMatrix(rzero, r_buff);

  /* Multiply transposed rzero with rmat */
  float rout[9];
  MatrixMultiply2(r_buff, rmat, rout);

  /* Euler angles from DCM */
  /* ===================== */
  /* atan2(rmat31,rmat11) */
  pan = atan2(rout[3], rout[0]) * RAD_TO_DEG;

  /* atan2(rmat32,rmat33) */
  tilt = -asin(rout[6]) * RAD_TO_DEG;

  /* -asin(rmat31) */
  roll = atan2(rout[7], rout[8]) * RAD_TO_DEG;
}

void DcmAhrsResetCenter()
{
  /* Center reset */
  /* ============ */
  for (int i = 0; i < 9; i++) {
    rzero[i] = rmat[i];
  }
}

float DcmGetTilt() { return tilt; }
float DcmGetRoll() { return roll; }
float DcmGetPan() { return pan; }
