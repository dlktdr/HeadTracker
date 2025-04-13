/*
 * Copyright (c) 2019 Kevin Townsend (KTOWN)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <errno.h>
 #include <stdio.h>
 #include <stdbool.h>
 #include <string.h>
 #include "zsl.h"
 #include "matrices.h"

 /*
  * WARNING: Work in progress!
  *
  * The code in this module is very 'naive' in the sense that no attempt
  * has been made at efficiency. It is written from the perspective
  * that code should be written to be 'reliable, elegant, efficient' in that
  * order.
  *
  * Clarity and reliability have been absolutely prioritized in this
  * early stage, with the key goal being good unit test coverage before
  * moving on to any form of general-purpose or architecture-specific
  * optimisation.
  */

 // TODO: Introduce local macros for bounds/shape checks to avoid duplication!

 int
 zsl_mtx_entry_fn_empty(struct zsl_mtx *m, size_t i, size_t j)
 {
   return zsl_mtx_set(m, i, j, 0);
 }

 int
 zsl_mtx_entry_fn_identity(struct zsl_mtx *m, size_t i, size_t j)
 {
   return zsl_mtx_set(m, i, j, i == j ? 1.0 : 0);
 }

 int
 zsl_mtx_entry_fn_random(struct zsl_mtx *m, size_t i, size_t j)
 {
   /* TODO: Determine an appropriate random number generator. */
   return zsl_mtx_set(m, i, j, 0);
 }

 int
 zsl_mtx_init(struct zsl_mtx *m, zsl_mtx_init_entry_fn_t entry_fn)
 {
   int rc;

   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_cols; j++) {
       /* If entry_fn is NULL, assign 0.0 values. */
       if (entry_fn == NULL) {
         rc = zsl_mtx_entry_fn_empty(m, i, j);
       } else {
         rc = entry_fn(m, i, j);
       }
       /* Abort if entry_fn returned an error code. */
       if (rc) {
         return rc;
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_from_arr(struct zsl_mtx *m, zsl_real_t *a)
 {
   memcpy(m->data, a, (m->sz_rows * m->sz_cols) * sizeof(zsl_real_t));

   return 0;
 }

 int
 zsl_mtx_copy(struct zsl_mtx *mdest, struct zsl_mtx *msrc)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Ensure that msrc and mdest have the same shape. */
   if ((mdest->sz_rows != msrc->sz_rows) ||
       (mdest->sz_cols != msrc->sz_cols)) {
     return -EINVAL;
   }
 #endif

   /* Make a copy of matrix 'msrc'. */
   memcpy(mdest->data, msrc->data, sizeof(zsl_real_t) *
          msrc->sz_rows * msrc->sz_cols);

   return 0;
 }

 int
 zsl_mtx_get(struct zsl_mtx *m, size_t i, size_t j, zsl_real_t *x)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((i >= m->sz_rows) || (j >= m->sz_cols)) {
     return -EINVAL;
   }
 #endif

   *x = m->data[(i * m->sz_cols) + j];

   return 0;
 }

 int
 zsl_mtx_set(struct zsl_mtx *m, size_t i, size_t j, zsl_real_t x)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((i >= m->sz_rows) || (j >= m->sz_cols)) {
     return -EINVAL;
   }
 #endif

   m->data[(i * m->sz_cols) + j] = x;

   return 0;
 }

 int
 zsl_mtx_get_row(struct zsl_mtx *m, size_t i, zsl_real_t *v)
 {
   int rc;
   zsl_real_t x;

   for (size_t j = 0; j < m->sz_cols; j++) {
     rc = zsl_mtx_get(m, i, j, &x);
     if (rc) {
       return rc;
     }
     v[j] = x;
   }

   return 0;
 }

 int
 zsl_mtx_set_row(struct zsl_mtx *m, size_t i, zsl_real_t *v)
 {
   int rc;

   for (size_t j = 0; j < m->sz_cols; j++) {
     rc = zsl_mtx_set(m, i, j, v[j]);
     if (rc) {
       return rc;
     }
   }

   return 0;
 }

 int
 zsl_mtx_get_col(struct zsl_mtx *m, size_t j, zsl_real_t *v)
 {
   int rc;
   zsl_real_t x;

   for (size_t i = 0; i < m->sz_rows; i++) {
     rc = zsl_mtx_get(m, i, j, &x);
     if (rc) {
       return rc;
     }
     v[i] = x;
   }

   return 0;
 }

 int
 zsl_mtx_set_col(struct zsl_mtx *m, size_t j, zsl_real_t *v)
 {
   int rc;

   for (size_t i = 0; i < m->sz_rows; i++) {
     rc = zsl_mtx_set(m, i, j, v[i]);
     if (rc) {
       return rc;
     }
   }

   return 0;
 }

 int
 zsl_mtx_unary_op(struct zsl_mtx *m, zsl_mtx_unary_op_t op)
 {
   /* Execute the unary operation component by component. */
   for (size_t i = 0; i < m->sz_cols * m->sz_rows; i++) {
     switch (op) {
     case ZSL_MTX_UNARY_OP_INCREMENT:
       m->data[i] += 1.0;
       break;
     case ZSL_MTX_UNARY_OP_DECREMENT:
       m->data[i] -= 1.0;
       break;
     case ZSL_MTX_UNARY_OP_NEGATIVE:
       m->data[i] = -m->data[i];
       break;
     case ZSL_MTX_UNARY_OP_ROUND:
       m->data[i] = ZSL_ROUND(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_ABS:
       m->data[i] = ZSL_ABS(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_FLOOR:
       m->data[i] = ZSL_FLOOR(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_CEIL:
       m->data[i] = ZSL_CEIL(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_EXP:
       m->data[i] = ZSL_EXP(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_LOG:
       m->data[i] = ZSL_LOG(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_LOG10:
       m->data[i] = ZSL_LOG10(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_SQRT:
       m->data[i] = ZSL_SQRT(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_SIN:
       m->data[i] = ZSL_SIN(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_COS:
       m->data[i] = ZSL_COS(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_TAN:
       m->data[i] = ZSL_TAN(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_ASIN:
       m->data[i] = ZSL_ASIN(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_ACOS:
       m->data[i] = ZSL_ACOS(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_ATAN:
       m->data[i] = ZSL_ATAN(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_SINH:
       m->data[i] = ZSL_SINH(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_COSH:
       m->data[i] = ZSL_COSH(m->data[i]);
       break;
     case ZSL_MTX_UNARY_OP_TANH:
       m->data[i] = ZSL_TANH(m->data[i]);
       break;
     default:
       /* Not yet implemented! */
       return -ENOSYS;
     }
   }

   return 0;
 }

 int
 zsl_mtx_unary_func(struct zsl_mtx *m, zsl_mtx_unary_fn_t fn)
 {
   int rc;

   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_cols; j++) {
       /* If fn is NULL, do nothing. */
       if (fn != NULL) {
         rc = fn(m, i, j);
         if (rc) {
           return rc;
         }
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_binary_op(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc,
       zsl_mtx_binary_op_t op)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((ma->sz_rows != mb->sz_rows) || (mb->sz_rows != mc->sz_rows) ||
       (ma->sz_cols != mb->sz_cols) || (mb->sz_cols != mc->sz_cols)) {
     return -EINVAL;
   }
 #endif

   /* Execute the binary operation component by component. */
   for (size_t i = 0; i < ma->sz_cols * ma->sz_rows; i++) {
     switch (op) {
     case ZSL_MTX_BINARY_OP_ADD:
       mc->data[i] = ma->data[i] + mb->data[i];
       break;
     case ZSL_MTX_BINARY_OP_SUB:
       mc->data[i] = ma->data[i] - mb->data[i];
       break;
     case ZSL_MTX_BINARY_OP_MULT:
       mc->data[i] = ma->data[i] * mb->data[i];
       break;
     case ZSL_MTX_BINARY_OP_DIV:
       if (mb->data[i] == 0.0) {
         mc->data[i] = 0.0;
       } else {
         mc->data[i] = ma->data[i] / mb->data[i];
       }
       break;
     case ZSL_MTX_BINARY_OP_MEAN:
       mc->data[i] = (ma->data[i] + mb->data[i]) / 2.0;
     case ZSL_MTX_BINARY_OP_EXPON:
       mc->data[i] = ZSL_POW(ma->data[i], mb->data[i]);
     case ZSL_MTX_BINARY_OP_MIN:
       mc->data[i] = ma->data[i] < mb->data[i] ?
               ma->data[i] : mb->data[i];
     case ZSL_MTX_BINARY_OP_MAX:
       mc->data[i] = ma->data[i] > mb->data[i] ?
               ma->data[i] : mb->data[i];
     case ZSL_MTX_BINARY_OP_EQUAL:
       mc->data[i] = ma->data[i] == mb->data[i] ? 1.0 : 0.0;
     case ZSL_MTX_BINARY_OP_NEQUAL:
       mc->data[i] = ma->data[i] != mb->data[i] ? 1.0 : 0.0;
     case ZSL_MTX_BINARY_OP_LESS:
       mc->data[i] = ma->data[i] < mb->data[i] ? 1.0 : 0.0;
     case ZSL_MTX_BINARY_OP_GREAT:
       mc->data[i] = ma->data[i] > mb->data[i] ? 1.0 : 0.0;
     case ZSL_MTX_BINARY_OP_LEQ:
       mc->data[i] = ma->data[i] <= mb->data[i] ? 1.0 : 0.0;
     case ZSL_MTX_BINARY_OP_GEQ:
       mc->data[i] = ma->data[i] >= mb->data[i] ? 1.0 : 0.0;
     default:
       /* Not yet implemented! */
       return -ENOSYS;
     }
   }

   return 0;
 }

 int
 zsl_mtx_binary_func(struct zsl_mtx *ma, struct zsl_mtx *mb,
         struct zsl_mtx *mc, zsl_mtx_binary_fn_t fn)
 {
   int rc;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((ma->sz_rows != mb->sz_rows) || (mb->sz_rows != mc->sz_rows) ||
       (ma->sz_cols != mb->sz_cols) || (mb->sz_cols != mc->sz_cols)) {
     return -EINVAL;
   }
 #endif

   for (size_t i = 0; i < ma->sz_rows; i++) {
     for (size_t j = 0; j < ma->sz_cols; j++) {
       /* If fn is NULL, do nothing. */
       if (fn != NULL) {
         rc = fn(ma, mb, mc, i, j);
         if (rc) {
           return rc;
         }
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_add(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc)
 {
   return zsl_mtx_binary_op(ma, mb, mc, ZSL_MTX_BINARY_OP_ADD);
 }

 int
 zsl_mtx_add_d(struct zsl_mtx *ma, struct zsl_mtx *mb)
 {
   return zsl_mtx_binary_op(ma, mb, ma, ZSL_MTX_BINARY_OP_ADD);
 }

 int
 zsl_mtx_sum_rows_d(struct zsl_mtx *m, size_t i, size_t j)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((i >= m->sz_rows) || (j >= m->sz_rows)) {
     return -EINVAL;
   }
 #endif

   /* Add row j to row i, element by element. */
   for (size_t x = 0; x < m->sz_cols; x++) {
     m->data[(i * m->sz_cols) + x] += m->data[(j * m->sz_cols) + x];
   }

   return 0;
 }

 int zsl_mtx_sum_rows_scaled_d(struct zsl_mtx *m,
             size_t i, size_t j, zsl_real_t s)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if ((i >= m->sz_rows) || (j >= m->sz_cols)) {
     return -EINVAL;
   }
 #endif

   /* Set the values in row 'i' to 'i[n] += j[n] * s' . */
   for (size_t x = 0; x < m->sz_cols; x++) {
     m->data[(i * m->sz_cols) + x] +=
       (m->data[(j * m->sz_cols) + x] * s);
   }

   return 0;
 }

 int
 zsl_mtx_sub(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc)
 {
   return zsl_mtx_binary_op(ma, mb, mc, ZSL_MTX_BINARY_OP_SUB);
 }

 int
 zsl_mtx_sub_d(struct zsl_mtx *ma, struct zsl_mtx *mb)
 {
   return zsl_mtx_binary_op(ma, mb, ma, ZSL_MTX_BINARY_OP_SUB);
 }

 int
 zsl_mtx_mult(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Ensure that ma has the same number as columns as mb has rows. */
   if (ma->sz_cols != mb->sz_rows) {
     return -EINVAL;
   }

   /* Ensure that mc has ma rows and mb cols */
   if ((mc->sz_rows != ma->sz_rows) || (mc->sz_cols != mb->sz_cols)) {
     return -EINVAL;
   }
 #endif

   ZSL_MATRIX_DEF(ma_copy, ma->sz_rows, ma->sz_cols);
   ZSL_MATRIX_DEF(mb_copy, mb->sz_rows, mb->sz_cols);
   zsl_mtx_copy(&ma_copy, ma);
   zsl_mtx_copy(&mb_copy, mb);

   for (size_t i = 0; i < ma_copy.sz_rows; i++) {
     for (size_t j = 0; j < mb_copy.sz_cols; j++) {
       mc->data[j + i * mb_copy.sz_cols] = 0;
       for (size_t k = 0; k < ma_copy.sz_cols; k++) {
         mc->data[j + i * mb_copy.sz_cols] +=
           ma_copy.data[k + i * ma_copy.sz_cols] *
           mb_copy.data[j + k * mb_copy.sz_cols];
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_mult_d(struct zsl_mtx *ma, struct zsl_mtx *mb)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Ensure that ma has the same number as columns as mb has rows. */
   if (ma->sz_cols != mb->sz_rows) {
     return -EINVAL;
   }

   /* Ensure that mb is a square matrix. */
   if (mb->sz_rows != mb->sz_cols) {
     return -EINVAL;
   }
 #endif

   zsl_mtx_mult(ma, mb, ma);

   return 0;
 }

 int
 zsl_mtx_scalar_mult_d(struct zsl_mtx *m, zsl_real_t s)
 {
   for (size_t i = 0; i < m->sz_rows * m->sz_cols; i++) {
     m->data[i] *= s;
   }

   return 0;
 }

 int
 zsl_mtx_scalar_mult_row_d(struct zsl_mtx *m, size_t i, zsl_real_t s)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   if (i >= m->sz_rows) {
     return -EINVAL;
   }
 #endif

   for (size_t k = 0; k < m->sz_cols; k++) {
     m->data[(i * m->sz_cols) + k] *= s;
   }

   return 0;
 }

 int
 zsl_mtx_trans(struct zsl_mtx *ma, struct zsl_mtx *mb)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Ensure that ma and mb have the same shape. */
   if ((ma->sz_rows != mb->sz_cols) || (ma->sz_cols != mb->sz_rows)) {
     return -EINVAL;
   }
 #endif

   zsl_real_t d[ma->sz_cols];

   for (size_t i = 0; i < ma->sz_rows; i++) {
     zsl_mtx_get_row(ma, i, d);
     zsl_mtx_set_col(mb, i, d);
   }

   return 0;
 }

 int
 zsl_mtx_adjoint_3x3(struct zsl_mtx *m, struct zsl_mtx *ma)
 {
   /* Make sure this is a square matrix. */
   if ((m->sz_rows != m->sz_cols) || (ma->sz_rows != ma->sz_cols)) {
     return -EINVAL;
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure this is a 3x3 matrix. */
   if ((m->sz_rows != 3) || (ma->sz_rows != 3)) {
     return -EINVAL;
   }
 #endif

   /*
    * 3x3 matrix element to array table:
    *
    * 1,1 = 0  1,2 = 1  1,3 = 2
    * 2,1 = 3  2,2 = 4  2,3 = 5
    * 3,1 = 6  3,2 = 7  3,3 = 8
    */

   ma->data[0] = m->data[4] * m->data[8] - m->data[7] * m->data[5];
   ma->data[1] = m->data[7] * m->data[2] - m->data[1] * m->data[8];
   ma->data[2] = m->data[1] * m->data[5] - m->data[4] * m->data[2];

   ma->data[3] = m->data[6] * m->data[5] - m->data[3] * m->data[8];
   ma->data[4] = m->data[0] * m->data[8] - m->data[6] * m->data[2];
   ma->data[5] = m->data[3] * m->data[2] - m->data[0] * m->data[5];

   ma->data[6] = m->data[3] * m->data[7] - m->data[6] * m->data[4];
   ma->data[7] = m->data[6] * m->data[1] - m->data[0] * m->data[7];
   ma->data[8] = m->data[0] * m->data[4] - m->data[3] * m->data[1];

   return 0;
 }

 int
 zsl_mtx_adjoint(struct zsl_mtx *m, struct zsl_mtx *ma)
 {
   /* Shortcut for 3x3 matrices. */
   if (m->sz_rows == 3) {
     return zsl_mtx_adjoint_3x3(m, ma);
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure this is a square matrix. */
   if (m->sz_rows != m->sz_cols) {
     return -EINVAL;
   }
 #endif

   zsl_real_t sign;
   zsl_real_t d;
   ZSL_MATRIX_DEF(mr, (m->sz_cols - 1), (m->sz_cols - 1));

   for (size_t i = 0; i < m->sz_cols; i++) {
     for (size_t j = 0; j < m->sz_cols; j++) {
       sign = 1.0;
       if ((i + j) % 2 != 0) {
         sign = -1.0;
       }
       zsl_mtx_reduce(m, &mr, i, j);
       zsl_mtx_deter(&mr, &d);
       d *= sign;
       zsl_mtx_set(ma, i, j, d);
     }
   }

   return 0;
 }

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int zsl_mtx_vec_wedge(struct zsl_mtx *m, struct zsl_vec *v)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure the dimensions of 'm' and 'v' match. */
   if (v->sz != m->sz_cols || v->sz < 4 || m->sz_rows != (m->sz_cols - 1)) {
     return -EINVAL;
   }
 #endif

   zsl_real_t d;

   ZSL_MATRIX_DEF(A, m->sz_cols, m->sz_cols);
   ZSL_MATRIX_DEF(Ai, m->sz_cols, m->sz_cols);
   ZSL_VECTOR_DEF(Av, m->sz_cols);
   ZSL_MATRIX_DEF(b, m->sz_cols, 1);

   zsl_mtx_init(&A, NULL);
   A.data[(m->sz_cols * m->sz_cols - 1)] = 1.0;

   for (size_t i = 0; i < m->sz_rows; i++) {
     zsl_mtx_get_row(m, i, Av.data);
     zsl_mtx_set_row(&A, i, Av.data);
   }

   zsl_mtx_deter(&A, &d);
   zsl_mtx_inv(&A, &Ai);
   zsl_mtx_init(&b, NULL);
   b.data[(m->sz_cols - 1)] = d;

   zsl_mtx_mult(&Ai, &b, &b);

   zsl_vec_from_arr(v, b.data);

   return 0;
 }
 #endif

 int
 zsl_mtx_reduce(struct zsl_mtx *m, struct zsl_mtx *mr, size_t i, size_t j)
 {
   size_t u = 0;
   zsl_real_t x;
   zsl_real_t v[mr->sz_rows * mr->sz_rows];

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure mr is 1 less than m. */
   if (mr->sz_rows != m->sz_rows - 1) {
     return -EINVAL;
   }
   if (mr->sz_cols != m->sz_cols - 1) {
     return -EINVAL;
   }
   if ((i >= m->sz_rows) || (j >= m->sz_cols)) {
     return -EINVAL;
   }
 #endif

   for (size_t k = 0; k < m->sz_rows; k++) {
     for (size_t g = 0; g < m->sz_rows; g++) {
       if (k != i && g != j) {
         zsl_mtx_get(m, k, g, &x);
         v[u] = x;
         u++;
       }
     }
   }

   zsl_mtx_from_arr(mr, v);

   return 0;
 }

 int
 zsl_mtx_reduce_iter(struct zsl_mtx *m, struct zsl_mtx *mred,
         struct zsl_mtx *place1, struct zsl_mtx *place2)
 {
   /* TODO: Properly check if matrix is square. */
   if (m->sz_rows == place1->sz_rows) {
     zsl_mtx_copy(place1, m);
   }

   if (place1->sz_rows == mred->sz_rows) {
     zsl_mtx_copy(mred, place1);

     /* restore the original placeholder size */
     place1->sz_rows = m->sz_rows;
     place1->sz_cols = m->sz_cols;
     place2->sz_rows = m->sz_rows;
     place2->sz_cols = m->sz_cols;
     return 0;
   }

   /* trick the iterative method by generating the inner
    * call intermediate matrix, adjusting its size
    */
   place2->sz_rows = place1->sz_rows - 1;
   place2->sz_cols = place1->sz_cols - 1;
   zsl_mtx_reduce(place1, place2, 0, 0);

   /* Do the same with the second placeholder matrix */
   place1->sz_rows--;
   place1->sz_cols--;
   zsl_mtx_copy(place1, place2);

   return -EAGAIN;
 }

 int
 zsl_mtx_augm_diag(struct zsl_mtx *m, struct zsl_mtx *maug)
 {
   zsl_real_t x;
   /* TODO: Properly check if matrix is square, and diff > 0. */
   size_t diff = (maug->sz_rows) - (m->sz_rows);

   zsl_mtx_init(maug, zsl_mtx_entry_fn_identity);
   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_rows; j++) {
       zsl_mtx_get(m, i, j, &x);
       zsl_mtx_set(maug, i + diff, j + diff, x);
     }
   }

   return 0;
 }

 int
 zsl_mtx_deter_3x3(struct zsl_mtx *m, zsl_real_t *d)
 {
   /* Make sure this is a square matrix. */
   if (m->sz_rows != m->sz_cols) {
     return -EINVAL;
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure this is a 3x3 matrix. */
   if (m->sz_rows != 3) {
     return -EINVAL;
   }
 #endif

   /*
    * 3x3 matrix element to array table:
    *
    * 1,1 = 0  1,2 = 1  1,3 = 2
    * 2,1 = 3  2,2 = 4  2,3 = 5
    * 3,1 = 6  3,2 = 7  3,3 = 8
    */

   *d = m->data[0] * (m->data[4] * m->data[8] - m->data[7] * m->data[5]);
   *d -= m->data[3] * (m->data[1] * m->data[8] - m->data[7] * m->data[2]);
   *d += m->data[6] * (m->data[1] * m->data[5] - m->data[4] * m->data[2]);

   return 0;
 }

 int
 zsl_mtx_deter(struct zsl_mtx *m, zsl_real_t *d)
 {
   /* Shortcut for 1x1 matrices. */
   if (m->sz_rows == 1) {
     *d = m->data[0];
     return 0;
   }

   /* Shortcut for 2x2 matrices. */
   if (m->sz_rows == 2) {
     *d = m->data[0] * m->data[3] - m->data[2] * m->data[1];
     return 0;
   }

   /* Shortcut for 3x3 matrices. */
   if (m->sz_rows == 3) {
     return zsl_mtx_deter_3x3(m, d);
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure this is a square matrix. */
   if (m->sz_rows != m->sz_cols) {
     return -EINVAL;
   }
 #endif

   /* Full calculation required for non 3x3 matrices. */
   int rc;
   zsl_real_t dtmp;
   zsl_real_t cur;
   zsl_real_t sign;
   ZSL_MATRIX_DEF(mr, (m->sz_rows - 1), (m->sz_rows - 1));

   /* Clear determinant output before starting. */
   *d = 0.0;

   /*
    * Iterate across row 0, removing columns one by one.
    * Note that these calls are recursive until we reach a 3x3 matrix,
    * which will be calculated using the shortcut at the top of this
    * function.
    */
   for (size_t g = 0; g < m->sz_cols; g++) {
     zsl_mtx_get(m, 0, g, &cur);     /* Get value at (0, g). */
     zsl_mtx_init(&mr, NULL);        /* Clear mr. */
     zsl_mtx_reduce(m, &mr, 0, g);   /* Remove row 0, column g. */
     rc = zsl_mtx_deter(&mr, &dtmp); /* Calc. determinant of mr. */
     sign = 1.0;
     if (rc) {
       return -EINVAL;
     }

     /* Uneven elements are negative. */
     if (g % 2 != 0) {
       sign = -1.0;
     }

     /* Add current determinant to final output value. */
     *d += dtmp * cur * sign;
   }

   return 0;
 }

 int
 zsl_mtx_gauss_elim(struct zsl_mtx *m, struct zsl_mtx *mg, struct zsl_mtx *mi,
        size_t i, size_t j)
 {
   int rc;
   zsl_real_t x, y;
   zsl_real_t epsilon = 1E-6;

   /* Make a copy of matrix m. */
   rc = zsl_mtx_copy(mg, m);
   if (rc) {
     return -EINVAL;
   }

   /* Get the value of the element at position (i, j). */
   rc = zsl_mtx_get(mg, i, j, &y);
   if (rc) {
     return rc;
   }

   /* If this is a zero value, don't do anything. */
   if ((y >= 0 && y < epsilon) || (y <= 0 && y > -epsilon)) {
     return 0;
   }

   /* Cycle through the matrix row by row. */
   for (size_t p = 0; p < mg->sz_rows; p++) {
     /* Skip row 'i'. */
     if (p == i) {
       p++;
     }
     if (p == mg->sz_rows) {
       break;
     }
     /* Get the value of (p, j), aborting if value is zero. */
     zsl_mtx_get(mg, p, j, &x);
     if ((x >= 1E-6) || (x <= -1E-6)) {
       rc = zsl_mtx_sum_rows_scaled_d(mg, p, i, -(x / y));

       if (rc) {
         return -EINVAL;
       }
       rc = zsl_mtx_sum_rows_scaled_d(mi, p, i, -(x / y));
       if (rc) {
         return -EINVAL;
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_gauss_elim_d(struct zsl_mtx *m, struct zsl_mtx *mi, size_t i, size_t j)
 {
   return zsl_mtx_gauss_elim(m, m, mi, i, j);
 }

 int
 zsl_mtx_gauss_reduc(struct zsl_mtx *m, struct zsl_mtx *mi,
         struct zsl_mtx *mg)
 {
   zsl_real_t v[m->sz_rows];
   zsl_real_t epsilon = 1E-6;
   zsl_real_t x;
   zsl_real_t y;

   /* Copy the input matrix into 'mg' so all the changes will be done to
    * 'mg' and the input matrix will not be destroyed. */
   zsl_mtx_copy(mg, m);

   for (size_t k = 0; k < m->sz_rows; k++) {

     /* Get every element in the diagonal. */
     zsl_mtx_get(mg, k, k, &x);

     /* If the diagonal element is zero, find another value in the
      * same column that isn't zero and add the row containing
      * the non-zero element to the diagonal element's row. */
     if ((x >= 0 && x < epsilon) || (x <= 0 && x > -epsilon)) {
       zsl_mtx_get_col(mg, k, v);
       for (size_t q = 0; q < m->sz_rows; q++) {
         zsl_mtx_get(mg, q, q, &y);
         if ((v[q] >= epsilon) || (v[q] <= -epsilon)) {

           /* If the non-zero element found is
            * above the diagonal, only add its row
            * if the diagonal element in this row
            * is zero, to avoid undoing previous
            * steps. */
           if (q < k && ((y >= epsilon)
                   || (y <= -epsilon))) {
           } else {
             zsl_mtx_sum_rows_d(mg, k, q);
             zsl_mtx_sum_rows_d(mi, k, q);
             break;
           }
         }
       }
     }

     /* Perform the gaussian elimination in the column of the
      * diagonal element to get rid of all the values in the column
      * except for the diagonal one. */
     zsl_mtx_gauss_elim_d(mg, mi, k, k);

     /* Divide the diagonal element's row by the diagonal element. */
     zsl_mtx_norm_elem_d(mg, mi, k, k);
   }

   return 0;
 }

 int
 zsl_mtx_gram_schmidt(struct zsl_mtx *m, struct zsl_mtx *mort)
 {
   ZSL_VECTOR_DEF(v, m->sz_rows);
   ZSL_VECTOR_DEF(w, m->sz_rows);
   ZSL_VECTOR_DEF(q, m->sz_rows);

   for (size_t t = 0; t < m->sz_cols; t++) {
     zsl_vec_init(&q);
     zsl_mtx_get_col(m, t, v.data);
     for (size_t g = 0; g < t; g++) {
       zsl_mtx_get_col(mort, g, w.data);

       /* Calculate the projection of every column vector
        * before 'g' on the 't'th column. */
       zsl_vec_project(&w, &v, &w);
       zsl_vec_add(&q, &w, &q);
     }

     /* Substract the sum of the projections on the 't'th column from
      * the 't'th column and set this vector as the 't'th column of
      * the output matrix. */
     zsl_vec_sub(&v, &q, &v);
     zsl_mtx_set_col(mort, t, v.data);
   }

   return 0;
 }

 int
 zsl_mtx_cols_norm(struct zsl_mtx *m, struct zsl_mtx *mnorm)
 {
   ZSL_VECTOR_DEF(v, m->sz_rows);

   for (size_t g = 0; g < m->sz_cols; g++) {
     zsl_mtx_get_col(m, g, v.data);
     zsl_vec_to_unit(&v);
     zsl_mtx_set_col(mnorm, g, v.data);
   }

   return 0;
 }

 int
 zsl_mtx_norm_elem(struct zsl_mtx *m, struct zsl_mtx *mn, struct zsl_mtx *mi,
       size_t i, size_t j)
 {
   int rc;
   zsl_real_t x;
   zsl_real_t epsilon = 1E-6;

   /* Make a copy of matrix m. */
   rc = zsl_mtx_copy(mn, m);
   if (rc) {
     return -EINVAL;
   }

   /* Get the value to normalise. */
   rc = zsl_mtx_get(mn, i, j, &x);
   if (rc) {
     return rc;
   }

   /* If the value is 0.0, abort. */
   if ((x >= 0 && x < epsilon) || (x <= 0 && x > -epsilon)) {
     return 0;
   }

   rc = zsl_mtx_scalar_mult_row_d(mn, i, (1.0 / x));
   if (rc) {
     return -EINVAL;
   }

   rc = zsl_mtx_scalar_mult_row_d(mi, i, (1.0 / x));
   if (rc) {
     return -EINVAL;
   }

   return 0;
 }

 int
 zsl_mtx_norm_elem_d(struct zsl_mtx *m, struct zsl_mtx *mi, size_t i, size_t j)
 {
   return zsl_mtx_norm_elem(m, m, mi, i, j);
 }

 int
 zsl_mtx_inv_3x3(struct zsl_mtx *m, struct zsl_mtx *mi)
 {
   int rc;
   zsl_real_t d;   /* Determinant. */
   zsl_real_t s;   /* Scale factor. */

   /* Make sure these are square matrices. */
   if ((m->sz_rows != m->sz_cols) || (mi->sz_rows != mi->sz_cols)) {
     return -EINVAL;
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure 'm' and 'mi' have the same shape. */
   if (m->sz_rows != mi->sz_rows) {
     return -EINVAL;
   }
   if (m->sz_cols != mi->sz_cols) {
     return -EINVAL;
   }
   /* Make sure these are 3x3 matrices. */
   if ((m->sz_cols != 3) || (mi->sz_cols != 3)) {
     return -EINVAL;
   }
 #endif

   /* Calculate the determinant. */
   rc = zsl_mtx_deter_3x3(m, &d);
   if (rc) {
     goto err;
   }

   /* Calculate the adjoint matrix. */
   rc = zsl_mtx_adjoint_3x3(m, mi);
   if (rc) {
     goto err;
   }

   /* Scale the output using the determinant. */
   if (d != 0) {
     s = 1.0 / d;
     rc = zsl_mtx_scalar_mult_d(mi, s);
   } else {
     /* Provide an identity matrix if the determinant is zero. */
     rc = zsl_mtx_init(mi, zsl_mtx_entry_fn_identity);
     if (rc) {
       return -EINVAL;
     }
   }

   return 0;
 err:
   return rc;
 }

 int
 zsl_mtx_inv(struct zsl_mtx *m, struct zsl_mtx *mi)
 {
   int rc;
   zsl_real_t d = 0.0;

   /* Shortcut for 3x3 matrices. */
   if (m->sz_rows == 3) {
     return zsl_mtx_inv_3x3(m, mi);
   }

   /* Make sure we have square matrices. */
   if ((m->sz_rows != m->sz_cols) || (mi->sz_rows != mi->sz_cols)) {
     return -EINVAL;
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure 'm' and 'mi' have the same shape. */
   if (m->sz_rows != mi->sz_rows) {
     return -EINVAL;
   }
   if (m->sz_cols != mi->sz_cols) {
     return -EINVAL;
   }
 #endif

   /* Make a copy of matrix m on the stack to avoid modifying it. */
   ZSL_MATRIX_DEF(m_tmp, mi->sz_rows, mi->sz_cols);
   rc = zsl_mtx_copy(&m_tmp, m);
   if (rc) {
     return -EINVAL;
   }

   /* Initialise 'mi' as an identity matrix. */
   rc = zsl_mtx_init(mi, zsl_mtx_entry_fn_identity);
   if (rc) {
     return -EINVAL;
   }

   /* Make sure the determinant of 'm' is not zero. */
   zsl_mtx_deter(m, &d);

   if (d == 0) {
     return 0;
   }

   /* Use Gauss-Jordan elimination for nxn matrices. */
   zsl_mtx_gauss_reduc(m, mi, &m_tmp);

   return 0;
 }

 int
 zsl_mtx_cholesky(struct zsl_mtx *m, struct zsl_mtx *l)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure 'm' is square. */
   if (m->sz_rows != m->sz_cols) {
     return -EINVAL;
   }

   /* Make sure 'm' is symmetric. */
   zsl_real_t a, b;
   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_rows; j++) {
       zsl_mtx_get(m, i, j, &a);
       zsl_mtx_get(m, j, i, &b);
       if (a != b) {
         return -EINVAL;
       }
     }
   }

   /* Make sure 'm' and 'l' have the same shape. */
   if (m->sz_rows != l->sz_rows) {
     return -EINVAL;
   }
   if (m->sz_cols != l->sz_cols) {
     return -EINVAL;
   }
 #endif

   zsl_real_t sum, x, y;
   zsl_mtx_init(l, zsl_mtx_entry_fn_empty);
   for (size_t j = 0; j < m->sz_cols; j++) {
     sum = 0.0;
     for (size_t k = 0; k < j; k++) {
       zsl_mtx_get(l, j, k, &x);
       sum += x * x;
     }
     zsl_mtx_get(m, j, j, &x);
     zsl_mtx_set(l, j, j, ZSL_SQRT(x - sum));

     for (size_t i = j + 1; i < m->sz_cols; i++) {
       sum = 0.0;
       for (size_t k = 0; k < j; k++) {
         zsl_mtx_get(l, j, k, &x);
         zsl_mtx_get(l, i, k, &y);
         sum += y * x;
       }
       zsl_mtx_get(l, j, j, &x);
       zsl_mtx_get(m, i, j, &y);
       zsl_mtx_set(l, i, j, (y - sum) / x);
     }
   }

   return 0;
 }

 int
 zsl_mtx_balance(struct zsl_mtx *m, struct zsl_mtx *mout)
 {
   int rc;
   bool done = false;
   zsl_real_t sum;
   zsl_real_t row, row2;
   zsl_real_t col, col2;

   /* Make sure we have square matrices. */
   if ((m->sz_rows != m->sz_cols) || (mout->sz_rows != mout->sz_cols)) {
     return -EINVAL;
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure 'm' and 'mout' have the same shape. */
   if (m->sz_rows != mout->sz_rows) {
     return -EINVAL;
   }
   if (m->sz_cols != mout->sz_cols) {
     return -EINVAL;
   }
 #endif

   rc = zsl_mtx_copy(mout, m);
   if (rc) {
     goto err;
   }

   while (!done) {
     done = true;

     for (size_t i = 0; i < m->sz_rows; i++) {
       /* Calculate sum of components of each row, column. */
       for (size_t j = 0; j < m->sz_cols; j++) {
         row += ZSL_ABS(mout->data[(i * m->sz_rows) +
                 j]);
         col += ZSL_ABS(mout->data[(j * m->sz_rows) +
                 i]);
       }

       /* TODO: Extend with a check against epsilon? */
       if (col != 0.0 && row != 0.0) {
         row2 = row / 2.0;
         col2 = 1.0;
         sum = col + row;

         while (col < row2) {
           col2 *= 2.0;
           col *= 4.0;
         }

         row2 = row * 2.0;

         while (col > row2) {
           col2 /= 2.0;
           col /= 4.0;
         }

         if ((col + row) / col2 < 0.95 * sum) {
           done = false;
           row2 = 1.0 / col2;

           for (int k = 0; k < m->sz_rows; k++) {
             mout->data[(i * m->sz_rows) + k]
               *= row2;
             mout->data[(k * m->sz_rows) + i]
               *= col2;
           }
         }
       }

       row = 0.0;
       col = 0.0;
     }
   }

 err:
   return rc;
 }

 int
 zsl_mtx_householder(struct zsl_mtx *m, struct zsl_mtx *h, bool hessenberg)
 {
   size_t size = m->sz_rows;

   if (hessenberg == true) {
     size--;
   }

   ZSL_VECTOR_DEF(v, size);
   ZSL_VECTOR_DEF(v2, m->sz_rows);
   ZSL_VECTOR_DEF(e1, size);

   ZSL_MATRIX_DEF(mv, size, 1);
   ZSL_MATRIX_DEF(mvt, 1, size);
   ZSL_MATRIX_DEF(id, size, size);
   ZSL_MATRIX_DEF(vvt, size, size);
   ZSL_MATRIX_DEF(h2, size, size);

   /* Create the e1 vector, i.e. the vector (1, 0, 0, ...). */
   zsl_vec_init(&e1);
   e1.data[0] = 1.0;

   /* Get the first column of the input matrix. */
   zsl_mtx_get_col(m, 0, v2.data);
   if (hessenberg == true) {
     zsl_vec_get_subset(&v2, 1, size, &v);
   } else {
     zsl_vec_copy(&v, &v2);
   }

   /* Change the 'sign' value according to the sign of the first
    * coefficient of the matrix. */
   zsl_real_t sign = 1.0;

   if (v.data[0] < 0) {
     sign = -1.0;
   }

   /* Calculate the vector 'v' that will later be used to calculate the
    * Householder matrix. */
   zsl_vec_scalar_mult(&e1, -sign * zsl_vec_norm(&v));

   zsl_vec_add(&v, &e1, &v);

   zsl_vec_scalar_div(&v, zsl_vec_norm(&v));

   /* Calculate the H householder matrix by doing:
    * H = IDENTITY - 2 * v * v^t. */
   zsl_mtx_from_arr(&mv, v.data);
   zsl_mtx_trans(&mv, &mvt);
   zsl_mtx_mult(&mv, &mvt, &vvt);
   zsl_mtx_init(&id, zsl_mtx_entry_fn_identity);
   zsl_mtx_scalar_mult_d(&vvt, -2);
   zsl_mtx_add(&id, &vvt, &h2);

   /* If Hessenberg set to true, augment the output to the size of 'm'.
    * If Hessenberg set to false, this line of code will do nothing but
    * copy the matrix 'h2' into the output matrix 'h', */
   zsl_mtx_augm_diag(&h2, h);

   return 0;
 }

 int
 zsl_mtx_qrd(struct zsl_mtx *m, struct zsl_mtx *q, struct zsl_mtx *r,
       bool hessenberg)
 {
   ZSL_MATRIX_DEF(r2, m->sz_rows, m->sz_cols);
   ZSL_MATRIX_DEF(hess, m->sz_rows, m->sz_cols);
   ZSL_MATRIX_DEF(h, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(h2, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(qt, m->sz_rows, m->sz_rows);

   zsl_mtx_init(&h, NULL);
   zsl_mtx_init(&qt, zsl_mtx_entry_fn_identity);
   zsl_mtx_copy(r, m);

   for (size_t g = 0; g < (m->sz_rows - 1); g++) {

     /* Reduce the matrix by 'g' rows and columns each time. */
     ZSL_MATRIX_DEF(mred, (m->sz_rows - g), (m->sz_cols - g));
     ZSL_MATRIX_DEF(hred, (m->sz_rows - g), (m->sz_rows - g));

     /* allocate the placeholder matrices for the reduction loop */
     ZSL_MATRIX_DEF(place1, r->sz_rows, r->sz_cols);
     ZSL_MATRIX_DEF(place2, r->sz_rows, r->sz_cols);

     while(zsl_mtx_reduce_iter(r, &mred, &place1, &place2) != 0);

     /* Calculate the reduced Householder matrix 'hred'. */
     if (hessenberg == true) {
       zsl_mtx_householder(&mred, &hred, true);
     } else {
       zsl_mtx_householder(&mred, &hred, false);
     }

     /* Augment the Householder matrix to the input matrix size. */
     zsl_mtx_augm_diag(&hred, &h);
     zsl_mtx_mult(&h, r, &r2);

     /* Multiply this Householder matrix by the previous ones,
      * stacked in 'qt'. */
     zsl_mtx_mult(&h, &qt, &h2);
     zsl_mtx_copy(&qt, &h2);
     if (hessenberg == true) {
       zsl_mtx_mult(&r2, &h, &hess);
       zsl_mtx_copy(r, &hess);
     } else {
       zsl_mtx_copy(r, &r2);
     }
   }

   /* Calculate the 'q' matrix by transposing 'qt'. */
   zsl_mtx_trans(&qt, q);

   return 0;
 }

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int
 zsl_mtx_qrd_iter(struct zsl_mtx *m, struct zsl_mtx *mout, size_t iter)
 {
   int rc;

   ZSL_MATRIX_DEF(q, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(r, m->sz_rows, m->sz_rows);


   /* Make a copy of 'm'. */
   rc = zsl_mtx_copy(mout, m);
   if (rc) {
     return -EINVAL;
   }

   for (size_t g = 1; g <= iter; g++) {
     /* Perform the QR decomposition. */
     zsl_mtx_qrd(mout, &q, &r, false);

     /* Multiply the results of the QR decomposition together but
      * changing its order. */
     zsl_mtx_mult(&r, &q, mout);
   }

   return 0;
 }
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int
 zsl_mtx_eigenvalues(struct zsl_mtx *m, struct zsl_vec *v, size_t iter)
 {
   zsl_real_t diag;
   zsl_real_t sdiag;
   size_t real = 0;

   /* Epsilon is used to check 0 values in the subdiagonal, to determine
    * if any coimplekx values were found. Increasing the number of
    * iterations will move these values closer to 0, but when using
    * single-precision floats the numbers can still be quite large, so
    * we need to set a delta of +/- 0.001 in this case. */

   zsl_real_t epsilon = 1E-6;

   ZSL_MATRIX_DEF(mout, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(mtemp, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(mtemp2, m->sz_rows, m->sz_rows);

   /* Balance the matrix. */
   zsl_mtx_balance(m, &mtemp);

   /* Put the balanced matrix into hessenberg form. */
   zsl_mtx_qrd(&mtemp, &mout, &mtemp2, true);

   /* Calculate the upper triangular matrix by using the recursive QR
    * decomposition method. */
   zsl_mtx_qrd_iter(&mtemp2, &mout, iter);

   zsl_vec_init(v);

   /* If the matrix is symmetric, then it will always have real
    * eigenvalues, so treat this case appart. */
   if (zsl_mtx_is_sym(m) == true) {
     for (size_t g = 0; g < m->sz_rows; g++) {
       zsl_mtx_get(&mout, g, g, &diag);
       v->data[g] = diag;
     }

     return 0;
   }

   /*
    * If any value just below the diagonal is non-zero, it means that the
    * numbers above and to the right of the non-zero value are a pair of
    * complex values, a complex number and its conjugate.
    *
    * SVD will always return real numbers so this can be ignored, but if
    * you are calculating eigenvalues outside the SVD method, you may
    * get complex numbers, which will be indicated with the return error
    * code '-ECOMPLEXVAL'.
    *
    * If the imput matrix has complex eigenvalues, then these will be
    * ignored and the output vector will not include them.
    *
    * NOTE: The real and imaginary parts of the complex numbers are not
    * available. This only checks if there are any complex eigenvalues and
    * returns an appropriate error code to alert the user that there are
    * non-real eigenvalues present.
    */

   for (size_t g = 0; g < (m->sz_rows - 1); g++) {
     /* Check if any element just below the diagonal isn't zero. */
     zsl_mtx_get(&mout, g + 1, g, &sdiag);
     if ((sdiag >= epsilon) || (sdiag <= -epsilon)) {
       /* Skip two elements if the element below
        * is not zero. */
       g++;
     } else {
       /* Get the diagonal element if the element below
        * is zero. */
       zsl_mtx_get(&mout, g, g, &diag);
       v->data[real] = diag;
       real++;
     }
   }


   /* Since it's not possible to check the coefficient below the last
    * diagonal element, then check the element to its left. */
   zsl_mtx_get(&mout, (m->sz_rows - 1), (m->sz_rows - 2), &sdiag);
   if ((sdiag >= epsilon) || (sdiag <= -epsilon)) {
     /* Do nothing if the element to its left is not zero. */
   } else {
     /* Get the last diagonal element if the element to its left
      * is zero. */
     zsl_mtx_get(&mout, (m->sz_rows - 1), (m->sz_rows - 1), &diag);
     v->data[real] = diag;
     real++;
   }

   /* If the number of real eigenvalues ('real' coefficient) is less than
    * the matrix dimensions, then there must be complex eigenvalues. */
   v->sz = real;
   if (real != m->sz_rows) {
     return -ECOMPLEXVAL;
   }

   /* Put the zeros to the end. */
   zsl_vec_zte(v);

   return 0;
 }
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int
 zsl_mtx_eigenvectors(struct zsl_mtx *m, struct zsl_mtx *mev, size_t iter,
          bool orthonormal)
 {
   size_t b = 0;           /* Total number of eigenvectors. */
   size_t e_vals = 0;      /* Number of unique eigenvalues. */
   size_t count = 0;       /* Number of eigenvectors for an eigenvalue. */
   size_t ga = 0;

   zsl_real_t epsilon = 1E-6;
   zsl_real_t x;

   /* The vector where all eigenvalues will be stored. */
   ZSL_VECTOR_DEF(k, m->sz_rows);
   /* Temp vector to store column data. */
   ZSL_VECTOR_DEF(f, m->sz_rows);
   /* The vector where all UNIQUE eigenvalues will be stored. */
   ZSL_VECTOR_DEF(o, m->sz_rows);
   /* Temporary mxm identity matrix placeholder. */
   ZSL_MATRIX_DEF(id, m->sz_rows, m->sz_rows);
   /* 'm' minus the eigenvalues * the identity matrix (id). */
   ZSL_MATRIX_DEF(mi, m->sz_rows, m->sz_rows);
   /* Placeholder for zsl_mtx_gauss_reduc calls (required param). */
   ZSL_MATRIX_DEF(mid, m->sz_rows, m->sz_rows);
   /* Matrix containing all column eigenvectors for an eigenvalue. */
   ZSL_MATRIX_DEF(evec, m->sz_rows, m->sz_rows);
   /* Matrix containing all column eigenvectors for an eigenvalue.
   * Two matrices are required for the Gramm-Schmidt operation. */
   ZSL_MATRIX_DEF(evec2, m->sz_rows, m->sz_rows);
   /* Matrix containing all column eigenvectors. */
   ZSL_MATRIX_DEF(mev2, m->sz_rows, m->sz_rows);

   /* TODO: Check that we have a SQUARE matrix, etc. */
   zsl_mtx_init(&mev2, NULL);
   zsl_vec_init(&o);
   zsl_mtx_eigenvalues(m, &k, iter);

   /* Copy every non-zero eigenvalue ONCE in the 'o' vector to get rid of
    * repeated values. */
   for (size_t q = 0; q < m->sz_rows; q++) {
     if ((k.data[q] >= epsilon) || (k.data[q] <= -epsilon)) {
       if (zsl_vec_contains(&o, k.data[q], epsilon) == 0) {
         o.data[e_vals] = k.data[q];
         /* Increment the unique eigenvalue counter. */
         e_vals++;
       }
     }
   }

   /* If zero is also an eigenvalue, copy it once in 'o'. */
   if (zsl_vec_contains(&k, 0.0, epsilon) > 0) {
     e_vals++;
   }

   /* Calculates the null space of 'm' minus each eigenvalue times
    * the identity matrix by performing the gaussian reduction. */
   for (size_t g = 0; g < e_vals; g++) {
     count = 0;
     ga = 0;

     zsl_mtx_init(&id, zsl_mtx_entry_fn_identity);
     zsl_mtx_scalar_mult_d(&id, -o.data[g]);
     zsl_mtx_add_d(&id, m);
     zsl_mtx_gauss_reduc(&id, &mid, &mi);

     /* If 'orthonormal' is true, perform the following process. */
     if (orthonormal == true) {
       /* Count how many eigenvectors ('count' coefficient)
        * there are for each eigenvalue. */
       for (size_t h = 0; h < m->sz_rows; h++) {
         zsl_mtx_get(&mi, h, h, &x);
         if ((x >= 0.0 && x < epsilon) ||
             (x <= 0.0 && x > -epsilon)) {
           count++;
         }
       }

       /* Resize evec* placeholders to have 'count' cols. */
       evec.sz_cols = count;
       evec2.sz_cols = count;

       /* Get all the eigenvectors for each eigenvalue and set
        * them as the columns of 'evec'. */
       for (size_t h = 0; h < m->sz_rows; h++) {
         zsl_mtx_get(&mi, h, h, &x);
         if ((x >= 0.0 && x < epsilon) ||
             (x <= 0.0 && x > -epsilon)) {
           zsl_mtx_set(&mi, h, h, -1);
           zsl_mtx_get_col(&mi, h, f.data);
           zsl_vec_neg(&f);
           zsl_mtx_set_col(&evec, ga, f.data);
           ga++;
         }
       }
       /* Orthonormalize the set of eigenvectors for each
        * eigenvalue using the Gram-Schmidt process. */
       zsl_mtx_gram_schmidt(&evec, &evec2);
       zsl_mtx_cols_norm(&evec2, &evec);

       /* Place these eigenvectors in the 'mev2' matrix,
        * that will hold all the eigenvectors for different
        * eigenvalues. */
       for (size_t gi = 0; gi < count; gi++) {
         zsl_mtx_get_col(&evec, gi, f.data);
         zsl_mtx_set_col(&mev2, b, f.data);
         b++;
       }

     } else {
       /* Orthonormal is false. */
       /* Get the eigenvectors for every eigenvalue and place
        * them in 'mev2'. */
       for (size_t h = 0; h < m->sz_rows; h++) {
         zsl_mtx_get(&mi, h, h, &x);
         if ((x >= 0.0 && x < epsilon) ||
             (x <= 0.0 && x > -epsilon)) {
           zsl_mtx_set(&mi, h, h, -1);
           zsl_mtx_get_col(&mi, h, f.data);
           zsl_vec_neg(&f);
           zsl_mtx_set_col(&mev2, b, f.data);
           b++;
         }
       }
     }
   }

   /* Since 'b' is the number of eigenvectors, reduce 'mev' (of size
    * m->sz_rows times b) to erase columns of zeros. */
   mev->sz_cols = b;

   for (size_t s = 0; s < b; s++) {
     zsl_mtx_get_col(&mev2, s, f.data);
     zsl_mtx_set_col(mev, s, f.data);
   }

   /* Checks if the number of eigenvectors is the same as the shape of
    * the input matrix. If the number of eigenvectors is less than
    * the number of columns in the input matrix 'm', this will be
    * indicated by EEIGENSIZE as a return code. */
   if (b != m->sz_cols) {
     return -EEIGENSIZE;
   }

   return 0;
 }
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int
 zsl_mtx_svd(struct zsl_mtx *m, struct zsl_mtx *u, struct zsl_mtx *e,
       struct zsl_mtx *v, size_t iter)
 {
   ZSL_MATRIX_DEF(aat, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(upri, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(ata, m->sz_cols, m->sz_cols);
   ZSL_MATRIX_DEF(at, m->sz_cols, m->sz_rows);
   ZSL_VECTOR_DEF(ui, m->sz_rows);
   ZSL_MATRIX_DEF(ui2, m->sz_cols, 1);
   ZSL_MATRIX_DEF(ui3, m->sz_rows, 1);
   ZSL_VECTOR_DEF(hu, m->sz_rows);

   zsl_real_t d;
   size_t pu = 0;
   size_t min = m->sz_cols;
   zsl_real_t epsilon = 1E-6;

   zsl_mtx_trans(m, &at);

   /* Calculate 'm' times 'm' transposed and viceversa. */
   zsl_mtx_mult(m, &at, &aat);
   zsl_mtx_mult(&at, m, &ata);

   /* Set the value 'min' as the minimum of number of columns and number
    * of rows. */
   if (m->sz_rows <= m->sz_cols) {
     min = m->sz_rows;
   }

   /* Calculate the eigenvalues of the square matrix 'm' times 'm'
    * transposed or the square matrix 'm' transposed times 'm', whichever
    * is smaller in dimensions. */
   ZSL_VECTOR_DEF(ev, min);
   if (min < m->sz_cols) {
     zsl_mtx_eigenvalues(&aat, &ev, iter);
   } else {
     zsl_mtx_eigenvalues(&ata, &ev, iter);
   }

   /* Place the square root of these eigenvalues in the diagonal entries
    * of 'e', the sigma matrix. */
   zsl_mtx_init(e, NULL);
   for (size_t g = 0; g < min; g++) {
     zsl_mtx_set(e, g, g, ZSL_SQRT(ev.data[g]));
   }

   /* Calculate the eigenvectors of 'm' times 'm' transposed and set them
    * as the columns of the 'v' matrix. */
   zsl_mtx_eigenvectors(&ata, v, iter, true);
   for (size_t i = 0; i < min; i++) {
     zsl_mtx_get_col(v, i, ui.data);
     zsl_mtx_from_arr(&ui2, ui.data);
     zsl_mtx_get(e, i, i, &d);

     /* Calculate the column vectors of 'u' by dividing these
      * eniegnvectors by the square root its eigenvalue and
      * multiplying them by the input matrix. */
     zsl_mtx_mult(m, &ui2, &ui3);
     if ((d >= 0.0 && d < epsilon) || (d <= 0.0 && d > -epsilon)) {
       pu++;
     } else {
       zsl_mtx_scalar_mult_d(&ui3, (1 / d));
       zsl_vec_from_arr(&ui, ui3.data);
       zsl_mtx_set_col(u, i, ui.data);
     }
   }

   /* Expand the columns of 'u' into an orthonormal basis if there are
    * zero eigenvalues or if the number of columns in 'm' is less than the
    * number of rows. */
   zsl_mtx_eigenvectors(&aat, &upri, iter, true);
   for (size_t f = min - pu; f < m->sz_rows; f++) {
     zsl_mtx_get_col(&upri, f, hu.data);
     zsl_mtx_set_col(u, f, hu.data);
   }

   return 0;
 }
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 int
 zsl_mtx_pinv(struct zsl_mtx *m, struct zsl_mtx *pinv, size_t iter)
 {
   zsl_real_t x;
   size_t min = m->sz_cols;
   zsl_real_t epsilon = 1E-6;

   ZSL_MATRIX_DEF(u, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(e, m->sz_rows, m->sz_cols);
   ZSL_MATRIX_DEF(v, m->sz_cols, m->sz_cols);
   ZSL_MATRIX_DEF(et, m->sz_cols, m->sz_rows);
   ZSL_MATRIX_DEF(ut, m->sz_rows, m->sz_rows);
   ZSL_MATRIX_DEF(pas, m->sz_cols, m->sz_rows);

   /* Determine the SVD decomposition of 'm'. */
   zsl_mtx_svd(m, &u, &e, &v, iter);

   /* Transpose the 'u' matrix. */
   zsl_mtx_trans(&u, &ut);

   /* Set the value 'min' as the minimum of number of columns and number
    * of rows. */
   if (m->sz_rows <= m->sz_cols) {
     min = m->sz_rows;
   }

   for (size_t g = 0; g < min; g++) {

     /* Invert the diagonal values in 'e'. If a value is zero, do
      * nothing to it. */
     zsl_mtx_get(&e, g, g, &x);
     if ((x < epsilon) || (x > -epsilon)) {
       x = 1 / x;
       zsl_mtx_set(&e, g, g, x);
     }
   }

   /* Transpose the sigma matrix. */
   zsl_mtx_trans(&e, &et);

   /* Multiply 'u' (transposed) times sigma (transposed and with inverted
    * eigenvalues) times 'v'. */
   zsl_mtx_mult(&v, &et, &pas);
   zsl_mtx_mult(&pas, &ut, pinv);

   return 0;
 }
 #endif

 int
 zsl_mtx_min(struct zsl_mtx *m, zsl_real_t *x)
 {
   zsl_real_t min = m->data[0];

   for (size_t i = 0; i < m->sz_cols * m->sz_rows; i++) {
     if (m->data[i] < min) {
       min = m->data[i];
     }
   }

   *x = min;

   return 0;
 }

 int
 zsl_mtx_max(struct zsl_mtx *m, zsl_real_t *x)
 {
   zsl_real_t max = m->data[0];

   for (size_t i = 0; i < m->sz_cols * m->sz_rows; i++) {
     if (m->data[i] > max) {
       max = m->data[i];
     }
   }

   *x = max;

   return 0;
 }

 int
 zsl_mtx_min_idx(struct zsl_mtx *m, size_t *i, size_t *j)
 {
   zsl_real_t min = m->data[0];

   *i = 0;
   *j = 0;

   for (size_t _i = 0; _i < m->sz_rows; _i++) {
     for (size_t _j = 0; _j < m->sz_cols; _j++) {
       if (m->data[_i * m->sz_cols + _j] < min) {
         min = m->data[_i * m->sz_cols + _j];
         *i = _i;
         *j = _j;
       }
     }
   }

   return 0;
 }

 int
 zsl_mtx_max_idx(struct zsl_mtx *m, size_t *i, size_t *j)
 {
   zsl_real_t max = m->data[0];

   *i = 0;
   *j = 0;

   for (size_t _i = 0; _i < m->sz_rows; _i++) {
     for (size_t _j = 0; _j < m->sz_cols; _j++) {
       if (m->data[_i * m->sz_cols + _j] > max) {
         max = m->data[_i * m->sz_cols + _j];
         *i = _i;
         *j = _j;
       }
     }
   }

   return 0;
 }

 bool
 zsl_mtx_is_equal(struct zsl_mtx *ma, struct zsl_mtx *mb)
 {
   int res;

   /* Make sure shape is the same. */
   if ((ma->sz_rows != mb->sz_rows) || (ma->sz_cols != mb->sz_cols)) {
     return false;
   }

   res = memcmp(ma->data, mb->data,
          sizeof(zsl_real_t) * (ma->sz_rows + ma->sz_cols));

   return res == 0 ? true : false;
 }

 bool
 zsl_mtx_is_notneg(struct zsl_mtx *m)
 {
   for (size_t i = 0; i < m->sz_rows * m->sz_cols; i++) {
     if (m->data[i] < 0.0) {
       return false;
     }
   }

   return true;
 }

 bool
 zsl_mtx_is_sym(struct zsl_mtx *m)
 {
   zsl_real_t x;
   zsl_real_t y;
   zsl_real_t diff;
   zsl_real_t epsilon = 1E-6;

   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_cols; j++) {
       zsl_mtx_get(m, i, j, &x);
       zsl_mtx_get(m, j, i, &y);
       diff = x - y;
       if (diff >= epsilon || diff <= -epsilon) {
         return false;
       }
     }
   }

   return true;
 }

 int
 zsl_mtx_print(struct zsl_mtx *m)
 {
   int rc;
   zsl_real_t x;

   for (size_t i = 0; i < m->sz_rows; i++) {
     for (size_t j = 0; j < m->sz_cols; j++) {
       rc = zsl_mtx_get(m, i, j, &x);
       if (rc) {
         printf("Error reading (%zu,%zu)!\n", i, j);
         return -EINVAL;
       }
       /* Print the current floating-point value. */
       printf("%f ", (double)x);
     }
     printf("\n");
   }

   printf("\n");

   return 0;
 }