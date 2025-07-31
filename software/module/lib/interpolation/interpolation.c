#include "interpolation.h"

void interp_init(interp_ctx_t *ctx, const interp_point_t *points, size_t num_points)
{
    ctx->points     = points;
    ctx->num_points = num_points;
}

int32_t interp_compute(const interp_ctx_t *ctx, int32_t x)
{
    // Handle empty context
    if (ctx->num_points == 0) {
        return 0;
    }

    // Handle single point
    if (ctx->num_points == 1) {
        return ctx->points[0].y;
    }

    // Handle x below range - return first y value
    if (x <= ctx->points[0].x) {
        return ctx->points[0].y;
    }

    // Handle x above range - return last y value
    if (x >= ctx->points[ctx->num_points - 1].x) {
        return ctx->points[ctx->num_points - 1].y;
    }

    // Find the segment containing x
    for (size_t i = 0; i < ctx->num_points - 1; i++) {
        if (x >= ctx->points[i].x && x <= ctx->points[i + 1].x) {
            // Linear interpolation between points i and i+1
            int32_t x0 = ctx->points[i].x;
            int32_t y0 = ctx->points[i].y;
            int32_t x1 = ctx->points[i + 1].x;
            int32_t y1 = ctx->points[i + 1].y;

            // Avoid division by zero (shouldn't happen with sorted points)
            if (x1 == x0) {
                return y0;
            }

            // Linear interpolation: y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
            int32_t numerator   = (y1 - y0) * (x - x0);
            int32_t denominator = (x1 - x0);

            return y0 + (numerator / denominator);
        }
    }

    // This should never be reached with proper input
    return ctx->points[ctx->num_points - 1].y;
}
