/* SPDX-License-Identifier: MIT */
/* An open double-armed Archimedean spiral: mathematically parametric. */
#include "parametric.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define RANGE 168.0f
#define X_STEP (0.5f)

/* The incoming and outgoing spiral arms are separated by PI at equal radii.
 * They join at their centre and leave to opposite ends of the baseline.
 * A straight connector at each end preserves an open, two-sided separator.
 * Unlike an y=f(x) graph, the tangent coordinate reverses within the curl. */
void proto_spiral_point(float u, float section_start, float section_span,
                        float amplitude, float *x, float *y) {
    const float centre = section_span * 0.5f;
    const float radius = fminf(42.0f, amplitude * 1.30f);
    const float start = centre - radius;
    const float finish = centre + radius;
    const float left = 0.18f, inner = 0.50f, right = 0.82f;
    float localx = 0.0f, localy = 0.0f;
    if (u < left) {
        localx = (u / left) * start;
    } else if (u <= inner) {
        float v = (u - left) / (inner - left);
        float r = radius * (1.0f - v);
        float a = PI + 3.0f * PI * v;
        localx = centre + r * cosf(a);
        localy = r * sinf(a);
    } else if (u <= right) {
        float v = (u - inner) / (right - inner);
        float r = radius * v;
        float a = 5.0f * PI - 3.0f * PI * v;
        localx = centre + r * cosf(a);
        localy = r * sinf(a);
    } else {
        localx = finish + (u - right) / (1.0f - right) * (section_span - finish);
    }
    *x = section_start + localx;
    *y = localy;
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
