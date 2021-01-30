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
// This file contains matrix manipulation functions.
//
#include "imuread.h"

// compile time constants that are private to this file
#define CORRUPTMATRIX 0.001F			// column vector modulus limit for rotation matrix

// vector components
#define X 0
#define Y 1
#define Z 2

// function sets the 3x3 matrix A to the identity matrix
void f3x3matrixAeqI(float A[][3])
{
    float *pAij;	// pointer to A[i][j]
    int8_t i, j;	// loop counters

    for (i = 0; i < 3; i++) {
        // set pAij to &A[i][j=0]
        pAij = A[i];
        for (j = 0; j < 3; j++) {
            *(pAij++) = 0.0F;
        }
        A[i][i] = 1.0F;
    }
}

// function sets the matrix A to the identity matrix
void fmatrixAeqI(float *A[], int16_t rc)
{
    // rc = rows and columns in A

    float *pAij;	// pointer to A[i][j]
    int8_t i, j;	// loop counters

    for (i = 0; i < rc; i++) {
        // set pAij to &A[i][j=0]
        pAij = A[i];
        for (j = 0; j < rc; j++) {
            *(pAij++) = 0.0F;
        }
        A[i][i] = 1.0F;
    }
}

// function sets every entry in the 3x3 matrix A to a constant scalar
void f3x3matrixAeqScalar(float A[][3], float Scalar)
{
    float *pAij;	// pointer to A[i][j]
    int8_t i, j;	// counters

    for (i = 0; i < 3; i++) {
        // set pAij to &A[i][j=0]
        pAij = A[i];
        for (j = 0; j < 3; j++) {
            *(pAij++) = Scalar;
        }
    }
}

// function multiplies all elements of 3x3 matrix A by the specified scalar
void f3x3matrixAeqAxScalar(float A[][3], float Scalar)
{
    float *pAij;	// pointer to A[i][j]
    int8_t i, j;	// loop counters

    for (i = 0; i < 3; i++) {
        // set pAij to &A[i][j=0]
        pAij = A[i];
        for (j = 0; j < 3; j++) {
            *(pAij++) *= Scalar;
        }
    }
}

// function negates all elements of 3x3 matrix A
void f3x3matrixAeqMinusA(float A[][3])
{
    float *pAij;	// pointer to A[i][j]
    int8_t i, j;	// loop counters

    for (i = 0; i < 3; i++) {
        // set pAij to &A[i][j=0]
        pAij = A[i];
        for (j = 0; j < 3; j++) {
            *pAij = -*pAij;
            pAij++;
        }
    }
}

// function directly calculates the symmetric inverse of a symmetric 3x3 matrix
// only the on and above diagonal terms in B are used and need to be specified
void f3x3matrixAeqInvSymB(float A[][3], float B[][3])
{
    float fB11B22mB12B12;	// B[1][1] * B[2][2] - B[1][2] * B[1][2]
    float fB12B02mB01B22;	// B[1][2] * B[0][2] - B[0][1] * B[2][2]
    float fB01B12mB11B02;	// B[0][1] * B[1][2] - B[1][1] * B[0][2]
    float ftmp;				// determinant and then reciprocal

    // calculate useful products
    fB11B22mB12B12 = B[1][1] * B[2][2] - B[1][2] * B[1][2];
    fB12B02mB01B22 = B[1][2] * B[0][2] - B[0][1] * B[2][2];
    fB01B12mB11B02 = B[0][1] * B[1][2] - B[1][1] * B[0][2];

    // set ftmp to the determinant of the input matrix B
    ftmp = B[0][0] * fB11B22mB12B12 + B[0][1] * fB12B02mB01B22 + B[0][2] * fB01B12mB11B02;

    // set A to the inverse of B for any determinant except zero
    if (ftmp != 0.0F) {
        ftmp = 1.0F / ftmp;
        A[0][0] = fB11B22mB12B12 * ftmp;
        A[1][0] = A[0][1] = fB12B02mB01B22 * ftmp;
        A[2][0] = A[0][2] = fB01B12mB11B02 * ftmp;
        A[1][1] = (B[0][0] * B[2][2] - B[0][2] * B[0][2]) * ftmp;
        A[2][1] = A[1][2] = (B[0][2] * B[0][1] - B[0][0] * B[1][2]) * ftmp;
        A[2][2] = (B[0][0] * B[1][1] - B[0][1] * B[0][1]) * ftmp;
    } else {
        // provide the identity matrix if the determinant is zero
        f3x3matrixAeqI(A);
    }
}

