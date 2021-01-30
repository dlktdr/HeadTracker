// Copyright (c) 2014, Freescale Semiconductor, Inc.
// All rights reserved.
// vim: set ts=4:
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Freescale Semiconductor, Inc. nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FREESCALE SEMICONDUCTOR, INC. BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file contains magnetic calibration functions.  It is STRONGLY RECOMMENDED
// that the casual developer NOT TOUCH THIS FILE.  The mathematics behind this file
// is extremely complex, and it will be very easy (almost inevitable) that you screw
// it up.
//
// Haha - This file has been edited!  Please do not blame or pester NXP (formerly
//        Freescale) about the "almost inevitable" issues!

#include "imuread.h"

#define FXOS8700_UTPERCOUNT  0.1f
#define DEFAULTB 50.0F				// default geomagnetic field (uT)
#define X 0                         // vector components
#define Y 1
#define Z 2
#define ONETHIRD 0.33333333F        // one third
#define ONESIXTH 0.166666667F       // one sixth
#define MINMEASUREMENTS4CAL 40      // minimum number of measurements for 4 element calibration
#define MINMEASUREMENTS7CAL 100     // minimum number of measurements for 7 element calibration
#define MINMEASUREMENTS10CAL 150    // minimum number of measurements for 10 element calibration
#define MINBFITUT 22.0F             // minimum geomagnetic field B (uT) for valid calibration
#define MAXBFITUT 67.0F             // maximum geomagnetic field B (uT) for valid calibration
#define FITERRORAGINGSECS 7200.0F   // 2 hours: time for fit error to increase (age) by e=2.718

static void fUpdateCalibration4INV(MagCalibration_t *MagCal);
static void fUpdateCalibration7EIG(MagCalibration_t *MagCal);
static void fUpdateCalibration10EIG(MagCalibration_t *MagCal);



// run the magnetic calibration
int MagCal_Run(void)
{
    int i, j;			// loop counters
    int isolver;		// magnetic solver used
    int count=0;
    static int waitcount=0;

    // only do the calibration occasionally
    if (++waitcount < 20) return 0;
    waitcount = 0;

    // count number of data points
    for (i=0; i < MAGBUFFSIZE; i++) {
        if (magcal.valid[i]) count++;
    }

    if (count < MINMEASUREMENTS4CAL) return 0;

    if (magcal.ValidMagCal) {
        // age the existing fit error to avoid one good calibration locking out future updates
        magcal.FitErrorAge *= 1.02f;
    }

    // is enough data collected
    if (count < MINMEASUREMENTS7CAL) {
        isolver = 4;
        fUpdateCalibration4INV(&magcal); // 4 element matrix inversion calibration
        if (magcal.trFitErrorpc < 12.0f) magcal.trFitErrorpc = 12.0f;
    } else if (count < MINMEASUREMENTS10CAL) {
        isolver = 7;
        fUpdateCalibration7EIG(&magcal); // 7 element eigenpair calibration
        if (magcal.trFitErrorpc < 7.5f) magcal.trFitErrorpc = 7.5f;
    } else {
        isolver = 10;
        fUpdateCalibration10EIG(&magcal); // 10 element eigenpair calibration
    }

    // the trial geomagnetic field must be in range (earth is 22uT to 67uT)
    if ((magcal.trB >= MINBFITUT) && (magcal.trB <= MAXBFITUT))	{
        // always accept the calibration if
        //  1: no previous calibration exists
        //  2: the calibration fit is reduced or
        //  3: an improved solver was used giving a good trial calibration (4% or under)
        if ((magcal.ValidMagCal == 0) ||
                (magcal.trFitErrorpc <= magcal.FitErrorAge) ||
                ((isolver > magcal.ValidMagCal) && (magcal.trFitErrorpc <= 4.0F))) {
            // accept the new calibration solution
            //printf("new magnetic cal, B=%.2f uT\n", magcal.trB);
            magcal.ValidMagCal = isolver;
            magcal.FitError = magcal.trFitErrorpc;
            if (magcal.trFitErrorpc > 2.0f) {
                magcal.FitErrorAge = magcal.trFitErrorpc;
            } else {
                magcal.FitErrorAge = 2.0f;
            }
            magcal.B = magcal.trB;
            magcal.FourBsq = 4.0F * magcal.trB * magcal.trB;
            for (i = X; i <= Z; i++) {
                magcal.V[i] = magcal.trV[i];
                for (j = X; j <= Z; j++) {
                    magcal.invW[i][j] = magcal.trinvW[i][j];
                }
            }
            return 1; // indicates new calibration applied
        }
    }
    return 0;
}



