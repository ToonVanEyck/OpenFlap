#pragma once

#include <stddef.h>
#include <stdint.h>

#define INTERPOLATION_MAX_DIMENSIONS 2

/**
 * @brief Piecewise linear interpolation context
 */
typedef struct {
    const int32_t *input_values[INTERPOLATION_MAX_DIMENSIONS]; /**< Array of  arrays of input variables for
                                                                  interpolation (must be sorted in ascending order). */
    const int32_t *out_values;                                 /**< Array of output variables
                                                               (must be sorted in ascending order). */
    size_t input_array_size[INTERPOLATION_MAX_DIMENSIONS];     /**< Number of data points per input array. */
    size_t output_array_size;                                  /**< Number of data points in the output array. */
} interp_ctx_t;

/**
 * @brief Initialize interpolation context for a 2d interpolation matrix
 *
 * @param ctx Interpolation context to initialize
 * @param in_1_values Array of input values for the x-axis (must be sorted in ascending order)
 * @param in_1_values_count Number of input values for the x-axis
 * @param in_2_values Array of input values for the y-axis (must be sorted in ascending order)
 * @param in_2_values_count Number of input values for the y-axis
 * @param out_values Array of output values (must be sorted in ascending order)
 * @param out_values_count Number of output values
 */
void interpolation_bilinear_init(interp_ctx_t *ctx, const int32_t *in_1_values, size_t in_1_values_count,
                                 const int32_t *in_2_values, size_t in_2_values_count, const int32_t *out_values,
                                 size_t out_values_count);

/**
 * @brief Initialize interpolation context
 *
 * @param ctx Interpolation context to initialize
 * @param input_values Array of input values (must be sorted in ascending order)
 * @param input_values_count Number of input values
 * @param output_values Array of output values (must be sorted in ascending order)
 * @param output_values_count Number of output values
 */
void interpolation_linear_init(interp_ctx_t *ctx, const int32_t *input_values, size_t input_values_count,
                               const int32_t *output_values, size_t output_values_count);

/**
 * @brief Perform piecewise linear interpolation
 *
 * @param ctx Interpolation context
 * @param input_value Input value to interpolate
 * @return Interpolated output value
 *
 * @note If input_value is outside the range of data points, the function will:
 *       - Return the first output_value if input_value < first input_value value
 *       - Return the last output_value if input_value > last input_value value
 */
int32_t interpolation_linear_compute(const interp_ctx_t *ctx, int32_t input_value);

int32_t interpolation_bilinear_compute(const interp_ctx_t *ctx, int32_t input_value_1, int32_t input_value_2);
