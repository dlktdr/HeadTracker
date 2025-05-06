/*
 * Copyright (c) 2021 Marti Riba Pons
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup FUSION_ALG_AQUA AQUA
 *
 * @brief AQUA sensor fuion algorithm.
 *
 * @ingroup FUSION_ALGORITHMS
 *  @{
 */

/**
 * @file
 * @brief AQUA sensor fusion algorithm.
 *
 * This file implements the AQUA sensor fusion algorithm.
 */

 #ifndef ZEPHYR_INCLUDE_ZSL_FUSION_AQUA_H_
 #define ZEPHYR_INCLUDE_ZSL_FUSION_AQUA_H_

 #include "vectors.h"
 #include "matrices.h"
 #include "quaternions.h"

 #ifdef __cplusplus
 extern "C" {
 #endif

 /* Source:
  * https://res.mdpi.com/sensors/sensors-15-19302/article_deploy/sensors-15-19302.pdf
  */

 /**
  * @brief Config settings for the AQUA sensor fusion algorithm.
  */
 struct zsl_fus_aqua_cfg {
   /**
    * @brief A value between 0.0 and 1.0 which sets the gain for the
    *        cut-off frequency of the filter. Adjust this value to control
    *        LERP or SLERP, which reduces the amount of high frequency
    *        noise in the accelerometer. Alpha must be calculated in
    *        static conditions.
    */
   zsl_real_t alpha;

   /**
    * @brief Similar to alpha, this variable is set between 0.0 and 1.0
    *        and is used to control the LERP or SLERP, which reduces the
    *        amount of high frequency noise in the magnetometer. Beta
    *        must be calculated in static conditions.
    */
   zsl_real_t beta;

   /**
    * @brief Threshold value to determine whether to compute LERP or
    *        SLERP to reduce the amount of high frequency noise of the
    *        accelerometer in the Dq_acc vector. Typically set to 0.9.
    */
   zsl_real_t e_a;

   /**
    * @brief Threshold value to determine whether to compute LERP or
    *        SLERP to reduce the amount of high frequency noise of the
    *        magnetometer in the Dq_mag vector. Typically set to 0.9.
    */
   zsl_real_t e_m;
 };

 /**
  * @brief Sets the sample frequency (in Hz) for the algorithm.
  *
  * @param freq  Sampling frequency in Hz.
  * @param cfg   Config struct for this filter.
  *
  * @return int  0 if everything executed correctly, otherwise an appropriate
  *              negative error code.
  */
 int zsl_fus_aqua_init(uint32_t freq, void *cfg);

 /**
  * @brief Algebraic Quaterion Algorithm (AQUA) sensor fusion algorithm
  *        implementation.
  *
  * Source:
  * https://res.mdpi.com/sensors/sensors-15-19302/article_deploy/sensors-15-19302.pdf
  *
  * @param a     Input accelerometer vector (3 samples required). NULL if none.
  * @param m     Input magnetometer vector (3 samples required). NULL if none.
  * @param g     Input gyroscope vector (3 samples required). NULL if none.
  * @param incl  Input magnetic inclination, in degrees. NULL for none.
  * @param q     Pointer to the output @ref zsl_quat.
  * @param cfg   Pointer to the config struct for this algorithm.
  *
  * @return int  0 if everything executed correctly, otherwise an appropriate
  *              negative error code.
  */
 int zsl_fus_aqua_feed(struct zsl_vec *a, struct zsl_vec *m,
           struct zsl_vec *g, zsl_real_t *incl, struct zsl_quat *q,
           void *cfg);

 /**
  * @brief Default error handler for the AQUA sensor fusion driver.
  *
  * @note Normally you will want to implement your own version of this function.
  *
  * @param error     The error code genereated.
  */
 void zsl_fus_aqua_error(int error);

 #ifdef __cplusplus
 }
 #endif

 #endif /* ZEPHYR_INCLUDE_ZSL_FUSION_AQUA_H_ */

 /** @} */ /* End of algorithms group */