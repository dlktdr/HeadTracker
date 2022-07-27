/* This Algorithm provided by Paul_BB on Rc Groups.
 *   Details to be filled in here! TODO
 */

#include "dcmahrs.h"

#include <math.h>
#include <zephyr.h>

#include "defines.h"
#include "sense.h"

float kp = 0.2;
float ki = 0.01;

#define R2D RAD_TO_DEG  // Radians to degrees
#define D2R DEG_TO_RAD  // Degrees to radians

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

void DcmAHRSInitialize()
{
  // Initalize

  /* Initial Euler angles */
  /* ==================== */
  pan = 0. * D2R;
  tilt = 0. * D2R;
  roll = 0. * D2R;

  /* Initialize DCM */
  /* ============== */
  /* xe in body frame */
  rmat[0] = cos(pan) * cos(tilt);
  rmat[1] = -sin(pan) * cos(roll) + cos(pan) * sin(tilt) * sin(roll);
  rmat[2] = sin(pan) * sin(roll) + cos(pan) * sin(tilt) * cos(roll);
  /* ye in body frame */
  rmat[3] = sin(pan) * cos(tilt);
  rmat[4] = cos(pan) * cos(roll) + sin(pan) * sin(tilt) * sin(roll);
  rmat[5] = -cos(pan) * sin(roll) + sin(pan) * sin(tilt) * cos(roll);
  /* ze in body frame */
  rmat[6] = -sin(tilt);
  rmat[7] = cos(tilt) * sin(roll);
  rmat[8] = cos(tilt) * cos(roll);

  /* Update matrix rup = identity */
  /* ============================ */
  for (int i = 0; i < 9; i++) {
    rup[i] = 0.;
    rzero[i] = 0.f;
  }
  rup[0] = 1.;
  rup[4] = 1.;
  rup[8] = 1.;
  rzero[0] = 1.0f;
  rzero[4] = 1.0f;
  rzero[8] = 1.0f;

  /* tilt-roll correction */
  /* ===================== */
  for (int i = 0; i < 3; i++) omegacorrPAcc[i] = 0.;

  /* pan correction */
  /* ============== */
  for (int i = 0; i < 3; i++) omegacorrPMag[i] = 0.;

  /* Gyros bias */
  /* ========== */
  for (int i = 0; i < 3; i++) biasGyros[i] = 0.;

  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
  // -------------------------------------------------------------------------
}

void DCMOneSecThread()
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
  // Don't do calulations if no accel or mag data, will result in Nan
  if (fabs(u1[0] + u1[1] + u1[2]) < 0.0001) return;
  if (fabs(u2[0] + u2[1] + u2[2]) < 0.0001) return;

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
    for (int i = 0; i < 3; i++) V_buff[i] *= sqrt(f_buff);

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

  /* -asin(rmat31) */
  tilt = atan2(rout[7], rout[8]) * RAD_TO_DEG;

  /* atan2(rmat32,rmat33) */
  roll = -asin(rout[6]) * RAD_TO_DEG;
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
void DcmSetKp(float p)
{
  if (p > 0 && p < 1) kp = p;
}

void DcmSetKi(float i)
{
  if (i > 0 && i < 1) ki = i;
}

K_THREAD_DEFINE(DCMOneSecThread_id, 1024, DCMOneSecThread, NULL, NULL, NULL, CALCULATE_THREAD_PRIO,
                K_FP_REGS, 1000);
