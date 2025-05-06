/*
 * Copyright (c) 2019-2020 Kevin Townsend (KTOWN)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup MATRICES Matrices
 *
 * @brief Various mxn matrices functions.
 *
 * TODO: Expand with examples, better high-level documentation, etc.
 */

/**
 * @file
 * @brief API header file for matrices in zscilib.
 *
 * This file contains the zscilib matrix APIs
 */

 #ifndef ZEPHYR_INCLUDE_ZSL_MATRICES_H_
 #define ZEPHYR_INCLUDE_ZSL_MATRICES_H_

 #include "vectors.h"

 #ifdef __cplusplus
 extern "C" {
 #endif

 /**
  * @addtogroup MTX_STRUCTS Structs and Macros
  *
  * @brief Common structs and macros for working with matrices.
  *
  * @ingroup MATRICES
  *  @{ */

 /** Error: The number of eigenvectors is less than the matrice's dimension. */
 #define EEIGENSIZE   (100)
 /** Error: Occurs when the input matrix has complex eigenvalues. */
 #define ECOMPLEXVAL  (101)

 /** @brief Represents a m x n matrix, with data stored in row-major order. */
 struct zsl_mtx {
   /** The number of rows in the matrix (typically denoted as 'm'). */
   size_t sz_rows;
   /** The number of columns in the matrix (typically denoted as 'n'). */
   size_t sz_cols;
   /** Data assigned to the matrix, in row-major order (left to right). */
   zsl_real_t *data;
 };

 /**
  * Macro to declare a matrix of shape m*n.
  *
  * Be sure to also call 'zsl_mtx_init' on the matrix after this macro, since
  * matrices declared on the stack may have non-zero values by default!
  */
 #define ZSL_MATRIX_DEF(name, m, n);	\
   zsl_real_t name ## _mtx[m * n];	\
   struct zsl_mtx name = {		\
     .sz_rows = m,		\
     .sz_cols = n,		\
     .data = name ## _mtx	\
   }

 /** @} */ /* End of MTX_STRUCTS group */

 /**
  * @addtogroup MTX_OPERANDS Operands
  *
  * @brief Unary and binary operands that can be performed on matrix
  *        coefficients.
  *
  * @ingroup MATRICES
  *  @{ */

 /** @brief Component-wise unary operations. */
 typedef enum zsl_mtx_unary_op {
   ZSL_MTX_UNARY_OP_INCREMENT,             /**< ++ */
   ZSL_MTX_UNARY_OP_DECREMENT,             /**< -- */
   ZSL_MTX_UNARY_OP_NEGATIVE,
   ZSL_MTX_UNARY_OP_ROUND,
   ZSL_MTX_UNARY_OP_ABS,
   ZSL_MTX_UNARY_OP_FLOOR,
   ZSL_MTX_UNARY_OP_CEIL,
   ZSL_MTX_UNARY_OP_EXP,
   ZSL_MTX_UNARY_OP_LOG,
   ZSL_MTX_UNARY_OP_LOG10,
   ZSL_MTX_UNARY_OP_SQRT,
   ZSL_MTX_UNARY_OP_SIN,
   ZSL_MTX_UNARY_OP_COS,
   ZSL_MTX_UNARY_OP_TAN,
   ZSL_MTX_UNARY_OP_ASIN,
   ZSL_MTX_UNARY_OP_ACOS,
   ZSL_MTX_UNARY_OP_ATAN,
   ZSL_MTX_UNARY_OP_SINH,
   ZSL_MTX_UNARY_OP_COSH,
   ZSL_MTX_UNARY_OP_TANH,
 } zsl_mtx_unary_op_t;

 /** @brief Component-wise binary operations. */
 typedef enum zsl_mtx_binary_op {
   ZSL_MTX_BINARY_OP_ADD,                  /**< a + b */
   ZSL_MTX_BINARY_OP_SUB,                  /**< a - b */
   ZSL_MTX_BINARY_OP_MULT,                 /**< a * b */
   ZSL_MTX_BINARY_OP_DIV,                  /**< a / b */
   ZSL_MTX_BINARY_OP_MEAN,                 /**< mean(a, b) */
   ZSL_MTX_BINARY_OP_EXPON,                /**< a ^ b */
   ZSL_MTX_BINARY_OP_MIN,                  /**< min(a, b) */
   ZSL_MTX_BINARY_OP_MAX,                  /**< max(a, b) */
   ZSL_MTX_BINARY_OP_EQUAL,                /**< a == b */
   ZSL_MTX_BINARY_OP_NEQUAL,               /**< a != b */
   ZSL_MTX_BINARY_OP_LESS,                 /**< a < b */
   ZSL_MTX_BINARY_OP_GREAT,                /**< a > b */
   ZSL_MTX_BINARY_OP_LEQ,                  /**< a <= b */
   ZSL_MTX_BINARY_OP_GEQ,                  /**< a >= b */
 } zsl_mtx_binary_op_t;

 /** @} */ /* End of MTX_OPERANDS group */

 /**
  * @addtogroup MTX_INIT Initialisation
  *
  * @brief Functions used to initialise matrices.
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Function prototype called when applying a set of component-wise unary
  * operations to a matrix via `zsl_mtx_unary_func`.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 typedef int (*zsl_mtx_unary_fn_t)(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief Function prototype called when applying a set of component-wise binary
  * operations using a pair of symmetrical matrices via `zsl_mtx_binary_func`.
  *
  * @param ma    Pointer to first zsl_mtx to use in the binary operation.
  * @param mb    Pointer to second zsl_mtx to use in the binary operation.
  * @param mc    Pointer to output zsl_mtx used to store results.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 typedef int (*zsl_mtx_binary_fn_t)(struct zsl_mtx *ma, struct zsl_mtx *mb,
            struct zsl_mtx *mc, size_t i, size_t j);

 /**
  * @brief Function prototype called when populating a matrix via `zsl_mtx_init`.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 typedef int (*zsl_mtx_init_entry_fn_t)(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief Assigns a zero-value to all entries in the matrix.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_entry_fn_empty(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief Sets the value to '1.0' if the entry is on the diagonal (row=col),
  * otherwise '0.0'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_entry_fn_identity(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief Sets the value to a random number between -1.0 and 1.0.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write (0-based).
  * @param j     The column number to write (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_entry_fn_random(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief Initialises matrix 'm' using the specified entry function to
  * assign values.
  *
  * @param m         Pointer to the zsl_mtx to use.
  * @param entry_fn  The zsl_mtx_init_entry_fn_t instance to call. If this
  *                  is set to NULL 'zsl_mtx_entry_fn_empty' will be called.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_init(struct zsl_mtx *m, zsl_mtx_init_entry_fn_t entry_fn);

 /**
  * @brief Converts an array of values into a matrix.
  *
  * The number of elements in array 'a' must match the number of elements in
  * matrix 'm' (m.sz_rows * m.sz_cols). As such, 'm' should be a previously
  * initialised matrix with appropriate values assigned to m.sz_rows and
  * m.sz_cols. Assumes array values are in row-major order.
  *
  * @param m The matrix that the contents of array 'a' should be assigned to.
  *          The m.sz_rows and m.sz_cols dimensions must match the number of
  *          elements in 'a', meaning that the matrix should be initialised
  *          before being passed in to this function.
  * @param a Pointer to the array containing the values to assign to 'm' in
  *          row-major order (left-to-right, top-to-bottom). The array will be
  *          read m.sz_rows * m.sz_cols elements deep.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_from_arr(struct zsl_mtx *m, zsl_real_t *a);

 /**
  * @brief Copies the contents of matrix 'msrc' into matrix 'mdest'.
  *
  * @param mdest Pointer to the destination matrix data will be copied to.
  * @param msrc  Pointer to the source matrix data will be copied from.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_copy(struct zsl_mtx *mdest, struct zsl_mtx *msrc);

 /** @} */ /* End of MTX_INIT group */

 /**
  * @addtogroup MTX_DATACCESS Data Access
  *
  * @brief Functions used to access or modify matrix rows, columns or
  *        coefficients.
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Gets a single value from the specified row (i) and column (j).
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to read (0-based).
  * @param j     The column number to read (0-based).
  * @param x     Pointer to where the value should be stored.
  *
  * @return  0 if everything executed correctly, or -EINVAL on an out of
  *          bounds error.
  */
 int zsl_mtx_get(struct zsl_mtx *m, size_t i, size_t j, zsl_real_t *x);

 /**
  * @brief Sets a single value at the specified row (i) and column (j).
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to update (0-based).
  * @param j     The column number to update (0-based).
  * @param x     The value to assign.
  *
  * @return  0 if everything executed correctly, or -EINVAL on an out of
  *          bounds error.
  */
 int zsl_mtx_set(struct zsl_mtx *m, size_t i, size_t j, zsl_real_t x);

 /**
  * @brief Gets the contents of row 'i' from matrix 'm', assigning the array
  *        of data to 'v'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to read (0-based).
  * @param v     Pointer to the array where the row vector should be written.
  *              Must be at least m->sz_cols elements long!
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_get_row(struct zsl_mtx *m, size_t i, zsl_real_t *v);

 /**
  * @brief Sets the contents of row 'i' in matrix 'm', assigning the values
  *        found in array 'v'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to write to (0-based).
  * @param v     Pointer to the array where the row vector data is stored.
  *              Must be at least m->sz_cols elements long!
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_set_row(struct zsl_mtx *m, size_t i, zsl_real_t *v);

 /**
  * @brief Gets the contents of column 'j' from matrix 'm', assigning the array
  *        of data to 'v'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param j     The column number to read (0-based).
  * @param v     Pointer to the array where the column vector should be written.
  *              Must be at least m->sz_rows elements long!
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_get_col(struct zsl_mtx *m, size_t j, zsl_real_t *v);

 /**
  * @brief Sets the contents of column 'j' in matrix 'm', assigning the values
  *        found in array 'v'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param j     The column number to write to (0-based).
  * @param v     Pointer to the array where the column vector data is stored.
  *              Must be at least m->sz_rows elements long!
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_set_col(struct zsl_mtx *m, size_t j, zsl_real_t *v);

 /** @} */ /* End of MTX_DATAACCESS group */

 /**
  * @ingroup MTX_OPERANDS
  *  @{ */

 /**
  * @brief Applies a unary operand on every coefficient in matrix 'm'.
  *
  * @param m         Pointer to the zsl_mtx to use.
  * @param op        The unary operation to apply to each coefficient.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_unary_op(struct zsl_mtx *m, zsl_mtx_unary_op_t op);

 /**
  * @brief Applies a unary function on every coefficient in matrix 'm', using the
  * specified 'zsl_mtx_apply_unary_fn_t' instance.
  *
  * @param m         Pointer to the zsl_mtx to use.
  * @param fn        The zsl_mtx_unary_fn_t instance to call.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_unary_func(struct zsl_mtx *m, zsl_mtx_unary_fn_t fn);

 /**
  * @brief Applies a component-wise binary operation on every coefficient in
  * symmetrical matrices 'ma' and 'mb', with the results being stored in the
  * identically shaped `mc` matrix.
  *
  * @param ma        Pointer to first zsl_mtx to use in the binary operation.
  * @param mb        Pointer to second zsl_mtx to use in the binary operation.
  * @param mc        Pointer to output zsl_mtx used to store results.
  * @param op        The binary operation to apply to each coefficient.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_binary_op(struct zsl_mtx *ma, struct zsl_mtx *mb,
           struct zsl_mtx *mc, zsl_mtx_binary_op_t op);

 /**
  * @brief Applies a component-wise binary operztion on every coefficient in
  * symmetrical matrices 'ma' and 'mb', with the results  being stored in the
  * identically shaped 'mc' matrix. The actual binary operation is executed
  * using the specified 'zsl_mtx_binary_fn_t' callback.
  *
  * @param ma        Pointer to first zsl_mtx to use in the binary operation.
  * @param mb        Pointer to second zsl_mtx to use in the binary operation.
  * @param mc        Pointer to output zsl_mtx used to store results.
  * @param fn        The zsl_mtx_binary_fn_t instance to call.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_binary_func(struct zsl_mtx *ma, struct zsl_mtx *mb,
       struct zsl_mtx *mc, zsl_mtx_binary_fn_t fn);

 /** @} */ /* End of MTX_OPERANDS group */

 /**
  * @addtogroup MTX_BASICMATH Basic Math
  *
  * @brief Basic mathematical operations for matrices (add, substract, etc.).
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Adds matrices 'ma' and 'mb', assigning the output to 'mc'.
  *        Matrices 'ma', 'mb' and 'mc' must all be identically shaped.
  *
  * @param ma    Pointer to the first input zsl_mtx.
  * @param mb    Pointer to the second input zsl_mtx.
  * @param mc    Pointer to the output zsl_mtx.
  *
  * @return  0 if everything executed correctly, or -EINVAL if the three
  *          matrices are not all identically shaped.
  */
 int zsl_mtx_add(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc);

 /**
  * @brief Adds matrices 'ma' and 'mb', assigning the output to 'ma'.
  *        Matrices 'ma', and 'mb' must be identically shaped.
  *
  * @param ma    Pointer to the first input zsl_mtx. This matrix will be
  *              overwritten with the results of the addition operations!
  * @param mb    Pointer to the second input zsl_mtx.
  *
  * @warning     This function is destructive to 'ma'!
  *
  * @return  0 if everything executed correctly, or -EINVAL if the two input
  *          matrices are not identically shaped.
  */
 int zsl_mtx_add_d(struct zsl_mtx *ma, struct zsl_mtx *mb);

 /**
  * @brief Adds the values of row 'j' to row 'i' in matrix 'm'. This operation
  *        is destructive for row 'i'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The first row number to add (0-based).
  * @param j     The second row number to add (0-based).
  *
  * @return  0 if everything executed correctly, or -EINVAL on an out of
  *          bounds error.
  */
 int zsl_mtx_sum_rows_d(struct zsl_mtx *m, size_t i, size_t j);

 /**
  * @brief This function takes the coefficients of row 'j' and multiplies them
  *        by scalar 's', then adds the resulting coefficient to the parallel
  *        element in row 'i'. Row 'i' will be modified in this operation.
  *
  * This function implements a key mechanism of Gauss–Jordan elimination, as can
  * be seen in @ref zsl_mtx_gauss_elim.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to update (0-based).
  * @param j     The row number to scale and then add to 'i' (0-based).
  * @param s     The scalar value to apply to the values in row 'j'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_sum_rows_scaled_d(struct zsl_mtx *m, size_t i, size_t j,
             zsl_real_t s);

 /**
  * @brief Subtracts matrices 'mb' from 'ma', assigning the output to 'mc'.
  *        Matrices 'ma', 'mb' and 'mc' must all be identically shaped.
  *
  * @param ma    Pointer to the first input zsl_mtx.
  * @param mb    Pointer to the second input zsl_mtx.
  * @param mc    Pointer to the output zsl_mtx.
  *
  * @return  0 if everything executed correctly, or -EINVAL if the three
  *          matrices are not all identically shaped.
  */
 int zsl_mtx_sub(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc);

 /**
  * @brief Subtracts matrix 'mb' from 'ma', assigning the output to 'ma'.
  *        Matrices 'ma', and 'mb' must be identically shaped.
  *
  * @param ma    Pointer to the first input zsl_mtx. This matrix will be
  *              overwritten with the results of the subtraction operations!
  * @param mb    Pointer to the second input zsl_mtx.
  *
  * @warning     This function is destructive to 'ma'!
  *
  * @return  0 if everything executed correctly, or -EINVAL if the two input
  *          matrices are not identically shaped.
  */
 int zsl_mtx_sub_d(struct zsl_mtx *ma, struct zsl_mtx *mb);

 /**
  * @brief Multiplies matrix 'ma' by 'mb', assigning the output to 'mc'.
  *        Matrices 'ma' and 'mb' must be compatibly shaped, meaning that
  *        'ma' must have the same numbers of columns as there are rows
  *        in 'mb'.
  *
  * @param ma    Pointer to the first input zsl_mtx.
  * @param mb    Pointer to the second input zsl_mtx.
  * @param mc    Pointer to the output zsl_mtx.
  *
  * @return  0 if everything executed correctly, or -EINVAL if the input
  *          matrices are not compatibly shaped.
  */
 int zsl_mtx_mult(struct zsl_mtx *ma, struct zsl_mtx *mb, struct zsl_mtx *mc);

 /**
  * @brief Multiplies matrix 'ma' by 'mb', assigning the output to 'ma'.
  *        Matrices 'ma' and 'mb' must be compatibly shaped, meaning that
  *        'ma' must have the same numbers of columns as there are rows
  *        in 'mb'. To use this function, 'mb' must be a square matrix.
  *        This function is destructive.
  *
  * @param ma    Pointer to the first input zsl_mtx.
  * @param mb    Pointer to the second input zsl_mtx.
  *
  * @return  0 if everything executed correctly, or -EINVAL if the input
  *          matrices are not compatibly shaped.
  */
 int zsl_mtx_mult_d(struct zsl_mtx *ma, struct zsl_mtx *mb);

 /**
  * @brief Multiplies all elements in matrix 'm' by scalar value 's'.
  *
  * @param m     Pointer to the zsl_mtz to adjust.
  * @param s     The scalar value to multiply elements in matrix 'm' with.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_scalar_mult_d(struct zsl_mtx *m, zsl_real_t s);

 /**
  * @brief Multiplies the elements of row 'i' in matrix 'm' by scalar 's'.
  *
  * @param m     Pointer to the zsl_mtx to use.
  * @param i     The row number to multiply by scalar 's' (0-based).
  * @param s     The scalar to use when multiplying elements of row 'i'.
  *
  * @return  0 if everything executed correctly, or -EINVAL on an out of
  *          bounds error.
  */
 int zsl_mtx_scalar_mult_row_d(struct zsl_mtx *m, size_t i, zsl_real_t s);

 /** @} */ /* End of MTX_BASICMATH group */

 /**
  * @addtogroup MTX_TRANSFORMATIONS Transformation
  *
  * @brief Transformation functions for matrices (transpose, inverse, guassian
  *        elimination, etc.).
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Transposes the matrix 'ma' into matrix 'mb'. Note that output
  *        matrix 'mb' must have 'ma->sz_rows' columns, and 'ma->sz_cols' rows.
  *
  * @param ma    Pointer to the input matrix to transpose.
  * @param mb    Pointer to the output zsl_mtz.
  *
  * @return  0 if everything executed correctly, or -EINVAL if ma and mb are
  *          not compatibly shaped.
  */
 int zsl_mtx_trans(struct zsl_mtx *ma, struct zsl_mtx *mb);

 /**
  * @brief Calculates the ajoint matrix, based on the input 3x3 matrix 'm'.
  *
  * @param m     The input 3x3 square matrix to use.
  * @param ma    The output 3x3 square matrix the adjoint values will be
  *              assigned to.
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          3x3 square matrix.
  */
 int zsl_mtx_adjoint_3x3(struct zsl_mtx *m, struct zsl_mtx *ma);

 /**
  * @brief Calculates the ajoint matrix, based on the input square matrix 'm'.
  *
  * @param m     The input square matrix to use.
  * @param ma    The output square matrix the adjoint values will be assigned to.
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          square matrix.
  */
 int zsl_mtx_adjoint(struct zsl_mtx *m, struct zsl_mtx *ma);

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief Calculates the wedge product of n-1 vectors of size n, which are the
  *        rows of the matrix 'm'. This n-1 vectors must be linearly independent.
  *
  * The wedge product is an extension of the cross product for dimension greater
  * than 3. Given a set of n-1 vectors of size n, the wedge product calculates
  * a n-dimensional vector that is perpendicular to all the others.
  *
  * @param m     The input (n-1) x n matrix, with n > 3, whose rows are the
  *              linearly independent vectors to use in the wedge product.
  * @param v     The output vector of dimension n, perpendicular to all the
  *              input vectors.
  *
  * @return  0 if everything executed correctly, or -EINVAL if n > 3, or if
  *          the input matrix isn't of the form (n-1) x n.
  */
 int zsl_mtx_vec_wedge(struct zsl_mtx *m, struct zsl_vec *v);
 #endif

 /**
  * @brief Removes row 'i' and column 'j' from square matrix 'm', assigning the
  *        remaining elements in the matrix to 'mr'.
  *
  * @param m     The input nxn square matrix to use.
  * @param mr    The output (n-1)x(n-1) square matrix.
  * @param i     The row number to remove (0-based).
  * @param j     The column number to remove (0-based).
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          square matrix.
  */
 int zsl_mtx_reduce(struct zsl_mtx *m, struct zsl_mtx *mr, size_t i, size_t j);

 /* NOTE: This is used for household method/QR. Should it be in the main lib? */
 /**
  * @brief   Reduces the number of rows/columns in the input square matrix 'm'
  *          to match the shape of 'mred', where mred < m. Rows/cols will be
  *          removed starting on the left and upper vectors.
  *
  * @param m     Pointer to the input square matrix.
  * @param mred  Pointer the the reduced output square matrix.
  * @param place1 Pointer to first placeholder matrix, it must have same size of m.
  * @param place2 Pointer to second placeholder matrix, it must have same size of m.
  *
  * @return  0 if everything executed correctly, otherwise -EAGAIN to instruct
  * 			caller to call this method again because operation is not done yet
  *
  * @note this method should be called iteratively until it returns 0
  */
 int zsl_mtx_reduce_iter(struct zsl_mtx *m, struct zsl_mtx *mred,
         struct zsl_mtx *place1, struct zsl_mtx *place2);

 /* NOTE: This is used for household method/QR. Should it be in the main lib? */
 /**
  * @brief Augments the input square matrix with additional rows and columns,
  *        based on the size 'diff' between m and maug (where maug > m). New rows
  *        and columns are assigned values based on an identity matrix, meaning
  *        1.0 on the new diagonal values and 0.0 above and below the diagonal.
  *
  * @param m     Pointer to the input square matrix.
  * @param maug  Pointer the the augmented output square matrix.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_augm_diag(struct zsl_mtx *m, struct zsl_mtx *maug);

 /**
  * @brief Calculates the determinant of the input 3x3 matrix 'm'.
  *
  * @param m     The input 3x3 square matrix to use.
  * @param d     The determinant of 3x3 square matrix m.
  *
  * @return  0 on success, or -EINVAL if this isn't a 3x3 square matrix.
  */
 int zsl_mtx_deter_3x3(struct zsl_mtx *m, zsl_real_t *d);

 /**
  * @brief Calculates the determinant of the input square matrix 'm'.
  *
  * @param m     The input square matrix to use.
  * @param d     The determinant of square matrix m.
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          square matrix.
  */
 int zsl_mtx_deter(struct zsl_mtx *m, zsl_real_t *d);

 /**
  * @brief Given the element (i,j) in matrix 'm', this function performs
  *        gaussian elimination by adding row 'i' to the other rows until
  *        all of the elements in column 'j' are equal to 0.0, aside from the
  *        element at position (i, j). The result of this process will be
  *        assigned to matrix 'mg'.
  *
  * @param m     Pointer to input zsl_mtx to use.
  * @param mg    Pointer to gaussian form output matrix.
  * @param mi    Pointer to the identity output matrix.
  * @param i     The row number of the element to use (0-based).
  * @param j     The column number of the element to use (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_gauss_elim(struct zsl_mtx *m, struct zsl_mtx *mg,
            struct zsl_mtx *mi, size_t i, size_t j);

 /**
  * @brief Given the element (i,j) in matrix 'm', this function performs
  *        gaussian elimination by adding row 'i' to the other rows until
  *        all of the elements in column 'j' are equal to 0.0, aside from the
  *        element at position (i, j). This function is destructive and will
  *        modify the contents of m.
  *
  * @param m     Pointer to zsl_mtx to use.
  * @param mi    Pointer to the identity output matrix.
  * @param i     The row number of the element to use (0-based).
  * @param j     The column number of the element to use (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_gauss_elim_d(struct zsl_mtx *m, struct zsl_mtx *mi,
        size_t i, size_t j);

 /**
  * @brief Given matrix 'm', puts the matrix into echelon form using
  *        Gauss-Jordan reduction.
  *
  * @param m     Pointer to the input square matrix to use.
  * @param mi    Pointer to the identity output matrix.
  * @param mg    Pointer to the output square matrix in echelon form.
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_gauss_reduc(struct zsl_mtx *m, struct zsl_mtx *mi,
       struct zsl_mtx *mg);

 /**
  * @brief Updates the values of every column vector in input matrix 'm' to have
  *        unitary values.
  *
  * @param m      Pointer to the input matrix.
  * @param mnorm  Pointer to the output matrix where the columns are unitary
  *               vectors.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_cols_norm(struct zsl_mtx *m, struct zsl_mtx *mnorm);

 /**
  * @brief Performs the Gram-Schmidt algorithm on the set of column vectors in
  *        matrix 'm'. This algorithm calculates a set of orthogonal vectors in
  *        the same vectorial space as the original vectors.
  *
  * @param m     Pointer to the input matrix containing the vector data.
  * @param mort  Pointer to the output matrix containing the orthogonal vector
  *              data.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_gram_schmidt(struct zsl_mtx *m, struct zsl_mtx *mort);

 /**
  * @brief Normalises elements in matrix m such that the element at position
  *        (i, j) is equal to 1.0.
  *
  * @param m     Pointer to input zsl_mtx to use.
  * @param mn    Pointer to normalised output zsl_mtx.
  * @param mi    Pointer to the output identity zsl_mtx.
  * @param i     The row number of the element to use (0-based).
  * @param j     The column number of the element to use (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_norm_elem(struct zsl_mtx *m, struct zsl_mtx *mn, struct zsl_mtx *mi,
           size_t i, size_t j);

 /**
  * @brief Normalises elements in matrix m such that the element at position
  *        (i, j) is equal to 1.0. This function is destructive and will
  *        modify the contents of m.
  *
  * @param m     Pointer to zsl_mtx to use.
  * @param mi    Pointer to the output identity zsl_mtx.
  * @param i     The row number of the element to use (0-based).
  * @param j     The column number of the element to use (0-based).
  *
  * @return 0 on success, and non-zero error code on failure
  */
 int zsl_mtx_norm_elem_d(struct zsl_mtx *m, struct zsl_mtx *mi,
       size_t i, size_t j);

 /**
  * @brief Calculates the inverse of 3x3 matrix 'm'. If the determinant of
  *        'm' is zero, an identity matrix will be returned via 'mi'.
  *
  * @param m     The input 3x3 matrix to use.
  * @param mi    The output inverse 3x3 matrix.
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          3x3 matrix.
  */
 int zsl_mtx_inv_3x3(struct zsl_mtx *m, struct zsl_mtx *mi);

 /**
  * @brief Calculates the inverse of square matrix 'm'.
  *
  * @param m     The input square matrix to use.
  * @param mi    The output inverse square matrix.
  *
  * @return  0 if everything executed correctly, or -EINVAL if this isn't a
  *          square matrix.
  */
 int zsl_mtx_inv(struct zsl_mtx *m, struct zsl_mtx *mi);

 /**
  * @brief Calculates the Cholesky decomposition of a symmetric square matrix
  *        using the Cholesky–Crout algorithm.
  *
  * Computing the Cholesky decomposition of a symmetric square matrix M consists
  * in finding a matrix L such that M = L * Lt (L multiplied by its transpose).
  *
  * @param m     The input symmetric square matrix to use.
  * @param l     The output lower triangular matrix.
  *
  * @return  0 if everything executed correctly, or -EINVAL if 'm' isn't a
  *          symmetric square matrix.
  */
 int zsl_mtx_cholesky(struct zsl_mtx *m, struct zsl_mtx *l);

 /**
  * @brief Balances the square matrix 'm', a process in which the eigenvalues of
  *        the output matrix are the same as the eigenvalues of the input matrix.
  *
  * @param m     The input square matrix to use.
  * @param mout  The output balanced matrix.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_balance(struct zsl_mtx *m, struct zsl_mtx *mout);

 /**
  * @brief Calculates the householder reflection of 'm'. Used as part of QR
  *        decomposition. When 'hessenberg' is active, it calculates the
  *        householder reflection but without using the first line of 'm'.
  *
  * @param m            Pointer to the input matrix to use.
  * @param hessenberg   If set to true, the first line in 'm' is ignored.
  * @param h            Pointer to the output square matrix where the
  *                     values of the householder reflection should be assigned.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_householder(struct zsl_mtx *m, struct zsl_mtx *h, bool hessenberg);

 /**
  * @brief If 'hessenberg' is set to false, this function performs the QR
  *        decomposition, which is a factorisation of matrix 'm' into an
  *        orthogonal matrix (q) and an upper triangular matrix (r). If
  *        'hessenberg' is set to true, this function puts the matrix 'm' into
  *        hessenberg form. This function uses the householder reflections.
  *
  * One of the uses of this function is to make use of the householder method to
  * perform the QR decomposition, which introduces the zeros below the diagonal
  * of matrix 'r'. Other algorithms exist, such as the Gram-Schmidt process,
  * but they tend to be less stable than the householder method for a similar
  * computational cost.
  *
  * @param m     Pointer to the input square matrix.
  * @param q     Pointer to the output orthoogonal square matrix.
  * @param r     Pointer to the output upper triangular square matrix or
  *              hessenberg matrix if set to true.
  * @param hessenberg Sets the matrix to hessenberg format if 'true'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_qrd(struct zsl_mtx *m, struct zsl_mtx *q, struct zsl_mtx *r,
     bool hessenberg);

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief Computes recursively the QR decompisition method to put the input
  *        square matrix into upper triangular form.
  *
  * @param m     The input square matrix to use when performing the QR
  *              decomposition.
  * @param mout  The output upper triangular square matrix where the results
  *              should be stored.
  * @param iter  The number of times that 'zsl_mtx_qrd' should be called.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_qrd_iter(struct zsl_mtx *m, struct zsl_mtx *mout, size_t iter);
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief   Calculates the eigenvalues for input matrix 'm' using QR
  *          decomposition recursively. The output vector will only contain real
  *          eigenvalues, even if the input matrix has complex eigenvalues.
  *
  * @param m     The input square matrix to use.
  * @param v     The placeholder for the output vector where the real eigenvalues
  *              should be stored.
  * @param iter  The number of times that 'zsl_mtx_qrd' should be called
  *              during the QR decomposition phase.
  *
  * NOTE: Large values required more iterations to get precise results, and
  * some trial and error may be required to find the right balance of
  * iterations versus processing time.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code. If -ECOMPLEXVAL is returned, it means that complex
  *          numbers were detected in the output eigenvalues.
  */
 int zsl_mtx_eigenvalues(struct zsl_mtx *m, struct zsl_vec *v, size_t iter);
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief Calcualtes the set of eigenvectors for input matrix 'm', using the
  *        specified number of iterations to find a balance between precision and
  *        processing effort. Optionally, the output eigenvectors can be
  *        orthonormalised.
  *
  * @param m             The input square matrix to use.
  * @param mev           The placeholder for the output square matrix where the
  *                      eigenvectors should be stored as column vectors.
  * @param iter          The number of times that 'zsl_mtx_qrd' should be called.
  *                      during the QR decomposition phase.
  * @param orthonormal   If set to true, the output matrix 'mev' will be
  *                      orthonormalised.
  *
  * NOTE: Large values required more iterations to get precise results, and
  * some trial and error may be required to find the right balance of
  * iterations versus processing time.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code. If the number of calcualted eigenvectors is less
  *          than the columns in 'm', EEIGENSIZE will be returned.
  */
 int zsl_mtx_eigenvectors(struct zsl_mtx *m, struct zsl_mtx *mev, size_t iter,
        bool orthonormal);
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief Performs singular value decomposition, converting input matrix 'm'
  *        into matrices 'u', 'e', and 'v'.
  *
  * @param m     The input mxn matrix to use.
  * @param u     The placeholder for the output mxm matrix u.
  * @param e     The placeholder for the output mxn matrix sigma.
  * @param v     The placeholder for the output nxn matrix v.
  * @param iter  The number of times that 'zsl_mtx_qrd' should be called.
  *              during the QR decomposition phase.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_svd(struct zsl_mtx *m, struct zsl_mtx *u, struct zsl_mtx *e,
     struct zsl_mtx *v, size_t iter);
 #endif

 #ifndef CONFIG_ZSL_SINGLE_PRECISION
 /**
  * @brief   Performs the pseudo-inverse (aka pinv or Moore-Penrose inverse)
  *          on input matrix 'm'.
  *
  * @param m     The input mxn matrix to use.
  * @param pinv  The placeholder for the output pseudo inverse nxm matrix.
  * @param iter  The number of times that 'zsl_mtx_qrd' should be called.
  *              during the QR decomposition phase.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_pinv(struct zsl_mtx *m, struct zsl_mtx *pinv, size_t iter);
 #endif

 /** @} */ /* End of MTX_TRANSFORMATIONS group */

 /**
  * @addtogroup MTX_LIMITS Limits
  *
  * @brief Min/max helpers to determine the range of the matrice's coefficients.
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Traverses the matrix elements to find the minimum element value.
  *
  * @param m     Pointer to the zsl_mtz to traverse.
  * @param x     The minimum element value found in matrix 'm'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_min(struct zsl_mtx *m, zsl_real_t *x);

 /**
  * @brief Traverses the matrix elements to find the maximum element value.
  *
  * @param m     Pointer to the zsl_mtz to traverse.
  * @param x     The maximum element value found in matrix 'm'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_max(struct zsl_mtx *m, zsl_real_t *x);

 /**
  * @brief Traverses the matrix elements to find the (i,j) index of the minimum
  *        element value. If multiple identical minimum values are founds,
  *        the (i, j) index values returned will refer to the first element.
  *
  * @param m     Pointer to the zsl_mtz to traverse.
  * @param i     Pointer to the row index of the minimum element value found
  *              in matrix 'm'.
  * @param j     Pointer to the column index of the minimum element value
  *              found in matrix 'm'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_min_idx(struct zsl_mtx *m, size_t *i, size_t *j);

 /**
  * @brief Traverses the matrix elements to find the (i,j) index of the maximum
  *        element value. If multiple identical maximum values are founds,
  *        the (i, j) index values returned will refer to the first element.
  *
  * @param m     Pointer to the zsl_mtz to traverse.
  * @param i     Pointer to the row index of the maximum element value found
  *              in matrix 'm'.
  * @param j     Pointer to the column index of the maximum element value
  *              found in matrix 'm'.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_max_idx(struct zsl_mtx *m, size_t *i, size_t *j);

 /** @} */ /* End of MTX_LIMITS group */

 /**
  * @addtogroup MTX_COMPARISON Comparison
  *
  * @brief Functions used to compare or verify matrices.
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Checks if two matrices are identical in shape and content.
  *
  * @param ma The first matrix.
  * @param mb The second matrix.
  *
  * @return true if the two matrices have the same shape and values,
  *         otherwise false.
  */
 bool zsl_mtx_is_equal(struct zsl_mtx *ma, struct zsl_mtx *mb);

 /**
  * @brief Checks if all elements in matrix m are >= zero.
  *
  * @param m The matrix to check.
  *
  * @return true if the all matrix elements are zero or positive,
  *         otherwise false.
  */
 bool zsl_mtx_is_notneg(struct zsl_mtx *m);

 /**
  * @brief Checks if the square input matrix is symmetric.
  *
  * @param m The matrix to check.
  *
  * @return true if the matrix is symmetric, otherwise false.
  */
 bool zsl_mtx_is_sym(struct zsl_mtx *m);

 /** @} */ /* End of MTX_COMPARISON group */

 /**
  * @addtogroup MTX_DISPLAY Display
  *
  * @brief Functions used to present matrices in a user-friendly format.
  *
  * @ingroup MATRICES
  *  @{ */

 /**
  * @brief Printf the supplied matrix using printf in a human-readable manner.
  *
  * @param m     Pointer to the matrix to print.
  *
  * @return  0 if everything executed correctly, otherwise an appropriate
  *          error code.
  */
 int zsl_mtx_print(struct zsl_mtx *m);

 // int      zsl_mtx_fprint(FILE *stream, zsl_mtx *m);

 /** @} */ /* End of MTX_DISPLAY group */

 #ifdef __cplusplus
 }
 #endif

 #endif /* ZEPHYR_INCLUDE_ZSL_MATRICES_H_ */

 /** @} */ /* End of matrices group */