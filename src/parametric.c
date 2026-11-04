/* SPDX-License-Identifier: MIT */
/* An open double-armed Archimedean spiral: mathematically parametric. */
#include "parametric.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define RANGE 168.0f
#define X_STEP (0.5f)

/* A SINGLE Archimedean-style arm per cell, not two mirrored curls.
 * The radius decreases to zero at its inner tip.  The outgoing connector
 * stays straight, so it cannot look like a second, competing spiral.
 * curl [0,1] unwinds both the angle and the displacement continuously. */
void proto_spiral_point(float u, float cell_start, float cell_span,
                        float amplitude, float curl, float *x, float *y) {
    const float pi = 3.14159265358979323846f;
    curl = fmaxf(0.0f, fminf(1.0f, curl));
    u = fmaxf(0.0f, fminf(1.0f, u));
    const float lead = 0.12f, tip = 0.77f;
    const float centre = 0.49f * cell_span;
    const float radius = fminf(amplitude * 1.15f, 0.37f * cell_span);
    float bx = u * cell_span;
    float coiled_x, coiled_y = 0.0f;
    if (u < lead) {
        coiled_x = (u / lead) * (centre - radius);
    } else if (u < tip) {
        const float v = (u - lead) / (tip - lead);
        const float r = radius * powf(1.0f - v, 1.10f);
        const float theta = pi + (3.60f * pi) * curl * v;
        coiled_x = centre + r * cosf(theta);
        coiled_y = r * sinf(theta);
    } else {
        coiled_x = centre + (u - tip) / (1.0f - tip) * (cell_span - centre);
    }
    *x = cell_start + bx + curl * (coiled_x - bx);
    *y = curl * coiled_y;
}

/* For the entire rotated polyline, intersect a vertical ray with the open
 * separator in its local frame. Scan conversion happens once per frame.
 * The even/odd rule handles the folded spiral without flood-fill or art. */
void proto_parametric_prepare(ProtoFrame *frame) {
    memset(frame->cross_count, 0, sizeof frame->cross_count);
    for (int i = 0; i < PROTO_PROFILE_SAMPLES - 1; ++i) {
        float x0 = frame->x_coords[i], x1 = frame->x_coords[i+1];
        float y0 = frame->offsets[i], y1 = frame->offsets[i+1];
        if (fabsf(x1 - x0) < 1.0e-6f) continue;
        float lo = fminf(x0,x1), hi = fmaxf(x0,x1);
        int begin = (int)ceilf((lo + RANGE) / X_STEP - 0.5f);
        int end = (int)ceilf((hi + RANGE) / X_STEP - 0.5f);
        if (begin < 0) begin = 0;
        if (end > PROTO_CROSS_COLS) end = PROTO_CROSS_COLS;
        for (int col = begin; col < end; ++col) {
            float px = -RANGE + (col + 0.5f) * X_STEP;
            float py = y0 + (px - x0) * (y1 - y0) / (x1 - x0);
            unsigned n = frame->cross_count[col];
            if (n >= PROTO_CROSS_MAX) continue;
            while (n > 0 && frame->cross_y[col][n-1] > py) {
                frame->cross_y[col][n] = frame->cross_y[col][n-1];
                --n;
            }
            frame->cross_y[col][n] = py;
            ++frame->cross_count[col];
        }
    }
}

float proto_parametric_side(const ProtoFrame *frame, float tangent, float normal) {
    int col = (int)floorf((tangent + RANGE) / X_STEP);
    if (col < 0) col = 0;
    if (col >= PROTO_CROSS_COLS) col = PROTO_CROSS_COLS - 1;
    unsigned count = frame->cross_count[col];
    float nearest = 1.0e6f;
    unsigned crossings = 0;
    for (unsigned i = 0; i < count; ++i) {
        float difference = normal - frame->cross_y[col][i];
        float distance = fabsf(difference);
        if (distance < nearest) nearest = distance;
        if (difference >= 0.0f) ++crossings;
    }
    /* Top of the local frame starts violet. One crossing flips to cyan. */
    return crossings & 1u ? nearest : -nearest;
}
