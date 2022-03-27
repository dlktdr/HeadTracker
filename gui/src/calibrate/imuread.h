#ifndef IMUREAD_H
#define IMUREAD_H

#include <QObject>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class imuread : public QObject
{
    Q_OBJECT
public:
    explicit imuread(QObject *parent = nullptr);

signals:

};

#if defined(LINUX)
  #include <termios.h>
  #include <unistd.h>
  #include <GL/gl.h>  // sudo apt install mesa-common-dev
#elif defined(WINDOWS)
  #include <windows.h>
  #include <GL/gl.h>
  #define random() rand()
  #ifndef M_PI
    #define M_PI 3.14159265358979323846
  #endif
#elif defined(MACOS)
  #include <termios.h>
  #include <unistd.h>
  #include <OpenGL/gl.h>
#endif

#if defined(LINUX)
  #define PORT "/dev/ttyACM0"
#elif defined(WINDOWS)
  #define PORT "COM3"
#elif defined(MACOSX)
  #define PORT "/dev/cu.usbmodemfd132"
#endif

#define TIMEOUT_MSEC 14

#define MAGBUFFSIZE 650 // Freescale's lib needs at least 392

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    float x;
    float y;
    float z;
    //int valid;
} Point_t;

typedef struct {
    float q0; // w
    float q1; // x
    float q2; // y
    float q3; // z
} Quaternion_t;
extern Quaternion_t current_orientation;

extern int port_is_open(void);
extern int open_port(const char *name);
extern int read_serial_data(void);
extern int write_serial_data(const void *ptr, int len);
extern void close_port(void);
void raw_data_reset(void);
void cal1_data(const float *data);
void cal2_data(const float *data);
void calibration_confirmed(void);
void raw_data(const int16_t *data);
int send_calibration(void);
void visualize_init(void);
void apply_calibration(int16_t rawx, int16_t rawy, int16_t rawz, Point_t *out);
void display_callback(void);
void resize_callback(int width, int height);
int MagCal_Run(void);
void quality_reset(void);
void quality_update(const Point_t *point);
float quality_surface_gap_error(void);
float quality_magnitude_variance_error(void);
float quality_wobble_error(void);
float quality_spherical_fit_error(void);

// magnetic calibration & buffer structure
typedef struct {
    float V[3];                  // current hard iron offset x, y, z, (uT)
    float invW[3][3];            // current inverse soft iron matrix
    float B;                     // current geomagnetic field magnitude (uT)
    float FourBsq;               // current 4*B*B (uT^2)
    float FitError;              // current fit error %
    float FitErrorAge;           // current fit error % (grows automatically with age)
    float trV[3];                // trial value of hard iron offset z, y, z (uT)
    float trinvW[3][3];          // trial inverse soft iron matrix size
    float trB;                   // trial value of geomagnetic field magnitude in uT
    float trFitErrorpc;          // trial value of fit error %
    float A[3][3];               // ellipsoid matrix A
    float invA[3][3];            // inverse of ellipsoid matrix A
    float matA[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float matB[10][10];          // scratch 10x10 matrix used by calibration algorithms
    float vecA[10];              // scratch 10x1 vector used by calibration algorithms
    float vecB[4];               // scratch 4x1 vector used by calibration algorithms
    int8_t ValidMagCal;          // integer value 0, 4, 7, 10 denoting both valid calibration and solver used
    int16_t BpFast[3][MAGBUFFSIZE];   // uncalibrated magnetometer readings
    int8_t  valid[MAGBUFFSIZE];        // 1=has data, 0=empty slot
    int16_t MagBufferCount;           // number of magnetometer readings
} MagCalibration_t;

extern MagCalibration_t magcal;


void f3x3matrixAeqI(float A[][3]);
void fmatrixAeqI(float *A[], int16_t rc);
void f3x3matrixAeqScalar(float A[][3], float Scalar);
void f3x3matrixAeqInvSymB(float A[][3], float B[][3]);
void f3x3matrixAeqAxScalar(float A[][3], float Scalar);
void f3x3matrixAeqMinusA(float A[][3]);
float f3x3matrixDetA(float A[][3]);
void eigencompute(float A[][10], float eigval[], float eigvec[][10], int8_t n);
void fmatrixAeqInvA(float *A[], int8_t iColInd[], int8_t iRowInd[], int8_t iPivot[], int8_t isize);
void fmatrixAeqRenormRotA(float A[][3]);


#define SENSORFS 100
#define OVERSAMPLE_RATIO 4


// accelerometer sensor structure definition
#define G_PER_COUNT 0.0001220703125F  // = 1/8192
typedef struct
{
    float Gp[3];           // slow (typically 25Hz) averaged readings (g)
    float GpFast[3];       // fast (typically 200Hz) readings (g)
} AccelSensor_t;

// magnetometer sensor structure definition
#define UT_PER_COUNT 0.1F
typedef struct
{
    float Bc[3];           // slow (typically 25Hz) averaged calibrated readings (uT)
    float BcFast[3];       // fast (typically 200Hz) calibrated readings (uT)
} MagSensor_t;

// gyro sensor structure definition
#define DEG_PER_SEC_PER_COUNT 0.0625F  // = 1/16
typedef struct
{
    float Yp[3];                           // raw gyro sensor output (deg/s)
    float YpFast[OVERSAMPLE_RATIO][3];     // fast (typically 200Hz) readings
} GyroSensor_t;


//#define USE_NXP_FUSION
#define USE_MAHONY_FUSION

void fusion_init(void);
void fusion_update(const AccelSensor_t *Accel, const MagSensor_t *Mag, const GyroSensor_t *Gyro,
    const MagCalibration_t *MagCal);
void fusion_read(Quaternion_t *q);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IMUREAD_H
