#include "interpolation.h"
#include <assert.h>

void interpolation_linear_init(interp_ctx_t *ctx, const int32_t *input_values, size_t input_values_count,
                               const int32_t *output_values, size_t output_values_count)
{
    ctx->input_values[0]     = input_values;
    ctx->input_values[1]     = NULL; /* Not used in 1D interpolation. */
    ctx->out_values          = output_values;
    ctx->input_array_size[0] = input_values_count;
    ctx->input_array_size[1] = 0; /* Not used in 1D interpolation. */
    ctx->output_array_size   = output_values_count;
    assert(output_values_count == input_values_count);
}

void interpolation_bilinear_init(interp_ctx_t *ctx, const int32_t *in_1_values, size_t in_1_values_count,
                                 const int32_t *in_2_values, size_t in_2_values_count, const int32_t *out_values,
                                 size_t out_values_count)
{
    ctx->input_values[0]     = in_1_values;
    ctx->input_values[1]     = in_2_values;
    ctx->out_values          = out_values;
    ctx->input_array_size[0] = in_1_values_count;
    ctx->input_array_size[1] = in_2_values_count;
    ctx->output_array_size   = out_values_count;
    assert(out_values_count == in_1_values_count * in_2_values_count);
}

int32_t interpolation_linear_compute(const interp_ctx_t *ctx, int32_t input_value)
{
    size_t n             = ctx->input_array_size[0];
    const int32_t *x_arr = ctx->input_values[0];
    const int32_t *y_arr = ctx->out_values;

    /* Clamp if input_value is out of range. */
    if (input_value <= x_arr[0]) {
        return y_arr[0];
    }
    if (input_value >= x_arr[n - 1]) {
        return y_arr[n - 1];
    }

    /* Binary search for the right interval. [low, high] */
    size_t low = 0, high = n - 1;
    while (high - low > 1) {
        size_t mid = (low + high) / 2;
        if (input_value < x_arr[mid]) {
            high = mid;
        } else {
            low = mid;
        }
    }

    int32_t x0 = x_arr[low];
    int32_t x1 = x_arr[high];
    int32_t y0 = y_arr[low];
    int32_t y1 = y_arr[high];

    assert(x0 < x1);

    /* Linear interpolation. */
    return y0 + (int64_t)(y1 - y0) * (input_value - x0) / (x1 - x0);
}

int32_t interpolation_bilinear_compute(const interp_ctx_t *ctx, int32_t input_value_1, int32_t input_value_2)
{
    size_t nx            = ctx->input_array_size[0];
    size_t ny            = ctx->input_array_size[1];
    const int32_t *x_arr = ctx->input_values[0];
    const int32_t *y_arr = ctx->input_values[1];
    const int32_t *f_arr = ctx->out_values;

    /* Clamp X */
    if (input_value_1 <= x_arr[0]) {
        input_value_1 = x_arr[0];
    }
    if (input_value_1 >= x_arr[nx - 1]) {
        input_value_1 = x_arr[nx - 1];
    }

    /* Clamp Y */
    if (input_value_2 <= y_arr[0]) {
        input_value_2 = y_arr[0];
    }
    if (input_value_2 >= y_arr[ny - 1]) {
        input_value_2 = y_arr[ny - 1];
    }

    /* Binary search in X. */
    size_t lx = 0, ux = nx - 1;
    while (ux - lx > 1) {
        size_t mid = (lx + ux) / 2;
        if (input_value_1 < x_arr[mid]) {
            ux = mid;
        } else {
            lx = mid;
        }
    }

    /* Binary search in Y. */
    size_t ly = 0, uy = ny - 1;
    while (uy - ly > 1) {
        size_t mid = (ly + uy) / 2;
        if (input_value_2 < y_arr[mid]) {
            uy = mid;
        } else {
            ly = mid;
        }
    }

    /* Grid coordinates. */
    int32_t x0 = x_arr[lx], x1 = x_arr[ux];
    int32_t y0 = y_arr[ly], y1 = y_arr[uy];

    /* Values at corners (row-major: f[x][y] → f[x * ny + y]) */
    int32_t f00 = f_arr[lx * ny + ly];
    int32_t f01 = f_arr[lx * ny + uy];
    int32_t f10 = f_arr[ux * ny + ly];
    int32_t f11 = f_arr[ux * ny + uy];

    assert(x0 < x1);
    assert(y0 < y1);

    /* Full bilinear interpolation. */
    int64_t wx1 = input_value_1 - x0;
    int64_t wx0 = x1 - input_value_1;
    int64_t wy1 = input_value_2 - y0;
    int64_t wy0 = y1 - input_value_2;

    int64_t denom = (int64_t)(x1 - x0) * (y1 - y0);

    int64_t result =
        (int64_t)f00 * wx0 * wy0 + (int64_t)f10 * wx1 * wy0 + (int64_t)f01 * wx0 * wy1 + (int64_t)f11 * wx1 * wy1;

    return (int32_t)(result / denom);
}
