#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Data point structure for piecewise linear interpolation
 */
typedef struct {
    int32_t x; /**< Input value */
    int32_t y; /**< Output value */
} interp_point_t;

/**
 * @brief Piecewise linear interpolation context
 */
typedef struct {
    const interp_point_t *points; /**< Array of data points (must be sorted by x values) */
    size_t num_points;            /**< Number of data points */
} interp_ctx_t;

/**
 * @brief Initialize interpolation context
 *
 * @param ctx Interpolation context to initialize
 * @param points Array of data points (must be sorted by x values in ascending order)
 * @param num_points Number of data points
 */
void interp_init(interp_ctx_t *ctx, const interp_point_t *points, size_t num_points);

/**
 * @brief Perform piecewise linear interpolation
 *
 * @param ctx Interpolation context
 * @param x Input value to interpolate
 * @return Interpolated output value
 *
 * @note If x is outside the range of data points, the function will:
 *       - Return the first y value if x < first x value
 *       - Return the last y value if x > last x value
 */
int32_t interp_compute(const interp_ctx_t *ctx, int32_t x);