// function calculates the determinant of a 3x3 matrix
float f3x3matrixDetA(float A[][3])
{
    return (A[X][X] * (A[Y][Y] * A[Z][Z] - A[Y][Z] * A[Z][Y]) +
            A[X][Y] * (A[Y][Z] * A[Z][X] - A[Y][X] * A[Z][Z]) +
            A[X][Z] * (A[Y][X] * A[Z][Y] - A[Y][Y] * A[Z][X]));
}

// function computes all eigenvalues and eigenvectors of a real symmetric matrix A[0..n-1][0..n-1]
// stored in the top left of a 10x10 array A[10][10]
// A[][] is changed on output.
// eigval[0..n-1] returns the eigenvalues of A[][].
// eigvec[0..n-1][0..n-1] returns the normalized eigenvectors of A[][]
// the eigenvectors are not sorted by value
void eigencompute(float A[][10], float eigval[], float eigvec[][10], int8_t n)
{
    // maximum number of iterations to achieve convergence: in practice 6 is typical
#define NITERATIONS 15

    // various trig functions of the jacobi rotation angle phi
    float cot2phi, tanhalfphi, tanphi, sinphi, cosphi;
    // scratch variable to prevent over-writing during rotations
    float ftmp;
    // residue from remaining non-zero above diagonal terms
    float residue;
    // matrix row and column indices
    int8_t ir, ic;
    // general loop counter
    int8_t j;
    // timeout ctr for number of passes of the algorithm
    int8_t ctr;

    // initialize eigenvectors matrix and eigenvalues array
    for (ir = 0; ir < n; ir++) {
        // loop over all columns
        for (ic = 0; ic < n; ic++) {
            // set on diagonal and off-diagonal elements to zero
            eigvec[ir][ic] = 0.0F;
        }

        // correct the diagonal elements to 1.0
        eigvec[ir][ir] = 1.0F;

        // initialize the array of eigenvalues to the diagonal elements of m
        eigval[ir] = A[ir][ir];
    }

    // initialize the counter and loop until converged or NITERATIONS reached
    ctr = 0;
    do {
        // compute the absolute value of the above diagonal elements as exit criterion
        residue = 0.0F;
        // loop over rows excluding last row
        for (ir = 0; ir < n - 1; ir++) {
            // loop over above diagonal columns
            for (ic = ir + 1; ic < n; ic++) {
                // accumulate the residual off diagonal terms which are being driven to zero
                residue += fabs(A[ir][ic]);
            }
        }

        // check if we still have work to do
        if (residue > 0.0F) {
            // loop over all rows with the exception of the last row (since only rotating above diagonal elements)
            for (ir = 0; ir < n - 1; ir++) {
                // loop over columns ic (where ic is always greater than ir since above diagonal)
                for (ic = ir + 1; ic < n; ic++) {
                    // only continue with this element if the element is non-zero
                    if (fabs(A[ir][ic]) > 0.0F) {
                        // calculate cot(2*phi) where phi is the Jacobi rotation angle
                        cot2phi = 0.5F * (eigval[ic] - eigval[ir]) / (A[ir][ic]);

                        // calculate tan(phi) correcting sign to ensure the smaller solution is used
                        tanphi = 1.0F / (fabs(cot2phi) + sqrtf(1.0F + cot2phi * cot2phi));
                        if (cot2phi < 0.0F) {
                            tanphi = -tanphi;
                        }

                        // calculate the sine and cosine of the Jacobi rotation angle phi
                        cosphi = 1.0F / sqrtf(1.0F + tanphi * tanphi);
                        sinphi = tanphi * cosphi;

                        // calculate tan(phi/2)
                        tanhalfphi = sinphi / (1.0F + cosphi);

                        // set tmp = tan(phi) times current matrix element used in update of leading diagonal elements
                        ftmp = tanphi * A[ir][ic];

                        // apply the jacobi rotation to diagonal elements [ir][ir] and [ic][ic] stored in the eigenvalue array
                        // eigval[ir] = eigval[ir] - tan(phi) *  A[ir][ic]
                        eigval[ir] -= ftmp;
                        // eigval[ic] = eigval[ic] + tan(phi) * A[ir][ic]
                        eigval[ic] += ftmp;

                        // by definition, applying the jacobi rotation on element ir, ic results in 0.0
                        A[ir][ic] = 0.0F;

                        // apply the jacobi rotation to all elements of the eigenvector matrix
                        for (j = 0; j < n; j++) {
                            // store eigvec[j][ir]
                            ftmp = eigvec[j][ir];
                            // eigvec[j][ir] = eigvec[j][ir] - sin(phi) * (eigvec[j][ic] + tan(phi/2) * eigvec[j][ir])
                            eigvec[j][ir] = ftmp - sinphi * (eigvec[j][ic] + tanhalfphi * ftmp);
                            // eigvec[j][ic] = eigvec[j][ic] + sin(phi) * (eigvec[j][ir] - tan(phi/2) * eigvec[j][ic])
                            eigvec[j][ic] = eigvec[j][ic] + sinphi * (ftmp - tanhalfphi * eigvec[j][ic]);
                        }

                        // apply the jacobi rotation only to those elements of matrix m that can change
                        for (j = 0; j <= ir - 1; j++) {
                            // store A[j][ir]
                            ftmp = A[j][ir];
                            // A[j][ir] = A[j][ir] - sin(phi) * (A[j][ic] + tan(phi/2) * A[j][ir])
                            A[j][ir] = ftmp - sinphi * (A[j][ic] + tanhalfphi * ftmp);
                            // A[j][ic] = A[j][ic] + sin(phi) * (A[j][ir] - tan(phi/2) * A[j][ic])
                            A[j][ic] = A[j][ic] + sinphi * (ftmp - tanhalfphi * A[j][ic]);
                        }
                        for (j = ir + 1; j <= ic - 1; j++) {
                            // store A[ir][j]
                            ftmp = A[ir][j];
                            // A[ir][j] = A[ir][j] - sin(phi) * (A[j][ic] + tan(phi/2) * A[ir][j])
                            A[ir][j] = ftmp - sinphi * (A[j][ic] + tanhalfphi * ftmp);
                            // A[j][ic] = A[j][ic] + sin(phi) * (A[ir][j] - tan(phi/2) * A[j][ic])
                            A[j][ic] = A[j][ic] + sinphi * (ftmp - tanhalfphi * A[j][ic]);
                        }
                        for (j = ic + 1; j < n; j++) {
                            // store A[ir][j]
                            ftmp = A[ir][j];
                            // A[ir][j] = A[ir][j] - sin(phi) * (A[ic][j] + tan(phi/2) * A[ir][j])
                            A[ir][j] = ftmp - sinphi * (A[ic][j] + tanhalfphi * ftmp);
                            // A[ic][j] = A[ic][j] + sin(phi) * (A[ir][j] - tan(phi/2) * A[ic][j])
                            A[ic][j] = A[ic][j] + sinphi * (ftmp - tanhalfphi * A[ic][j]);
                        }
                    }   // end of test for matrix element already zero
                }   // end of loop over columns
            }   // end of loop over rows
        }  // end of test for non-zero residue
    } while ((residue > 0.0F) && (ctr++ < NITERATIONS)); // end of main loop
}

