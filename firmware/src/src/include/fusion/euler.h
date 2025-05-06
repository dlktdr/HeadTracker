/*
 * Copyright (c) 2019-2021 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup EULER Euler Angles
 *
 * @brief Functions and structs for dealing with Euler (specifically,
 *        Cardanian) angles.
 *
 * @ingroup ORIENTATION
 *  @{ */

/**
 * @file
 * @brief API header file for euler angles in zscilib.
 *
 * This file contains the zscilib euler angle APIs
 */

 #ifndef ZEPHYR_INCLUDE_ZSL_EULER_H_
 #define ZEPHYR_INCLUDE_ZSL_EULER_H_

 #include "vectors.h"
 #include "matrices.h"

 #ifdef __cplusplus
 extern "C" {
 #endif

 /**
  * @brief An ordered triplet of real numbers describing the orientation of a
  *        rigid body in three-dimensional (Euclidean) space, with respect to a
  *        fixed coordinate system. Each element represents the rotation about
  *        the specified axis, expressed in radians.
  *
  * @note Technically a Cardanian (AKA Tait–Bryan) angle, being limited to a
  *       single instance each of X, Y and Z.
  */
 struct zsl_euler {
   union {
     struct {
       zsl_real_t x;   /**< @brief X axis, in radians. */
       zsl_real_t y;   /**< @brief Y axis, in radians. */
       zsl_real_t z;   /**< @brief Z axis, in radians. */
     };
     /** @brief Array-based access. */
     zsl_real_t idx[3];
   };
 };

 /**
  * @brief Takes the values in the supplied @ref zsl_euler, and assigns the
  *        same memory address to a @ref zsl_vec, allowing for vector functions
  *        to be used on the zsl_euler XYZ values.
  *
  * @param e     Pointer to the zsl_euler struct to use.
  * @param v     Pointer to the zsl_vec struct to use.
  *
  * @return 0 if everything executed correctly, otherwise a negative error code.
  */
 int zsl_eul_to_vec(struct zsl_euler *e, struct zsl_vec *v);

 /**
  * @brief Print the supplied euler angles vector using printf in a
  *        human-readable manner.
  *
  * @param e     Pointer to the vector containing the euler angles to print.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_eul_print(struct zsl_euler *e);

 #ifdef __cplusplus
 }
 #endif

 #endif /* ZEPHYR_INCLUDE_ZSL_EULER_H_ */

 /** @} */ /* End of euler group */