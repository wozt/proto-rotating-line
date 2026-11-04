/* SPDX-License-Identifier: MIT */
/* An open double-armed Archimedean spiral: mathematically parametric. */
#include "parametric.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f
#define RANGE 168.0f
#define X_STEP (0.5f)

/* Stylized curling wave crest: a broad rising body flows into ONE
 * progressively narrowing tip. The final connector lies near the baseline,
 * avoiding the old symmetric "C" silhouette. The spiral radius tends to zero
 * before the connector, rather than creating another full-size arm.
 * A fraction of the shape is interpolated against the straight baseline when
 * curl is in [0, 1], keeping all intermediate stages continuous. */
void proto_spiral_point(float u, float cell_start, float cell_span,
                        float amplitude, float curl, float *x, float *y) {
    curl = fmaxf(0.0f, fminf(1.0f, curl));
    u = fmaxf(0.0f, fminf(1.0f, u));
    const float crest_end = 0.37f;
    const float coil_end = 0.88f;
    const float crest_x = 0.29f * cell_span;
    const float crest_y = fminf(0.85f * amplitude, 0.34f * cell_span);
    const float coil_x = 0.70f * cell_span;
    const float coil_y = 0.63f * crest_y;
    float coiled_x, coiled_y;

    if (u <= crest_end) {
        /* Gradually rising and narrow wave stalk. A cubic crest gives the
         * Wind Waker-inspired lip a slender, tapering silhouette. */
        const float v = u / crest_end;
        coiled_x = crest_x * v;
        coiled_y = crest_y * v * v * v * (4.0f - 3.0f * v);
    } else if (u < coil_end) {
        /* The lip curls inward while radius falls to zero at its tip;
         * this is a tapering single arm, not a pair of constant-width arcs. */
        const float v = (u - crest_end) / (coil_end - crest_end);
        const float dx = crest_x - coil_x;
        const float dy = crest_y - coil_y;
        /* Preserve the outside curl and taper the inner tongue to a hairline.
         * Smoothstep avoids a kink in the radius derivative at v=0.60. */
        const float q = fmaxf(0.0f, fminf(1.0f, (v - 0.60f) / 0.40f));
        const float tip_taper = 1.0f - 0.65f * q * q * (3.0f - 2.0f * q);
        const float radius = hypotf(dx, dy) * powf(1.0f - v, 1.25f) * tip_taper;
        const float theta = atan2f(dy, dx) - 2.75f * PI * v;
        coiled_x = coil_x + radius * cosf(theta);
        coiled_y = coil_y + radius * sinf(theta);
    } else {
        /* A short descent to the baseline leaves the next cell connected. */
        const float v = (u - coil_end) / (1.0f - coil_end);
        coiled_x = coil_x + (cell_span - coil_x) * v;
        coiled_y = coil_y * (1.0f - v);
    }
    const float straight_x = u * cell_span;
    *x = cell_start + straight_x + curl * (coiled_x - straight_x);
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