// 4 element calibration using 4x4 matrix inverse
static void fUpdateCalibration4INV(MagCalibration_t *MagCal)
{
    float fBp2;					// fBp[X]^2+fBp[Y]^2+fBp[Z]^2
    float fSumBp4;				// sum of fBp2
    float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
    float fE;					// error function = r^T.r
    int16_t iOffset[3];			// offset to remove large DC hard iron bias in matrix
    int16_t iCount;				// number of measurements counted
    int i, j, k;				// loop counters

    // working arrays for 4x4 matrix inversion
    float *pfRows[4];
    int8_t iColInd[4];
    int8_t iRowInd[4];
    int8_t iPivot[4];

    // compute fscaling to reduce multiplications later
    fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

    // the trial inverse soft iron matrix invW always equals
    // the identity matrix for 4 element calibration
    f3x3matrixAeqI(MagCal->trinvW);

    // zero fSumBp4=Y^T.Y, vecB=X^T.Y (4x1) and on and above
    // diagonal elements of matA=X^T*X (4x4)
    fSumBp4 = 0.0F;
    for (i = 0; i < 4; i++) {
        MagCal->vecB[i] = 0.0F;
        for (j = i; j < 4; j++) {
            MagCal->matA[i][j] = 0.0F;
        }
    }

    // the offsets are guaranteed to be set from the first element but to avoid compiler error
    iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

    // use from MINEQUATIONS up to MAXEQUATIONS entries from magnetic buffer to compute matrices
    iCount = 0;
    for (j = 0; j < MAGBUFFSIZE; j++) {
        if (MagCal->valid[j]) {
            // use first valid magnetic buffer entry as estimate (in counts) for offset
            if (iCount == 0) {
                for (k = X; k <= Z; k++) {
                    iOffset[k] = MagCal->BpFast[k][j];
                }
            }

            // store scaled and offset fBp[XYZ] in vecA[0-2] and fBp[XYZ]^2 in vecA[3-5]
            for (k = X; k <= Z; k++) {
                MagCal->vecA[k] = (float)((int32_t)MagCal->BpFast[k][j]
                    - (int32_t)iOffset[k]) * fscaling;
                MagCal->vecA[k + 3] = MagCal->vecA[k] * MagCal->vecA[k];
            }

            // calculate fBp2 = Bp[X]^2 + Bp[Y]^2 + Bp[Z]^2 (scaled uT^2)
            fBp2 = MagCal->vecA[3] + MagCal->vecA[4] + MagCal->vecA[5];

            // accumulate fBp^4 over all measurements into fSumBp4=Y^T.Y
            fSumBp4 += fBp2 * fBp2;

            // now we have fBp2, accumulate vecB[0-2] = X^T.Y =sum(Bp2.Bp[XYZ])
            for (k = X; k <= Z; k++) {
                MagCal->vecB[k] += MagCal->vecA[k] * fBp2;
            }

            //accumulate vecB[3] = X^T.Y =sum(fBp2)
            MagCal->vecB[3] += fBp2;

            // accumulate on and above-diagonal terms of matA = X^T.X ignoring matA[3][3]
            MagCal->matA[0][0] += MagCal->vecA[X + 3];
            MagCal->matA[0][1] += MagCal->vecA[X] * MagCal->vecA[Y];
            MagCal->matA[0][2] += MagCal->vecA[X] * MagCal->vecA[Z];
            MagCal->matA[0][3] += MagCal->vecA[X];
            MagCal->matA[1][1] += MagCal->vecA[Y + 3];
            MagCal->matA[1][2] += MagCal->vecA[Y] * MagCal->vecA[Z];
            MagCal->matA[1][3] += MagCal->vecA[Y];
            MagCal->matA[2][2] += MagCal->vecA[Z + 3];
            MagCal->matA[2][3] += MagCal->vecA[Z];

            // increment the counter for next iteration
            iCount++;
        }
    }

    // set the last element of the measurement matrix to the number of buffer elements used
    MagCal->matA[3][3] = (float) iCount;

    // store the number of measurements accumulated
    MagCal->MagBufferCount = iCount;

    // use above diagonal elements of symmetric matA to set both matB and matA to X^T.X
    for (i = 0; i < 4; i++) {
        for (j = i; j < 4; j++) {
            MagCal->matB[i][j] = MagCal->matB[j][i]
                = MagCal->matA[j][i] = MagCal->matA[i][j];
        }
    }

    // calculate in situ inverse of matB = inv(X^T.X) (4x4) while matA still holds X^T.X
    for (i = 0; i < 4; i++) {
        pfRows[i] = MagCal->matB[i];
    }
    fmatrixAeqInvA(pfRows, iColInd, iRowInd, iPivot, 4);

    // calculate vecA = solution beta (4x1) = inv(X^T.X).X^T.Y = matB * vecB
    for (i = 0; i < 4; i++) {
        MagCal->vecA[i] = 0.0F;
        for (k = 0; k < 4; k++) {
            MagCal->vecA[i] += MagCal->matB[i][k] * MagCal->vecB[k];
        }
    }

    // calculate P = r^T.r = Y^T.Y - 2 * beta^T.(X^T.Y) + beta^T.(X^T.X).beta
    // = fSumBp4 - 2 * vecA^T.vecB + vecA^T.matA.vecA
    // first set P = Y^T.Y - 2 * beta^T.(X^T.Y) = SumBp4 - 2 * vecA^T.vecB
    fE = 0.0F;
    for (i = 0; i < 4; i++) {
        fE += MagCal->vecA[i] * MagCal->vecB[i];
    }
    fE = fSumBp4 - 2.0F * fE;

    // set vecB = (X^T.X).beta = matA.vecA
    for (i = 0; i < 4; i++) {
        MagCal->vecB[i] = 0.0F;
        for (k = 0; k < 4; k++) {
            MagCal->vecB[i] += MagCal->matA[i][k] * MagCal->vecA[k];
        }
    }

    // complete calculation of P by adding beta^T.(X^T.X).beta = vecA^T * vecB
    for (i = 0; i < 4; i++) {
        fE += MagCal->vecB[i] * MagCal->vecA[i];
    }

    // compute the hard iron vector (in uT but offset and scaled by FMATRIXSCALING)
    for (k = X; k <= Z; k++) {
        MagCal->trV[k] = 0.5F * MagCal->vecA[k];
    }

    // compute the scaled geomagnetic field strength B (in uT but scaled by FMATRIXSCALING)
    MagCal->trB = sqrtf(MagCal->vecA[3] + MagCal->trV[X] * MagCal->trV[X] +
            MagCal->trV[Y] * MagCal->trV[Y] + MagCal->trV[Z] * MagCal->trV[Z]);

    // calculate the trial fit error (percent) normalized to number of measurements
    // and scaled geomagnetic field strength
    MagCal->trFitErrorpc = sqrtf(fE / (float) MagCal->MagBufferCount) * 100.0F /
            (2.0F * MagCal->trB * MagCal->trB);

    // correct the hard iron estimate for FMATRIXSCALING and the offsets applied (result in uT)
    for (k = X; k <= Z; k++) {
        MagCal->trV[k] = MagCal->trV[k] * DEFAULTB
            + (float)iOffset[k] * FXOS8700_UTPERCOUNT;
    }

    // correct the geomagnetic field strength B to correct scaling (result in uT)
    MagCal->trB *= DEFAULTB;
}