// function uses Gauss-Jordan elimination to compute the inverse of matrix A in situ
// on exit, A is replaced with its inverse
void fmatrixAeqInvA(float *A[], int8_t iColInd[], int8_t iRowInd[], int8_t iPivot[], int8_t isize)
{
    float largest;					// largest element used for pivoting
    float scaling;					// scaling factor in pivoting
    float recippiv;					// reciprocal of pivot element
    float ftmp;						// temporary variable used in swaps
    int8_t i, j, k, l, m;			// index counters
    int8_t iPivotRow, iPivotCol;	// row and column of pivot element

    // to avoid compiler warnings
    iPivotRow = iPivotCol = 0;

    // initialize the pivot array to 0
    for (j = 0; j < isize; j++) {
        iPivot[j] = 0;
    }

    // main loop i over the dimensions of the square matrix A
    for (i = 0; i < isize; i++) {
        // zero the largest element found for pivoting
        largest = 0.0F;
        // loop over candidate rows j
        for (j = 0; j < isize; j++) {
            // check if row j has been previously pivoted
            if (iPivot[j] != 1) {
                // loop over candidate columns k
                for (k = 0; k < isize; k++) {
                    // check if column k has previously been pivoted
                    if (iPivot[k] == 0) {
                        // check if the pivot element is the largest found so far
                        if (fabs(A[j][k]) >= largest) {
                            // and store this location as the current best candidate for pivoting
                            iPivotRow = j;
                            iPivotCol = k;
                            largest = (float) fabs(A[iPivotRow][iPivotCol]);
                        }
                    } else if (iPivot[k] > 1) {
                        // zero determinant situation: exit with identity matrix
                        fmatrixAeqI(A, isize);
                        return;
                    }
                }
            }
        }
        // increment the entry in iPivot to denote it has been selected for pivoting
        iPivot[iPivotCol]++;

        // check the pivot rows iPivotRow and iPivotCol are not the same before swapping
        if (iPivotRow != iPivotCol) {
            // loop over columns l
            for (l = 0; l < isize; l++) {
                // and swap all elements of rows iPivotRow and iPivotCol
                ftmp = A[iPivotRow][l];
                A[iPivotRow][l] = A[iPivotCol][l];
                A[iPivotCol][l] = ftmp;
            }
        }

        // record that on the i-th iteration rows iPivotRow and iPivotCol were swapped
        iRowInd[i] = iPivotRow;
        iColInd[i] = iPivotCol;

        // check for zero on-diagonal element (singular matrix) and return with identity matrix if detected
        if (A[iPivotCol][iPivotCol] == 0.0F) {
            // zero determinant situation: exit with identity matrix
            fmatrixAeqI(A, isize);
            return;
        }

        // calculate the reciprocal of the pivot element knowing it's non-zero
        recippiv = 1.0F / A[iPivotCol][iPivotCol];
        // by definition, the diagonal element normalizes to 1
        A[iPivotCol][iPivotCol] = 1.0F;
        // multiply all of row iPivotCol by the reciprocal of the pivot element including the diagonal element
        // the diagonal element A[iPivotCol][iPivotCol] now has value equal to the reciprocal of its previous value
        for (l = 0; l < isize; l++) {
            A[iPivotCol][l] *= recippiv;
        }
        // loop over all rows m of A
        for (m = 0; m < isize; m++) {
            if (m != iPivotCol) {
                // scaling factor for this row m is in column iPivotCol
                scaling = A[m][iPivotCol];
                // zero this element
                A[m][iPivotCol] = 0.0F;
                // loop over all columns l of A and perform elimination
                for (l = 0; l < isize; l++) {
                    A[m][l] -= A[iPivotCol][l] * scaling;
                }
            }
        }
    } // end of loop i over the matrix dimensions

    // finally, loop in inverse order to apply the missing column swaps
    for (l = isize - 1; l >= 0; l--) {
        // set i and j to the two columns to be swapped
        i = iRowInd[l];
        j = iColInd[l];

        // check that the two columns i and j to be swapped are not the same
        if (i != j) {
            // loop over all rows k to swap columns i and j of A
            for (k = 0; k < isize; k++) {
                ftmp = A[k][i];
                A[k][i] = A[k][j];
                A[k][j] = ftmp;
            }
        }
    }
}

