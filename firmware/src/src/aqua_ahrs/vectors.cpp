/*
 * Copyright (c) 2019 Kevin Townsend (KTOWN)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <errno.h>
 #include <stdio.h>
 #include <stdbool.h>
 #include <string.h>
 #include "vectors.h"
 #include "zsl.h"

 /* Enable optimised ARM Thumb/Thumb2 functions if available. */
 #if (CONFIG_ZSL_PLATFORM_OPT == 1 || CONFIG_ZSL_PLATFORM_OPT == 2)
 #include <zsl/asm/arm/asm_arm_vectors.h>
 #endif

 int zsl_vec_init(struct zsl_vec *v)
 {
   memset(v->data, 0, v->sz * sizeof(zsl_real_t));

   return 0;
 }

 int zsl_vec_from_arr(struct zsl_vec *v, zsl_real_t *a)
 {
   memcpy(v->data, a, v->sz * sizeof(zsl_real_t));

   return 0;
 }

 int zsl_vec_copy(struct zsl_vec *vdest, struct zsl_vec *vsrc)
 {
   vdest->sz = vsrc->sz;
   memcpy(vdest->data, vsrc->data, sizeof(zsl_real_t) *
          vdest->sz);

   return 0;
 }

 int zsl_vec_get_subset(struct zsl_vec *v, size_t offset, size_t len,
            struct zsl_vec *vsub)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure offset doesn't exceed v->sz. */
   if (offset >= v->sz) {
     return -EINVAL;
   }
   /* If offset+len exceeds v->sz, truncate len. */
   if (offset + len >= v->sz) {
     len = v->sz - offset;
   }
   /* Make sure vsub->sz is at least len, otherwise buffer overrun. */
   if (vsub->sz < len) {
     return -EINVAL;
   }
 #endif

   /* Truncate vsub->sz if there is an underrun. */
   if (vsub->sz > len) {
     vsub->sz = len;
   }

   memcpy(vsub->data, &v->data[offset], len * sizeof(zsl_real_t));

   return 0;
 }

 #if !asm_vec_add
 int zsl_vec_add(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *x)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure v and w are equal length. */
   if ((v->sz != w->sz) || (v->sz != x->sz)) {
     return -EINVAL;
   }
 #endif

   for (size_t i = 0; i < v->sz; i++) {
     x->data[i] = v->data[i] + w->data[i];
   }

   return 0;
 }
 #endif

 int zsl_vec_sub(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *x)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure v and w are equal length. */
   if ((v->sz != w->sz) || (v->sz != x->sz)) {
     return -EINVAL;
   }
 #endif

   for (size_t i = 0; i < v->sz; i++) {
     x->data[i] = v->data[i] - w->data[i];
   }

   return 0;
 }

 int zsl_vec_neg(struct zsl_vec *v)
 {
   for (size_t i = 0; i < v->sz; i++) {
     v->data[i] = -v->data[i];
   }

   return 0;
 }

 int zsl_vec_sum(struct zsl_vec **v, size_t n, struct zsl_vec *w)
 {
   size_t sz_last;

   if (!n) {
     return -EINVAL;
   }

   sz_last = v[0]->sz;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure all vectors have the same size. */
   for (size_t i = 0; i < n; i++) {
     if (sz_last != v[i]->sz) {
       return -EINVAL;
     }
   }
 #endif

   /* Sum all vectors. */
   w->sz = sz_last;
   for (size_t i = 0; i < n; i++) {
     for (size_t j = 0; j < w->sz; j++) {
       w->data[j] += v[i]->data[j];
     }
   }

   return 0;
 }

 #if !asm_vec_scalar_add
 int zsl_vec_scalar_add(struct zsl_vec *v, zsl_real_t s)
 {
   for (size_t i = 0; i < v->sz; i++) {
     v->data[i] += s;
   }

   return 0;
 }
 #endif

 #if !asm_vec_scalar_mult
 int zsl_vec_scalar_mult(struct zsl_vec *v, zsl_real_t s)
 {
   for (size_t i = 0; i < v->sz; i++) {
     v->data[i] *= s;
   }

   return 0;
 }
 #endif

 #if !asm_vec_scalar_div
 int zsl_vec_scalar_div(struct zsl_vec *v, zsl_real_t s)
 {
   /* Avoid divide by zero errors. */
   if (s == 0) {
     return -EINVAL;
   }

   for (size_t i = 0; i < v->sz; i++) {
     v->data[i] /= s;
   }

   return 0;
 }
 #endif

 zsl_real_t zsl_vec_dist(struct zsl_vec *v, struct zsl_vec *w)
 {
   int rc = 0;

   zsl_real_t xdat[v->sz];

   struct zsl_vec x = { .sz = v->sz, .data = xdat };

   memset(xdat, 0, x.sz * sizeof(zsl_real_t));

   rc = zsl_vec_sub(v, w, &x);
   if (rc) {
     return NAN;
   }

   return zsl_vec_norm(&x);
 }

 int zsl_vec_dot(struct zsl_vec *v, struct zsl_vec *w, zsl_real_t *d)
 {
   zsl_real_t res = 0.0f;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure v and w are equal length. */
   if (v->sz != w->sz) {
     return -EINVAL;
   }
 #endif

   for (size_t i = 0; i < v->sz; i++) {
     res += v->data[i] * w->data[i];
   }

   *d = res;

   return 0;
 }

 zsl_real_t zsl_vec_norm(struct zsl_vec *v)
 {
   /*
    * |v| = sqrt( v[0]^2 + v[1]^2 + V[...]^2 )
    */
   if (v == NULL) {
     return 0;
   }
   return ZSL_SQRT(zsl_vec_sum_of_sqrs(v));
 }

 int zsl_vec_project(struct zsl_vec *u, struct zsl_vec *v, struct zsl_vec *w)
 {
   zsl_real_t p;
   zsl_real_t t;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure v, w and u are equal length. */
   if ((v->sz != w->sz) || (v->sz != u->sz)) {
     return -EINVAL;
   }
 #endif

   zsl_vec_copy(w, u);
   zsl_vec_dot(v, u, &p);
   zsl_vec_dot(u, u, &t);
   zsl_vec_scalar_mult(w, p / t);

   return 0;
 }

 int zsl_vec_to_unit(struct zsl_vec *v)
 {
   zsl_real_t norm = zsl_vec_norm(v);

   /*
    *            v
    * unit(v) = ---
    *           |v|
    */

   /* Avoid divide by zero errors. */
   if (norm != 0.0f) {
     zsl_vec_scalar_mult(v, 1.0f / norm);
   } else {
     /* TODO: What is the best approach here? */
     /* On div by zero clear vector and return v[0] = 1.0f. */
     zsl_vec_init(v);
     v->data[0] = 1.0f;
   }

   return 0;
 }

 int zsl_vec_cross(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *c)
 {
 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure this is a 3-vector. */
   if ((v->sz != 3) || (w->sz != 3) || (c->sz != 3)) {
     return -EINVAL;
   }
 #endif

   /*
    * Given:
    *
    *       |Cx|      |Vx|      |Wx|
    *   C = |Cy|, V = |Vy|, W = |Wy|
    *       |Cz|      |Vz|      |Wz|
    *
    * The cross product can be represented as:
    *
    *   Cx = VyWz - VzWy
    *   Cy = VzWx - VxWz
    *   Cz = VxWy - VyWx
    */

   c->data[0] = v->data[1] * w->data[2] - v->data[2] * w->data[1];
   c->data[1] = v->data[2] * w->data[0] - v->data[0] * w->data[2];
   c->data[2] = v->data[0] * w->data[1] - v->data[1] * w->data[0];

   return 0;
 }

 zsl_real_t zsl_vec_sum_of_sqrs(struct zsl_vec *v)
 {
   zsl_real_t dot = 0.0f;

   zsl_vec_dot(v, v, &dot);

   return dot;
 }

 int zsl_vec_mean(struct zsl_vec **v, size_t n, struct zsl_vec *m)
 {
   int rc;

 #if CONFIG_ZSL_BOUNDS_CHECKS
   /* Make sure the mean vector has an approproate size. */
   if (m->sz != v[0]->sz) {
     return -EINVAL;
   }
 #endif

   /* sum also checks that all vectors have the same length. */
   rc = zsl_vec_sum(v, n, m);
   if (rc) {
     return rc;
   }

   rc = zsl_vec_scalar_mult(m, 1.0f / (zsl_real_t) n);

   return 0;
 }

 int zsl_vec_ar_mean(struct zsl_vec *v, zsl_real_t *m)
 {
   /* Avoid divide by zero errors. */
   if (v->sz < 1) {
     return -EINVAL;
   }

   *m = 0.0f;
   for (size_t i = 0; i < v->sz; i++) {
     *m += v->data[i];
   }

   *m /= v->sz;

   return 0;
 }

 int zsl_vec_rev(struct zsl_vec *v)
 {
   zsl_real_t t;
   size_t start = 0;
   size_t end = v->sz - 1;

   while (start < end) {
     t = v->data[start];
     v->data[start] = v->data[end];
     v->data[end] = t;
     start++;
     end--;
   }

   return 0;
 }

 int zsl_vec_zte(struct zsl_vec *v)
 {
   size_t x = 0;
   zsl_real_t epsilon = 1E-6f;

   for (size_t g = 0; g < v->sz; g++) {
     if ((v->data[g - x] >= 0.0f && v->data[g - x] < epsilon) ||
         (v->data[g - x] <= 0.0f && v->data[g - x] > epsilon)) {
       for (size_t p = g - x; p < (v->sz - 1); p++) {
         v->data[p] = v->data[p + 1];
       }
       v->data[v->sz - 1] = 0.0f;
       x++;
     }
   }

   return 0;
 }

 bool zsl_vec_is_equal(struct zsl_vec *v, struct zsl_vec *w, zsl_real_t eps)
 {
   zsl_real_t c;

   if (v->sz != w->sz) {
     return false;
   }

   for (size_t i = 0; i < v->sz; i++) {
     c = v->data[i] - w->data[i];
     if (c >= eps || -c >= eps) {
       return false;
     }
   }

   return true;
 }

 bool zsl_vec_is_nonneg(struct zsl_vec *v)
 {
   for (size_t i = 0; i < v->sz; i++) {
     if (v->data[i] < 0.0f) {
       return false;
     }
   }

   return true;
 }

 int zsl_vec_contains(struct zsl_vec *v, zsl_real_t val, zsl_real_t eps)
 {
   zsl_real_t c;
   int count = 0;

   for (size_t i = 0; i < v->sz; i++) {
     c = v->data[i] - val;
     if (c < eps && -c < eps) {
       count++;
     }
   }

   return count;
 }

 /**
  * @brief Quicksort implementation used by zsl_vec_sort.
  *
  * @param v    	Unsorted input vector.
  * @param low 	Low index value.
  * @param high  High index value.
  *
  * @return 0 if everything executed properly, otherwise a negative error code.
  */
 static int zsl_vec_quicksort(struct zsl_vec *v, size_t low, size_t high)
 {
   size_t i, j, p;
   zsl_real_t t;

   if (low < high) {
     p = low;
     i = low;
     j = high;

     while (i < j) {
       while (v->data[i] <= v->data[p] && i <= high) {
         i++;
       }
       while (v->data[j] > v->data[p] && j >= low) {
         j--;
       }
       if (i < j) {
         t = v->data[i];
         v->data[i] = v->data[j];
         v->data[j] = t;
       }
     }

     t = v->data[j];
     v->data[j] = v->data[p];
     v->data[p] = t;

     zsl_vec_quicksort(v, low, i - 1);
     zsl_vec_quicksort(v, j + 1, high);
   }

   return 0;
 }

 int zsl_vec_sort(struct zsl_vec *v, struct zsl_vec *w)
 {
   /* Set counters to zero. */
   size_t count = 0;
   size_t count2 = 0;
   size_t i, j, k;

   ZSL_VECTOR_DEF(u, v->sz);
   zsl_vec_init(&u);

   /* Copy the vector 'v' into the vector 'u' with no repeated values. */
   for (j = 0; j < v->sz; j++) {
     if (v->data[j] >= 1E-5f || v->data[j] <= 1E-5f) {
       if (zsl_vec_contains(&u, v->data[j], 1E-5f) == 0) {
         u.data[count] = v->data[j];
         count++;
       }
     }
   }

   if (zsl_vec_contains(v, 0.0f, 1E-5f) > 0) {
     count++;
   }

   u.sz = count;

   /* Sort the vector with no repeated values 'u'. */
   zsl_vec_quicksort(&u, 0, count - 1);

   /* Add back the repeated values in the correct order into the vector 'w'. */
   for (i = 0; i < count; i++) {
     for (k = 0; k < static_cast<size_t>(zsl_vec_contains(v, u.data[i], 1E-5f)); k++) {
       w->data[count2] = u.data[i];
       count2++;
     }
   }

   return 0;
 }

 int zsl_vec_print(struct zsl_vec *v)
 {
   for (size_t g = 0; g < v->sz; g++) {
     printf("%f ", (double)v->data[g]);
   }
   printf("\n\n");

   return 0;
 }