// 7 element calibration using direct eigen-decomposition
static void fUpdateCalibration7EIG(MagCalibration_t *MagCal)
{
    float det;					// matrix determinant
    float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
    float ftmp;					// scratch variable
    int16_t iOffset[3];			// offset to remove large DC hard iron bias
    int16_t iCount;				// number of measurements counted
    int i, j, k, m, n;			// loop counters

    // compute fscaling to reduce multiplications later
    fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

    // the offsets are guaranteed to be set from the first element but to avoid compiler error
    iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

    // zero the on and above diagonal elements of the 7x7 symmetric measurement matrix matA
    for (m = 0; m < 7; m++) {
        for (n = m; n < 7; n++) {
            MagCal->matA[m][n] = 0.0F;
        }
    }

    // place from MINEQUATIONS to MAXEQUATIONS entries into product matrix matA
    iCount = 0;
    for (j = 0; j < MAGBUFFSIZE; j++) {
        if (MagCal->valid[j]) {
            // use first valid magnetic buffer entry as offset estimate (bit counts)
            if (iCount == 0) {
                for (k = X; k <= Z; k++) {
                    iOffset[k] = MagCal->BpFast[k][j];
                }
            }

            // apply the offset and scaling and store in vecA
            for (k = X; k <= Z; k++) {
                MagCal->vecA[k + 3] = (float)((int32_t)MagCal->BpFast[k][j]
                    - (int32_t)iOffset[k]) * fscaling;
                MagCal->vecA[k] = MagCal->vecA[k + 3] * MagCal->vecA[k + 3];
            }

            // accumulate the on-and above-diagonal terms of
            // MagCal->matA=Sigma{vecA^T * vecA}
            // with the exception of matA[6][6] which will sum to the number
            // of measurements and remembering that vecA[6] equals 1.0F
            // update the right hand column [6] of matA except for matA[6][6]
            for (m = 0; m < 6; m++) {
                MagCal->matA[m][6] += MagCal->vecA[m];
            }
            // update the on and above diagonal terms except for right hand column 6
            for (m = 0; m < 6; m++) {
                for (n = m; n < 6; n++) {
                    MagCal->matA[m][n] += MagCal->vecA[m] * MagCal->vecA[n];
                }
            }

            // increment the measurement counter for the next iteration
            iCount++;
        }
    }

    // finally set the last element matA[6][6] to the number of measurements
    MagCal->matA[6][6] = (float) iCount;

    // store the number of measurements accumulated
    MagCal->MagBufferCount = iCount;

    // copy the above diagonal elements of matA to below the diagonal
    for (m = 1; m < 7; m++) {
        for (n = 0; n < m; n++) {
            MagCal->matA[m][n] = MagCal->matA[n][m];
        }
    }

    // set tmpA7x1 to the unsorted eigenvalues and matB to the unsorted eigenvectors of matA
    eigencompute(MagCal->matA, MagCal->vecA, MagCal->matB, 7);

    // find the smallest eigenvalue
    j = 0;
    for (i = 1; i < 7; i++) {
        if (MagCal->vecA[i] < MagCal->vecA[j]) {
            j = i;
        }
    }

    // set ellipsoid matrix A to the solution vector with smallest eigenvalue,
    // compute its determinant and the hard iron offset (scaled and offset)
    f3x3matrixAeqScalar(MagCal->A, 0.0F);
    det = 1.0F;
    for (k = X; k <= Z; k++) {
        MagCal->A[k][k] = MagCal->matB[k][j];
        det *= MagCal->A[k][k];
        MagCal->trV[k] = -0.5F * MagCal->matB[k + 3][j] / MagCal->A[k][k];
    }

    // negate A if it has negative determinant
    if (det < 0.0F) {
        f3x3matrixAeqMinusA(MagCal->A);
        MagCal->matB[6][j] = -MagCal->matB[6][j];
        det = -det;
    }

    // set ftmp to the square of the trial geomagnetic field strength B
    // (counts times FMATRIXSCALING)
    ftmp = -MagCal->matB[6][j];
    for (k = X; k <= Z; k++) {
        ftmp += MagCal->A[k][k] * MagCal->trV[k] * MagCal->trV[k];
    }

    // calculate the trial normalized fit error as a percentage
    MagCal->trFitErrorpc = 50.0F *
        sqrtf(fabs(MagCal->vecA[j]) / (float) MagCal->MagBufferCount) / fabs(ftmp);

    // normalize the ellipsoid matrix A to unit determinant
    f3x3matrixAeqAxScalar(MagCal->A, powf(det, -(ONETHIRD)));

    // convert the geomagnetic field strength B into uT for normalized
    // soft iron matrix A and normalize
    MagCal->trB = sqrtf(fabs(ftmp)) * DEFAULTB * powf(det, -(ONESIXTH));

    // compute trial invW from the square root of A also with normalized
    // determinant and hard iron offset in uT
    f3x3matrixAeqI(MagCal->trinvW);
    for (k = X; k <= Z; k++) {
        MagCal->trinvW[k][k] = sqrtf(fabs(MagCal->A[k][k]));
        MagCal->trV[k] = MagCal->trV[k] * DEFAULTB + (float)iOffset[k] * FXOS8700_UTPERCOUNT;
    }
}