// function re-orthonormalizes a 3x3 rotation matrix
void fmatrixAeqRenormRotA(float A[][3])
{
    float ftmp;					// scratch variable

    // normalize the X column of the low pass filtered orientation matrix
    ftmp = sqrtf(A[X][X] * A[X][X] + A[Y][X] * A[Y][X] + A[Z][X] * A[Z][X]);
    if (ftmp > CORRUPTMATRIX) {
        // normalize the x column vector
        ftmp = 1.0F / ftmp;
        A[X][X] *= ftmp;
        A[Y][X] *= ftmp;
        A[Z][X] *= ftmp;
    } else {
        // set x column vector to {1, 0, 0}
        A[X][X] = 1.0F;
        A[Y][X] = A[Z][X] = 0.0F;
    }

    // force the y column vector to be orthogonal to x using y = y-(x.y)x
    ftmp = A[X][X] * A[X][Y] + A[Y][X] * A[Y][Y] + A[Z][X] * A[Z][Y];
    A[X][Y] -= ftmp * A[X][X];
    A[Y][Y] -= ftmp * A[Y][X];
    A[Z][Y] -= ftmp * A[Z][X];

    // normalize the y column vector
    ftmp = sqrtf(A[X][Y] * A[X][Y] + A[Y][Y] * A[Y][Y] + A[Z][Y] * A[Z][Y]);
    if (ftmp > CORRUPTMATRIX) {
        // normalize the y column vector
        ftmp = 1.0F / ftmp;
        A[X][Y] *= ftmp;
        A[Y][Y] *= ftmp;
        A[Z][Y] *= ftmp;
    } else {
        // set y column vector to {0, 1, 0}
        A[Y][Y] = 1.0F;
        A[X][Y] = A[Z][Y] = 0.0F;
    }

    // finally set the z column vector to x vector cross y vector (automatically normalized)
    A[X][Z] = A[Y][X] * A[Z][Y] - A[Z][X] * A[Y][Y];
    A[Y][Z] = A[Z][X] * A[X][Y] - A[X][X] * A[Z][Y];
    A[Z][Z] = A[X][X] * A[Y][Y] - A[Y][X] * A[X][Y];
}
