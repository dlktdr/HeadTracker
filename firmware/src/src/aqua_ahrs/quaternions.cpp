/*
 * Copyright (c) 2021 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <math.h>
 #include <errno.h>
 #include <stdio.h>
 #include "zsl.h"
 #include "vectors.h"
 #include "quaternions.h"

 /**
  * @brief Helper function to compare float values.
  *
  * @param a 		First float too compare.
  * @param b 		Second float to compare.
  * @param epsilon 	Allowed deviatin between first and second values.

  * @return true		If values are the same within the limits of epsilon.
  * @return false 	If values are different to an extent greater than epsilon.
  */
 static bool
 zsl_quat_val_is_equal(zsl_real_t a, zsl_real_t b, zsl_real_t epsilon)
 {
   zsl_real_t c;

   c = a - b;

   if (ZSL_ABS(c) < epsilon) {
     return 1;
   } else {
     return 0;
   }
 }

 void zsl_quat_init(struct zsl_quat *q, enum zsl_quat_type type)
 {
   switch (type) {
   case ZSL_QUAT_TYPE_IDENTITY:
     q->r = 1.0f;
     q->i = 0.0f;
     q->j = 0.0f;
     q->k = 0.0f;
     break;
   case ZSL_QUAT_TYPE_EMPTY:
   default:
     q->r = 0.0f;
     q->i = 0.0f;
     q->j = 0.0f;
     q->k = 0.0f;
     break;
   }
 }

 zsl_real_t zsl_quat_magn(struct zsl_quat *q)
 {
   return ZSL_SQRT(q->r * q->r + q->i * q->i + q->j * q->j + q->k * q->k);
 }

 int zsl_quat_to_unit(struct zsl_quat *q, struct zsl_quat *qn)
 {
   int rc = 0;
   zsl_real_t m = zsl_quat_magn(q);

   if (ZSL_ABS(m) < 1E-6f) {
     qn->r = 0.0f;
     qn->i = 0.0f;
     qn->j = 0.0f;
     qn->k = 0.0f;
   } else {
     qn->r = q->r / m;
     qn->i = q->i / m;
     qn->j = q->j / m;
     qn->k = q->k / m;
   }

   return rc;
 }

 int zsl_quat_to_unit_d(struct zsl_quat *q)
 {
   return zsl_quat_to_unit(q, q);
 }

 bool zsl_quat_is_unit(struct zsl_quat *q)
 {
   zsl_real_t unit_len;

   /* Verify that sqrt(r^2+i^2+j^2+k^2) = 1.0f. */
   unit_len = ZSL_SQRT(
     q->r * q->r +
     q->i * q->i +
     q->j * q->j +
     q->k * q->k);

   return zsl_quat_val_is_equal(unit_len, 1.0f, 1E-6f);
 }

 int zsl_quat_scale(struct zsl_quat *q, zsl_real_t s, struct zsl_quat *qs)
 {
   int rc = 0;

   qs->r = q->r * s;
   qs->i = q->i * s;
   qs->j = q->j * s;
   qs->k = q->k * s;

   return rc;
 }

 int zsl_quat_scale_d(struct zsl_quat *q, zsl_real_t s)
 {
   return zsl_quat_scale(q, s, q);
 }

 int zsl_quat_mult(struct zsl_quat *qa, struct zsl_quat *qb,
       struct zsl_quat *qm)
 {
   int rc = 0;

   /* Make copies so this function can be used as a destructive one. */
   struct zsl_quat qac;
   struct zsl_quat qbc;

   qac.r = qa->r;
   qac.i = qa->i;
   qac.j = qa->j;
   qac.k = qa->k;

   qbc.r = qb->r;
   qbc.i = qb->i;
   qbc.j = qb->j;
   qbc.k = qb->k;

   qm->i = qac.r * qbc.i + qac.i * qbc.r + qac.j * qbc.k - qac.k * qbc.j;
   qm->j = qac.r * qbc.j - qac.i * qbc.k + qac.j * qbc.r + qac.k * qbc.i;
   qm->k = qac.r * qbc.k + qac.i * qbc.j - qac.j * qbc.i + qac.k * qbc.r;
   qm->r = qac.r * qbc.r - qac.i * qbc.i - qac.j * qbc.j - qac.k * qbc.k;

   return rc;
 }

 int zsl_quat_exp(struct zsl_quat *q, struct zsl_quat *qe)
 {
   int rc = 0;

   ZSL_VECTOR_DEF(v, 3);
   zsl_real_t vmag;        /* Magnitude of v. */
   zsl_real_t vsin;        /* Sine of vm. */
   zsl_real_t rexp;        /* Exponent of q->r. */

   /* Populate the XYZ vector using ijk from q. */
   v.data[0] = q->i;
   v.data[1] = q->j;
   v.data[2] = q->k;

   /* Calculate magnitude of v. */
   vmag = zsl_vec_norm(&v);

   /* Normalise v to unit vector. */
   zsl_vec_to_unit(&v);

   vsin = ZSL_SIN(vmag);
   rexp = ZSL_EXP(q->r);

   qe->r = ZSL_COS(vmag) * rexp;
   qe->i = v.data[0] * vsin * rexp;
   qe->j = v.data[1] * vsin * rexp;
   qe->k = v.data[2] * vsin * rexp;

   return rc;
 }

 int zsl_quat_log(struct zsl_quat *q, struct zsl_quat *ql)
 {
   int rc = 0;

   ZSL_VECTOR_DEF(v, 3);   /* Vector part of unit quat q. */
   zsl_real_t qmag;        /* Magnitude of q. */
   zsl_real_t racos;       /* Acos of q->r/qmag. */

   /* Populate the XYZ vector using ijk from q. */
   v.data[0] = q->i;
   v.data[1] = q->j;
   v.data[2] = q->k;

   /* Normalise v to unit vector. */
   zsl_vec_to_unit(&v);

   /* Calculate magnitude of input quat. */
   qmag = zsl_quat_magn(q);

   racos = ZSL_ACOS(q->r / qmag);

   ql->r = ZSL_LOG(qmag);
   ql->i = v.data[0] * racos;
   ql->j = v.data[1] * racos;
   ql->k = v.data[2] * racos;

   return rc;
 }

 int zsl_quat_pow(struct zsl_quat *q, zsl_real_t exp,
      struct zsl_quat *qout)
 {
   int rc = 0;

   struct zsl_quat qlog;
   struct zsl_quat qsc;

   rc = zsl_quat_log(q, &qlog);
   if (rc) {
     goto err;
   }

   rc = zsl_quat_scale(&qlog, exp, &qsc);
   if (rc) {
     goto err;
   }

   rc = zsl_quat_exp(&qsc, qout);
   if (rc) {
     zsl_quat_init(qout, ZSL_QUAT_TYPE_EMPTY);
     goto err;
   }

 err:
   return rc;
 }

 int zsl_quat_conj(struct zsl_quat *q, struct zsl_quat *qc)
 {
   int rc = 0;

   /* TODO: Check for div by zero before running this! */
   qc->r = q->r;
   qc->i = q->i * -1.0f;
   qc->j = q->j * -1.0f;
   qc->k = q->k * -1.0f;

   return rc;
 }

 int zsl_quat_inv(struct zsl_quat *q, struct zsl_quat *qi)
 {
   int rc = 0;
   zsl_real_t m = zsl_quat_magn(q);

   if (ZSL_ABS(m) < 1E-6f) {
     /* Set to all 0's. */
     zsl_quat_init(qi, ZSL_QUAT_TYPE_EMPTY);
   } else {
     /* TODO: Check for div by zero before running this! */
     m *= m;
     qi->r = q->r / m;
     qi->i = q->i / -m;
     qi->j = q->j / -m;
     qi->k = q->k / -m;
   }

   return rc;
 }

 int zsl_quat_inv_d(struct zsl_quat *q)
 {
   return zsl_quat_inv(q, q);
 }

 int zsl_quat_diff(struct zsl_quat *qa, struct zsl_quat *qb,
       struct zsl_quat *qd)
 {
   int rc;
   struct zsl_quat qi;

   rc = zsl_quat_inv(qa, &qi);
   if (rc) {
     goto err;
   }

   /* Note: order matters here!*/
   rc = zsl_quat_mult(&qi, qb, qd);

 err:
   return rc;
 }

 int zsl_quat_rot(struct zsl_quat *qa, struct zsl_quat *qb, struct zsl_quat *qr)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   if (ZSL_ABS(qb->r) > 1E-6f) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   struct zsl_quat qm;
   struct zsl_quat qn;
   struct zsl_quat qi;

   zsl_quat_to_unit(qa, &qn);
   zsl_quat_mult(&qn, qb, &qm);
   zsl_quat_inv(&qn, &qi);
   zsl_quat_mult(&qm, &qi, qr);

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_lerp(struct zsl_quat *qa, struct zsl_quat *qb,
       zsl_real_t t, struct zsl_quat *qi)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure t is between 0 and 1 (included). */
   if (t < 0.0f || t > 1.0f) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   struct zsl_quat q1, q2;

   /* Turn input quaternions into unit quaternions. */
   struct zsl_quat qa_u;
   struct zsl_quat qb_u;
   zsl_quat_to_unit(qa, &qa_u);
   zsl_quat_to_unit(qb, &qb_u);

   /* Calculate intermediate quats. */
   zsl_quat_scale(&qa_u, 1.0f - t, &q1);
   zsl_quat_scale(&qb_u, t, &q2);

   /* Final result = q1 + q2. */
   qi->r = q1.r + q2.r;
   qi->i = q1.i + q2.i;
   qi->j = q1.j + q2.j;
   qi->k = q1.k + q2.k;

   /* Normalize output quaternion. */
   zsl_quat_to_unit_d(qi);

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_slerp(struct zsl_quat *qa, struct zsl_quat *qb,
        zsl_real_t t, struct zsl_quat *qi)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure t is between 0 and 1 (included). */
   if (t < 0.0f || t > 1.0f) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   struct zsl_quat q1, q2; /* Interim quats. */
   zsl_real_t dot;         /* Dot product bewteen qa and qb. */
   zsl_real_t phi;         /* arccos(dot). */
   zsl_real_t phi_s;       /* sin(phi). */
   zsl_real_t phi_st;      /* sin(phi * (t)). */
   zsl_real_t phi_smt;     /* sin(phi * (1.0f - t)). */

   /*
    * Unit quaternion slerp = qa * (qa^-1 * qb)^t
    *
    * We get there in a round-about way in this code, but we avoid pushing
    * and popping values on the stack with trivial calls to helper functions.
    */

   /* Turn input quaternions into unit quaternions. */
   struct zsl_quat qa_u;
   struct zsl_quat qb_u;
   zsl_quat_to_unit(qa, &qa_u);
   zsl_quat_to_unit(qb, &qb_u);

   /* When t = 0.0f or t = 1.0f, just memcpy qa or qb. */
   if (t == 0.0f) {
     qi->r = qa_u.r;
     qi->i = qa_u.i;
     qi->j = qa_u.j;
     qi->k = qa_u.k;
     goto err;
   } else if (t == 1.0f) {
     qi->r = qb_u.r;
     qi->i = qb_u.i;
     qi->j = qb_u.j;
     qi->k = qb_u.k;
     goto err;
   }

   /* Compute the dot product of the two normalized input quaternions. */
   dot = qa_u.r * qb_u.r + qa_u.i * qb_u.i + qa_u.j * qb_u.j + qa_u.k * qb_u.k;

   /* The value dot is always between -1 and 1. If dot = 1.0f, qa = qb and there
    * is no interpolation. */
   if (ZSL_ABS(dot - 1.0f) < 1E-6f) {
     qi->r = qa_u.r;
     qi->i = qa_u.i;
     qi->j = qa_u.j;
     qi->k = qa_u.k;
     goto err;
   }

   /* If dot = -1, then qa = - qb and the interpolation is invald. */
   if (ZSL_ABS(dot + 1.0f) < 1E-6f) {
     rc = -EINVAL;
     goto err;
   }

   /*
    * Slerp often has problems with angles close to zero. Consider handling
    * those edge cases slightly differently?
    */

   /* Calculate these once before-hand. */
   phi = ZSL_ACOS(dot);
   phi_s = ZSL_SIN(phi);
   phi_st = ZSL_SIN(phi * t);
   phi_smt = ZSL_SIN(phi * (1.0f - t));

   /* Calculate intermediate quats. */
   zsl_quat_scale(&qa_u, phi_smt / phi_s, &q1);
   zsl_quat_scale(&qb_u, phi_st / phi_s, &q2);

   /* Final result = q1 + q2. */
   qi->r = q1.r + q2.r;
   qi->i = q1.i + q2.i;
   qi->j = q1.j + q2.j;
   qi->k = q1.k + q2.k;

 err:
   return rc;
 }

 int zsl_quat_from_ang_vel(struct zsl_vec *w, struct zsl_quat *qin,
         zsl_real_t t, struct zsl_quat *qout)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure time is positive or zero and the angular velocity is a
    * tridimensional vector. */
   if (w->sz != 3 || t < 0.0f) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   struct zsl_quat qin2;
   struct zsl_quat qout2;
   struct zsl_quat wq;
   struct zsl_quat wquat = {
     .r = 0.0f,
     .i = w->data[0],
     .j = w->data[1],
     .k = w->data[2]
   };

   zsl_quat_to_unit(qin, &qin2);
   zsl_quat_mult(&wquat, &qin2, &wq);
   zsl_quat_scale_d(&wq, 0.5f * t);
   qout2.r = qin2.r + wq.r;
   qout2.i = qin2.i + wq.i;
   qout2.j = qin2.j + wq.j;
   qout2.k = qin2.k + wq.k;

   zsl_quat_to_unit(&qout2, qout);

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_from_ang_mom(struct zsl_vec *l, struct zsl_quat *qin,
         zsl_real_t *i, zsl_real_t t, struct zsl_quat *qout)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure time is positive or zero and the angular velocity is a
    * tridimensional vector. Inertia can't be negative or zero. */
   if (l->sz != 3 || t < 0.0f || *i <= 0.0f) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   ZSL_VECTOR_DEF(w, 3);
   zsl_vec_copy(&w, l);
   zsl_vec_scalar_div(&w, *i);
   zsl_quat_from_ang_vel(&w, qin, t, qout);

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_to_euler(struct zsl_quat *q, struct zsl_euler *e)
 {
   int rc = 0;
   struct zsl_quat qn;

   zsl_quat_to_unit(q, &qn);
   zsl_real_t gl = qn.i * qn.k + qn.j * qn.r;
   zsl_real_t v = 2.0f * gl;

   if (v > 1.0f) {
     v = 1.0f;
   } else if (v < -1.0f) {
     v = -1.0f;
   }

   e->y = ZSL_ASIN(v);

   /* Gimbal lock case. */
   if (ZSL_ABS(gl - 0.5f) < 1E-6f || ZSL_ABS(gl + 0.5f) < 1E-6f) {
     e->x = ZSL_ATAN2(2.0f * (qn.j * qn.k + qn.i * qn.r),
          1.0f - 2.0f * (qn.i * qn.i + qn.k * qn.k));
     e->z = 0.0f;
     return rc;
   }

   e->x = ZSL_ATAN2(2.0f * (qn.i * qn.r - qn.j * qn.k),
        1.0f - 2.0f * (qn.i * qn.i + qn.j * qn.j));
   e->z = ZSL_ATAN2(2.0f * (qn.k * qn.r - qn.i * qn.j),
        1.0f - 2.0f * (qn.j * qn.j + qn.k * qn.k));

   return rc;
 }

 int zsl_quat_from_euler(struct zsl_euler *e, struct zsl_quat *q)
 {
   int rc = 0;

   zsl_real_t roll_c = ZSL_COS(e->x * 0.5f);
   zsl_real_t roll_s = ZSL_SIN(e->x * 0.5f);
   zsl_real_t pitch_c = ZSL_COS(e->y * 0.5f);
   zsl_real_t pitch_s = ZSL_SIN(e->y * 0.5f);
   zsl_real_t yaw_c = ZSL_COS(e->z * 0.5f);
   zsl_real_t yaw_s = ZSL_SIN(e->z * 0.5f);

   q->r = roll_c * pitch_c * yaw_c - roll_s * pitch_s * yaw_s;
   q->i = roll_s * pitch_c * yaw_c + roll_c * pitch_s * yaw_s;
   q->j = roll_c * pitch_s * yaw_c - roll_s * pitch_c * yaw_s;
   q->k = roll_c * pitch_c * yaw_s + roll_s * pitch_s * yaw_c;

   return rc;
 }

 int zsl_quat_to_rot_mtx(struct zsl_quat *q, struct zsl_mtx *m)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure that the rotation matrix has an appropriate shape and size. */
   if ((m->sz_cols != 3) || (m->sz_rows != 3)) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   /* Note: This can be optimised by pre-calculating shared values. */

   /* Row 0. */
   zsl_mtx_set(m, 0, 0, 1.0f - 2.0f * (q->j * q->j + q->k * q->k));
   zsl_mtx_set(m, 0, 1, 2.0f * (q->i * q->j - q->k * q->r));
   zsl_mtx_set(m, 0, 2, 2.0f * (q->i * q->k + q->j * q->r));

   /* Row 1. */
   zsl_mtx_set(m, 1, 0, 2.0f * (q->i * q->j + q->k * q->r));
   zsl_mtx_set(m, 1, 1, 1.0f - 2.0f * (q->i * q->i + q->k * q->k));
   zsl_mtx_set(m, 1, 2, 2.0f * (q->j * q->k - q->i * q->r));

   /* Row 2. */
   zsl_mtx_set(m, 2, 0, 2.0f * (q->i * q->k - q->j * q->r));
   zsl_mtx_set(m, 2, 1, 2.0f * (q->j * q->k + q->i * q->r));
   zsl_mtx_set(m, 2, 2, 1.0f - 2.0f * (q->i * q->i + q->j * q->j));

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_from_rot_mtx(struct zsl_mtx *m, struct zsl_quat *q)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure that the rotation matrix has an appropriate shape and size. */
   if ((m->sz_cols != 3) || (m->sz_rows != 3)) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   /* Convert rotation matrix to unit quaternion. */
   q->r = 0.5f * ZSL_SQRT(m->data[0] + m->data[4] + m->data[8] + 1.0f);
   q->i = 0.5f * ZSL_SQRT(m->data[0] - m->data[4] - m->data[8] + 1.0f);
   q->j = 0.5f * ZSL_SQRT(-m->data[0] + m->data[4] - m->data[8] + 1.0f);
   q->k = 0.5f * ZSL_SQRT(-m->data[0] - m->data[4] + m->data[8] + 1.0f);

   if (ZSL_ABS(m->data[7] - m->data[5]) > 1E-6f) {
     /* Multiply by the sign of m21 - m12. */
     q->i *= (m->data[7] - m->data[5]) / ZSL_ABS(m->data[7] - m->data[5]);
   }

   if (ZSL_ABS(m->data[2] - m->data[6]) > 1E-6f) {
     /* Multiply by the sign of m02 - m20. */
     q->j *= (m->data[2] - m->data[6]) / ZSL_ABS(m->data[2] - m->data[6]);
   }

   if (ZSL_ABS(m->data[3] - m->data[1]) > 1E-6f) {
     /* Multiply by the sign of m10 - m01. */
     q->k *= (m->data[3] - m->data[1]) / ZSL_ABS(m->data[3] - m->data[1]);
   }

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_to_axis_angle(struct zsl_quat *q, struct zsl_vec *a,
          zsl_real_t *b)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure that the axis vector is size 3. */
   if (a->sz != 3) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   struct zsl_quat qn;
   zsl_quat_to_unit(q, &qn);

   if (ZSL_ABS(qn.r - 1.0f) < 1E-6f) {
     a->data[0] = 0.0f;
     a->data[1] = 0.0f;
     a->data[2] = 0.0f;
     *b = 0.0f;
     return 0;
   }

   zsl_real_t s = ZSL_SQRT(1.0f - (qn.r * qn.r));
   *b = 2.0f * ZSL_ACOS(qn.r);
   a->data[0] = qn.i / s;
   a->data[1] = qn.j / s;
   a->data[2] = qn.k / s;

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_from_axis_angle(struct zsl_vec *a, zsl_real_t *b,
            struct zsl_quat *q)
 {
   int rc = 0;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure that the axis vector is size 3. */
   if (a->sz != 3) {
     rc = -EINVAL;
     goto err;
   }
 #endif

   zsl_real_t norm = ZSL_SQRT(a->data[0] * a->data[0] +
            a->data[1] * a->data[1] +
            a->data[2] * a->data[2]);

   if (norm < 1E-6f) {
     q->r = 0.0f;
     q->i = 0.0f;
     q->j = 0.0f;
     q->k = 0.0f;
     return 0;
   }

   ZSL_VECTOR_DEF(an, 3);
   zsl_vec_copy(&an, a);
   zsl_vec_scalar_div(&an, norm);

   q->r = ZSL_COS(*b / 2.0f);
   q->i = an.data[0] * ZSL_SIN(*b / 2.0f);
   q->j = an.data[1] * ZSL_SIN(*b / 2.0f);
   q->k = an.data[2] * ZSL_SIN(*b / 2.0f);

 #if CONFIG_ZSL_BOUNDS_CHECKS
 err:
 #endif
   return rc;
 }

 int zsl_quat_print(struct zsl_quat *q)
 {
   printf("%.16f + %.16f i + %.16f j + %.16f k\n",
     (double)q->r, (double)q->i, (double)q->j, (double)q->k);

   return 0;
 }