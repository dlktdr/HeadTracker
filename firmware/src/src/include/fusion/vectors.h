/*
 * Copyright (c) 2019-2020 Kevin Townsend (KTOWN)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup VECTORS Vectors
 *
 * @brief Vectors functions
 *
 * TODO: Expand with examples, better high-level documentation, etc.
 */

/**
 * @file
 * @brief API header file for vectors in zscilib.
 *
 * This file contains the zscilib vector APIs
 */

 #ifndef ZEPHYR_INCLUDE_ZSL_VECTORS_H_
 #define ZEPHYR_INCLUDE_ZSL_VECTORS_H_

 #include "zsl.h"

 #ifdef __cplusplus
 extern "C" {
 #endif

 /**
  * @addtogroup VEC_STRUCTS Structs, Enums and Macros
  *
  * @brief Various struct, enums and macros related to vectors.
  *
  * @ingroup VECTORS
  *  @{ */

 /** @brief Represents a vector. */
 struct zsl_vec {
   /** The number of elements in the vector. */
   size_t sz;
   /** The array of real number values assigned to the vector. */
   zsl_real_t *data;
 };

 /** Macro to declare a vector of size `n`.
  *
  * Be sure to also call 'zsl_vec_init' on the vector after this macro, since
  * matrices declared on the stack may have non-zero values by default!
  */
 #define ZSL_VECTOR_DEF(name, n)	     \
   zsl_real_t name ## _vec[n];  \
   struct zsl_vec name = {	     \
     .sz = n,	     \
     .data = name ## _vec \
   }

 /** @} */ /* End of VEC_STRUCTS group */

 /**
  * @addtogroup VEC_INIT Initialisation
  *
  * @brief Vector initisialisation.
  *
  * @ingroup VECTORS
  *  @{ */

 /**
  * Initialises vector 'v' with 0.0 values.
  *
  * @param v         Pointer to the zsl_vec to initialise/clear.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_init(struct zsl_vec *v);

 /**
  * @brief Converts an array of values into a vector. The number of elements in
  *        array 'a' must match the number of elements in vector 'v'.
  *        As such, 'v' should be previously initialised with an appropriate
  *        value assigned to v.sz.
  *
  * @param v The vector that the contents of array 'a' should be assigned to.
  *          The v.sz value must match the number of elements in 'a', meaning
  *          that the vector should be initialised before being passed in to
  *          this function.
  * @param a Pointer to the array containing the values to assign to 'v'.
  *          The array will be read v.sz elements deep.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_from_arr(struct zsl_vec *v, zsl_real_t *a);

 /**
  * @brief Copies the contents of vector 'vsrc' into vector 'vdest'.
  *
  * @param vdest Pointer to the destination vector data will be copied to.
  * @param vsrc  Pointer to the source vector data will be copied from.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_copy(struct zsl_vec *vdest, struct zsl_vec *vsrc);

 /** @} */ /* End of VEC_INIT group */

 /**
  * @addtogroup VEC_SELECTION Data Selection
  *
  * @brief Functions used to access subsets of the vector.
  *
  * @ingroup VECTORS
  *  @{ */

 /**
  * @brief Returns a subset of source vector 'v' in 'vsub'..
  *
  * @param v         The parent vector to read a subset of.
  * @param offset    The starting index (zero-based) foor the data subset.
  * @param len       The number of values to read, starting at offset.
  * @param vsub      The subset vector, which must have a buffer of at least
  *                  size 'len'. If the data buffer isn't sufficiently large,
  *                  this function will return -EINVAL.
  *
  * @return 0 on success, -EINVAL on a size of index error.
  */
 int zsl_vec_get_subset(struct zsl_vec *v, size_t offset, size_t len,
            struct zsl_vec *vsub);

 /** @} */ /* End of VEC_SELECTION group */

 /**
  * @addtogroup VEC_BASICMATH Basic Math
  *
  * @brief Basic mathematical operations (add, sum, norm, etc.).
  *
  * @ingroup VECTORS
  *  @{ */

 /**
  * @brief Adds corresponding vector elements in 'v' and 'w', saving to 'x'.
  *
  * @param v The first vector.
  * @param w The second vector.
  * @param x The output vector.
  *
  * @return 0 on success, -EINVAL if v and w are not equal length.
  */
 int zsl_vec_add(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *x);

 /**
  * @brief Subtracts corresponding vector elements in 'v' and 'w', saving to 'x'.
  *
  * @param v The first vector.
  * @param w The second vector.
  * @param x The output vector.
  *
  * @return 0 on success, -EINVAL if v and w are not equal length.
  */
 int zsl_vec_sub(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *x);

 /**
  * @brief Negates the elements in vector 'v'.
  *
  * @param v The vector to negate.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_neg(struct zsl_vec *v);

 /**
  * @brief Component-wise sum of a set of equal-length vectors.
  *
  * @param v  Pointer to the array of vectors.
  * @param n  The number of vectors in 'v'.
  * @param w  Pointer to the output vector containing the component-wise sum.
  *
  * @return 0 on success, -EINVAL if vectors in 'v' are no equal length, or
  *         -EINVAL if 'n' = 0.
  */
 int zsl_vec_sum(struct zsl_vec **v, size_t n, struct zsl_vec *w);

 /**
  * @brief Adds a scalar to each element in a vector.
  *
  * @param v The vector to scale.
  * @param s The scalar to add to each element.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_scalar_add(struct zsl_vec *v, zsl_real_t s);

 /**
  * @brief Multiply a vector by a scalar.
  *
  * @param v The vector to scale.
  * @param s The scalar to multiply by.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_scalar_mult(struct zsl_vec *v, zsl_real_t s);

 /**
  * @brief Divide a vector by a scalar.
  *
  * @param v The vector to scale.
  * @param s The scalar to divide all elements by.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_scalar_div(struct zsl_vec *v, zsl_real_t s);

 /**
  * @brief Calculates the distance between two vectors, which is equal to the
  *        norm of vector v - vector w.
  *
  * @param v The first vector.
  * @param w The second vector.
  *
  * @return The norm of vector v - vector w, or NAN is there was a
  *         size mismatch between vectors v and w.
  */
 zsl_real_t zsl_vec_dist(struct zsl_vec *v, struct zsl_vec *w);

 /**
  * @brief Computes the dot (aka scalar) product of two equal-length vectors
  *        (the sum of their component-wise products). The dot product is an
  *        indicator of the "agreement" between two vectors, or can be used
  *        to determine how far vector w deviates from vector v.
  *
  * @param v The first vector.
  * @param w The second vector.
  * @param d The dot product.
  *
  * @return 0 on success, or -EINVAL if vectors v and w aren't equal-length.
  */
 int zsl_vec_dot(struct zsl_vec *v, struct zsl_vec *w, zsl_real_t *d);

 /**
  * @brief Calculates the norm or absolute value of vector 'v' (the
  *        square root of the vector's dot product).
  *
  * @param v The vector to calculate the norm of.
  *
  * @return The norm of vector 'v'.
  */
 zsl_real_t zsl_vec_norm(struct zsl_vec *v);

 /**
  * @brief   Calculates the projection of vector 'u' over vector 'v', placing
  *          the results in vector 'w'.
  *
  * @param u Pointer to the first input vector.
  * @param v Pointer to the second input vector.
  * @param w Pointer to the output vector where the proj. results are stored.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_vec_project(struct zsl_vec *u, struct zsl_vec *v, struct zsl_vec *w);

 /**
  * @brief Converts (normalises) vector 'v' to a unit vector (a vector of
  *        magnitude or length 1). This is accomploished by dividing each
  *        element by the it's norm.
  *
  * @note Unit vectors are important since they have the ability to provide
  *       'directional' information, without requiring a specific magnitude.
  *
  * @param v The vector to convert to a unit vector.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_vec_to_unit(struct zsl_vec *v);

 /**
  * @brief Computes the cross (or vector) product of two 3-vectors.
  *
  * @param v The first 3-vector.
  * @param w The second 3-vector.
  * @param c The cross product 3-vector output.
  *
  * @return 0 on success, or -EINVAL if a non 3-vector is provided.
  *
  * Given two linearly independent 3-vectors (v and w), the cross product,
  * (v X w), is a vector that is perpendicular to both v and w and thus 'normal'
  * to the plane containing them.
  *
  * If two vectors have the same direction (or have the exact opposite direction
  * from one another, i.e. are not linearly independent) or if either one has
  * zero length, then their cross product is zero.
  *
  * The norm of the cross product equals the area of a parallelogram
  * with vectors v and w for sides.
  *
  * For a discusson of geometric and algebraic applications, see:
  * https://en.wikipedia.org/wiki/Cross_product
  */
 int zsl_vec_cross(struct zsl_vec *v, struct zsl_vec *w, struct zsl_vec *c);

 /**
  * @brief Computes the vector's sum of squares.
  *
  * @param v The vector to use.
  *
  * @return The sum of the squares of vector 'v'.
  */
 zsl_real_t zsl_vec_sum_of_sqrs(struct zsl_vec *v);

 /**
  * @brief Computes the component-wise mean of a set of identically-sized
  * vectors.
  *
  * @param v  Pointer to the array of vectors.
  * @param n  The number of vectors in 'v'.
  * @param m  Pointer to the output vector whose i'th element is the mean of
  *           the i'th elements of the input vectors.
  *
  * @return 0 on success, and -EINVAL if all vectors aren't identically sized.
  */
 int zsl_vec_mean(struct zsl_vec **v, size_t n, struct zsl_vec *m);

 /**
  * @brief Computes the arithmetic mean of a vector.
  *
  * @param v  The vector to use.
  * @param m  The arithmetic mean of vector v.
  *
  * @return 0 on success, otherwise an appropriate error code.
  */
 int zsl_vec_ar_mean(struct zsl_vec *v, zsl_real_t *m);

 /**
  * @brief Reverses the order of the entries in vector 'v'.
  *
  * @param v The vector to use.
  *
  * @return 0 on success, otherwise an appropriate error code.
  */
 int zsl_vec_rev(struct zsl_vec *v);

 /**
  * @brief Rearranges the input vector to place any zero-values at the end.
  *
  * @param v   The input vector to rearrange.
  *
  * @warning   This function is destructive to 'v'!
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_vec_zte(struct zsl_vec *v);

 /** @} */ /* End of VEC_BASICMATH group */

 /**
  * @addtogroup VEC_COMPARE Comparison
  *
  * @brief Functions used to compare or verify vectors.
  *
  * @ingroup VECTORS
  *  @{ */

 /**
  * @brief Checks if two vectors are identical in size and content.
  *
  * @param v     The first vector.
  * @param w     The second vector.
  * @param eps   The margin for floating-point equality (ex. '1E-5').
  *
  * @return true if the two vectors have the same size and the same values,
  *         otherwise false.
  */
 bool zsl_vec_is_equal(struct zsl_vec *v, struct zsl_vec *w, zsl_real_t eps);

 /**
  * @brief Checks if all elements in vector v are >= zero.
  *
  * @param v The vector to check.
  *
  * @return true if all elements in 'v' are zero or positive, otherwise false.
  */
 bool zsl_vec_is_nonneg(struct zsl_vec *v);

 /**
  * @brief Checks if vector v contains val, returning the number of occurences.
  *
  * @param v     The vector to check.
  * @param val   The value to check all coefficients for.
  * @param eps   The margin for floating-point equality (ex. '1E-5').
  *
  * @return The number of occurences of val withing range eps, 0 if no
  *         matching occurences were found, or a negative error code.
  */
 int zsl_vec_contains(struct zsl_vec *v, zsl_real_t val, zsl_real_t eps);

 /**
  * @brief Sorts the values in vector v from smallest to largest using quicksort,
  *        and assigns the sorted output to vector w.
  *
  * @param v     The unsorted, input vector.
  * @param w     The sorted, output vector.
  *
  * @return 0 if everything executed properly, otherwise a negative error code.
  */
 int zsl_vec_sort(struct zsl_vec *v, struct zsl_vec *w);

 /** @} */ /* End of VEC_COMPARE group */

 /**
  * @addtogroup VEC_DISPLAY Display
  *
  * @brief Functions used to present vectors in a user-friendly format.
  *
  * @ingroup VECTORS
  *  @{ */

 /**
  * @brief Print the supplied vector using printf in a human-readable manner.
  *
  * @param v     Pointer to the vector to print.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_vec_print(struct zsl_vec *v);

 // int      zsl_vec_fprint(FILE *stream, zsl_vec *v);

 /** @} */ /* End of VEC_DISPLAY group */

 #ifdef __cplusplus
 }
 #endif

 #endif /* ZEPHYR_INCLUDE_ZSL_VECTORS_H_ */

 /** @} */ /* End of vectors group */