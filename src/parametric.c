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

    const float lead_end = 0.20f;
    const float coil_end = 0.86f;
    const float straight_x = u * cell_span;
    const float center_x = 0.50f * cell_span;
    const float center_y = 0.0f;
    const float outer_radius = fminf(0.60f * amplitude, 0.15f * cell_span);
    const float tail_start_x = center_x; /* Tangent-continuous join at curl centre. */
    float shape_x = straight_x;
    float shape_y = 0.0f;

    if (u <= lead_end) {
        const float v = u / lead_end;
        const float entry_x = center_x - outer_radius;
        shape_x = entry_x * v;
        shape_y = 0.0f;
    } else if (u < coil_end) {
        const float v = (u - lead_end) / (coil_end - lead_end);
        const float eased = v * v * (3.0f - 2.0f * v);
        const float turns = 1.65f;
        const float theta = PI + 2.0f * PI * turns * eased;
        const float radius = outer_radius * powf(1.0f - eased, 2.65f);
        shape_x = center_x + radius * cosf(theta);
        shape_y = center_y + radius * sinf(theta);
    } else {
        const float v = (u - coil_end) / (1.0f - coil_end);
        shape_x = tail_start_x + (cell_span - tail_start_x) * v;
        shape_y = 0.0f;
    }

    *x = cell_start + straight_x + curl * (shape_x - straight_x);
    *y = curl * shape_y;
}

/* For the entire rotated polyline, intersect a vertical ray with the open
 * separator in its local frame. Scan conversion happens once per frame.
 * The even/odd rule handles the folded spiral without flood-fill or art. */
/* Single-armed tentacle curl, joined to the separator at both endpoints.
 * The base of the curl is relatively round, its inner tip reaches radius 0,
 * and its centre remains on the horizontal line. This is NOT the old wave crest. */
void proto_tentacle_gradient_point(float u, float start, float span,
                                  float amplitude, float *x, float *y) {
    u = fmaxf(0.0f, fminf(1.0f, u));
    const float lead = 0.23f, finish = 0.83f;
    const float centre = 0.51f * span;
    const float radius0 = fminf(0.85f * amplitude, 0.335f * span);
    float px, py;
    if (u <= lead) {
        px = (centre - radius0) * (u / lead);
        py = 0.0f;
    } else if (u < finish) {
        float v = (u - lead) / (finish - lead);
        float theta = PI + 3.55f * PI * v;
        float radius = radius0 * powf(1.0f - v, 1.82f);
        px = centre + radius * cosf(theta);
        py = radius * sinf(theta);
    } else {
        float v = (u - finish) / (1.0f - finish);
        px = centre + (span - centre) * v;
        py = 0.0f;
    }
    *x = start + px;
    *y = py;
}

/* Wolfram Alpha: {(1-t)^2 cos(6*pi*t), (1-t)^2 sin(6*pi*t)},
 * t in [0,0.5].  The inner arc is EXACTLY this function, only translated
 * and uniformly scaled to fit a cell; the two baseline connectors are separate.
 * strength=0 yields a straight divider; strength=1 yields the full curve. */
void proto_wolfram_halfturn_point(float p, float cell_start,
                                  float cell_span, float amplitude,
                                  float strength, float *x, float *y) {
    p = fmaxf(0.0f, fminf(1.0f, p));
    strength = fmaxf(0.0f, fminf(1.0f, strength));
    const float lead = 0.12f, finish = 0.88f;
    const float centre_x = 0.5f * cell_span;
    const float scale = fminf(fmaxf(0.0f, amplitude), 0.42f * cell_span);
    const float baseline_x = p * cell_span;
    float shape_x, shape_y = 0.0f;
    if (p <= lead) {
        shape_x = (centre_x + scale) * (p / lead);
    } else if (p < finish) {
        const float t = 0.5f * (p - lead) / (finish - lead);
        const float r = (1.0f - t) * (1.0f - t);
        shape_x = centre_x + scale * r * cosf(6.0f * PI * t);
        shape_y = scale * r * sinf(6.0f * PI * t);
    } else {
        const float q = (p - finish) / (1.0f - finish);
        const float last_x = centre_x - 0.25f * scale;
        shape_x = last_x + (cell_span - last_x) * q;
    }
    *x = cell_start + baseline_x + strength * (shape_x - baseline_x);
    *y = strength * shape_y;
}

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