// 10 element calibration using direct eigen-decomposition
static void fUpdateCalibration10EIG(MagCalibration_t *MagCal)
{
    float det;					// matrix determinant
    float fscaling;				// set to FUTPERCOUNT * FMATRIXSCALING
    float ftmp;					// scratch variable
    int16_t iOffset[3];			// offset to remove large DC hard iron bias in matrix
    int16_t iCount;				// number of measurements counted
    int i, j, k, m, n;			// loop counters

    // compute fscaling to reduce multiplications later
    fscaling = FXOS8700_UTPERCOUNT / DEFAULTB;

    // the offsets are guaranteed to be set from the first element but to avoid compiler error
    iOffset[X] = iOffset[Y] = iOffset[Z] = 0;

    // zero the on and above diagonal elements of the 10x10 symmetric measurement matrix matA
    for (m = 0; m < 10; m++) {
        for (n = m; n < 10; n++) {
            MagCal->matA[m][n] = 0.0F;
        }
    }

    // sum between MINEQUATIONS to MAXEQUATIONS entries into the 10x10 product matrix matA
    iCount = 0;
    for (j = 0; j < MAGBUFFSIZE; j++) {
        if (MagCal->valid[j]) {
            // use first valid magnetic buffer entry as estimate for offset
            // to help solution (bit counts)
            if (iCount == 0) {
                for (k = X; k <= Z; k++) {
                    iOffset[k] = MagCal->BpFast[k][j];
                }
            }

            // apply the fixed offset and scaling and enter into vecA[6-8]
            for (k = X; k <= Z; k++) {
                MagCal->vecA[k + 6] = (float)((int32_t)MagCal->BpFast[k][j]
                    - (int32_t)iOffset[k]) * fscaling;
            }

            // compute measurement vector elements vecA[0-5] from vecA[6-8]
            MagCal->vecA[0] = MagCal->vecA[6] * MagCal->vecA[6];
            MagCal->vecA[1] = 2.0F * MagCal->vecA[6] * MagCal->vecA[7];
            MagCal->vecA[2] = 2.0F * MagCal->vecA[6] * MagCal->vecA[8];
            MagCal->vecA[3] = MagCal->vecA[7] * MagCal->vecA[7];
            MagCal->vecA[4] = 2.0F * MagCal->vecA[7] * MagCal->vecA[8];
            MagCal->vecA[5] = MagCal->vecA[8] * MagCal->vecA[8];

            // accumulate the on-and above-diagonal terms of matA=Sigma{vecA^T * vecA}
            // with the exception of matA[9][9] which equals the number of measurements
            // update the right hand column [9] of matA[0-8][9] ignoring matA[9][9]
            for (m = 0; m < 9; m++) {
                MagCal->matA[m][9] += MagCal->vecA[m];
            }
            // update the on and above diagonal terms of matA ignoring right hand column 9
            for (m = 0; m < 9; m++) {
                for (n = m; n < 9; n++) {
                    MagCal->matA[m][n] += MagCal->vecA[m] * MagCal->vecA[n];
                }
            }

            // increment the measurement counter for the next iteration
            iCount++;
        }
    }

    // set the last element matA[9][9] to the number of measurements
    MagCal->matA[9][9] = (float) iCount;

    // store the number of measurements accumulated
    MagCal->MagBufferCount = iCount;

    // copy the above diagonal elements of symmetric product matrix matA to below the diagonal
    for (m = 1; m < 10; m++) {
        for (n = 0; n < m; n++) {
            MagCal->matA[m][n] = MagCal->matA[n][m];
        }
    }

    // set MagCal->vecA to the unsorted eigenvalues and matB to the unsorted
    // normalized eigenvectors of matA
    eigencompute(MagCal->matA, MagCal->vecA, MagCal->matB, 10);

    // set ellipsoid matrix A from elements of the solution vector column j with
    // smallest eigenvalue
    j = 0;
    for (i = 1; i < 10; i++) {
        if (MagCal->vecA[i] < MagCal->vecA[j]) {
            j = i;
        }
    }
    MagCal->A[0][0] = MagCal->matB[0][j];
    MagCal->A[0][1] = MagCal->A[1][0] = MagCal->matB[1][j];
    MagCal->A[0][2] = MagCal->A[2][0] = MagCal->matB[2][j];
    MagCal->A[1][1] = MagCal->matB[3][j];
    MagCal->A[1][2] = MagCal->A[2][1] = MagCal->matB[4][j];
    MagCal->A[2][2] = MagCal->matB[5][j];

    // negate entire solution if A has negative determinant
    det = f3x3matrixDetA(MagCal->A);
    if (det < 0.0F) {
        f3x3matrixAeqMinusA(MagCal->A);
        MagCal->matB[6][j] = -MagCal->matB[6][j];
        MagCal->matB[7][j] = -MagCal->matB[7][j];
        MagCal->matB[8][j] = -MagCal->matB[8][j];
        MagCal->matB[9][j] = -MagCal->matB[9][j];
        det = -det;
    }

    // compute the inverse of the ellipsoid matrix
    f3x3matrixAeqInvSymB(MagCal->invA, MagCal->A);

    // compute the trial hard iron vector in offset bit counts times FMATRIXSCALING
    for (k = X; k <= Z; k++) {
        MagCal->trV[k] = 0.0F;
        for (m = X; m <= Z; m++) {
            MagCal->trV[k] += MagCal->invA[k][m] * MagCal->matB[m + 6][j];
        }
        MagCal->trV[k] *= -0.5F;
    }

    // compute the trial geomagnetic field strength B in bit counts times FMATRIXSCALING
    MagCal->trB = sqrtf(fabs(MagCal->A[0][0] * MagCal->trV[X] * MagCal->trV[X] +
            2.0F * MagCal->A[0][1] * MagCal->trV[X] * MagCal->trV[Y] +
            2.0F * MagCal->A[0][2] * MagCal->trV[X] * MagCal->trV[Z] +
            MagCal->A[1][1] * MagCal->trV[Y] * MagCal->trV[Y] +
            2.0F * MagCal->A[1][2] * MagCal->trV[Y] * MagCal->trV[Z] +
            MagCal->A[2][2] * MagCal->trV[Z] * MagCal->trV[Z] - MagCal->matB[9][j]));

    // calculate the trial normalized fit error as a percentage
    MagCal->trFitErrorpc = 50.0F * sqrtf(
        fabs(MagCal->vecA[j]) / (float) MagCal->MagBufferCount) /
        (MagCal->trB * MagCal->trB);

    // correct for the measurement matrix offset and scaling and
    // get the computed hard iron offset in uT
    for (k = X; k <= Z; k++) {
        MagCal->trV[k] = MagCal->trV[k] * DEFAULTB + (float)iOffset[k] * FXOS8700_UTPERCOUNT;
    }

    // convert the trial geomagnetic field strength B into uT for
    // un-normalized soft iron matrix A
    MagCal->trB *= DEFAULTB;

    // normalize the ellipsoid matrix A to unit determinant and
    // correct B by root of this multiplicative factor
    f3x3matrixAeqAxScalar(MagCal->A, powf(det, -(ONETHIRD)));
    MagCal->trB *= powf(det, -(ONESIXTH));

    // compute trial invW from the square root of fA (both with normalized determinant)
    // set vecA to the unsorted eigenvalues and matB to the unsorted eigenvectors of matA
    // where matA holds the 3x3 matrix fA in its top left elements
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            MagCal->matA[i][j] = MagCal->A[i][j];
        }
    }
    eigencompute(MagCal->matA, MagCal->vecA, MagCal->matB, 3);

    // set MagCal->matB to be eigenvectors . diag(sqrt(sqrt(eigenvalues))) =
    //   matB . diag(sqrt(sqrt(vecA))
    for (j = 0; j < 3; j++) { // loop over columns j
        ftmp = sqrtf(sqrtf(fabs(MagCal->vecA[j])));
        for (i = 0; i < 3; i++) { // loop over rows i
            MagCal->matB[i][j] *= ftmp;
        }
    }

    // set trinvW to eigenvectors * diag(sqrt(eigenvalues)) * eigenvectors^T =
    //   matB * matB^T = sqrt(fA) (guaranteed symmetric)
    // loop over rows
    for (i = 0; i < 3; i++) {
        // loop over on and above diagonal columns
        for (j = i; j < 3; j++) {
            MagCal->trinvW[i][j] = 0.0F;
            // accumulate the matrix product
            for (k = 0; k < 3; k++) {
                MagCal->trinvW[i][j] += MagCal->matB[i][k] * MagCal->matB[j][k];
            }
            // copy to below diagonal element
            MagCal->trinvW[j][i] = MagCal->trinvW[i][j];
        }
    }
}




