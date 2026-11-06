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

/* The Wolfram spiral, with an analytically derived unit normal for the
 * second nearby strand. The centreline equation itself is not modified. */
static void wolfram_tentacle_sample(float t, float *x, float *y,
                                    float *nx, float *ny) {
    const float radius = (1.0f - t) * (1.0f - t);
    const float dr = -2.0f * (1.0f - t);
    const float theta = 6.0f * PI * t;
    const float cs = cosf(theta), sn = sinf(theta);
    const float dx = dr * cs - 6.0f * PI * radius * sn;
    const float dy = dr * sn + 6.0f * PI * radius * cs;
    const float speed = sqrtf(dx * dx + dy * dy);
    *x = radius * cs;
    *y = radius * sn;
    *nx = dy / speed;
    *ny = -dx / speed;
}

/* Two near-parallel spirals form a continuous, pointed tentacle outline.
 * From p=.12 to .50 the first strand samples Wolfram t=0..t_limit.
 * From p=.50 to .88 the second strand samples t_limit..0 in reverse.
 * Its normal offset is largest at the base and tends to zero at the tip,
 * so both arms meet exactly at p=.50. The remaining span is a straight
 * connector to the separator. A zero t_limit gives an exact flat line. */
void proto_wolfram_tentacle_point(float p, float cell_start, float cell_span,
                                  float amplitude, float t_limit,
                                  float strand_offset, float *x, float *y) {
    p = fmaxf(0.0f, fminf(1.0f, p));
    t_limit = fmaxf(0.0f, fminf(0.5f, t_limit));
    const float lead = 0.12f, tip = 0.50f, tail = 0.88f;
    const float half_span = 0.5f * cell_span;
    const float scale = fminf(fmaxf(0.0f, amplitude), 0.39f * cell_span);
    const float offset = fminf(1.25f, fmaxf(0.0f, strand_offset));
    const float blend = fminf(1.0f, t_limit / 0.045f);
    const float fade = blend * blend * (3.0f - 2.0f * blend);
    float sx = p * cell_span, sy = 0.0f;
    float wx = 0.0f, wy = 0.0f, nx = 0.0f, ny = 0.0f;

    if (p <= lead) {
        sx = (half_span + scale) * p / lead;
    } else if (p <= tip) {
        const float q = (p - lead) / (tip - lead);
        wolfram_tentacle_sample(q * t_limit, &wx, &wy, &nx, &ny);
        sx = half_span + scale * wx;
        sy = scale * wy;
    } else if (p <= tail) {
        const float q = (tail - p) / (tail - tip);
        const float t = q * t_limit;
        wolfram_tentacle_sample(t, &wx, &wy, &nx, &ny);
        /* Width decreases continuously to zero at the moving inner tip. */
        const float width = offset * powf(1.0f - q, 1.4f);
        sx = half_span + scale * wx + width * nx;
        sy = scale * wy + width * ny;
    } else {
        /* Continue from the second arm's base without any positional jump. */
        wolfram_tentacle_sample(0.0f, &wx, &wy, &nx, &ny);
        const float q = (p - tail) / (1.0f - tail);
        const float start_x = half_span + scale + offset * nx;
        const float start_y = offset * ny;
        sx = start_x + (cell_span - start_x) * q;
        sy = start_y * (1.0f - q);
    }
    const float baseline_x = p * cell_span;
    *x = cell_start + baseline_x + fade * (sx - baseline_x);
    *y = fade * sy;
}

/* A variable-radius capsule gives a closed, filled strip along one segment
 * of the Wolfram centreline. Rasterizing the union of consecutive capsules
 * avoids the self-intersecting polygon / even-odd artifacts of double spirals.
 * Coordinates are pixel-centred, and all calculations are in logical pixels. */
static void ribbon_capsule(ProtoFrame *frame,
                           float ax, float ay, float ar,
                           float bx, float by, float br,
                           unsigned char ink) {
    const float rmax = fmaxf(ar, br);
    int xmin = (int)floorf(fminf(ax, bx) - rmax - 1.0f);
    int xmax = (int)ceilf(fmaxf(ax, bx) + rmax + 1.0f);
    int ymin = (int)floorf(fminf(ay, by) - rmax - 1.0f);
    int ymax = (int)ceilf(fmaxf(ay, by) + rmax + 1.0f);
    if (xmin < 0) xmin = 0;
    if (ymin < 0) ymin = 0;
    if (xmax >= PROTO_WIDTH) xmax = PROTO_WIDTH - 1;
    if (ymax >= PROTO_HEIGHT) ymax = PROTO_HEIGHT - 1;
    const float vx = bx - ax, vy = by - ay;
    const float length2 = vx*vx + vy*vy;
    for (int y = ymin; y <= ymax; ++y) {
        const float py = (float)y + 0.5f;
        for (int x = xmin; x <= xmax; ++x) {
            const float px = (float)x + 0.5f;
            float u = length2 > 1.0e-9f ?
                ((px-ax)*vx + (py-ay)*vy) / length2 : 0.0f;
            u = fmaxf(0.0f, fminf(1.0f, u));
            const float rx = px - (ax + u*vx), ry = py - (ay + u*vy);
            const float radius = ar + u * (br-ar);
            if (rx*rx + ry*ry <= radius*radius)
                frame->ribbon_mask[(size_t)y * PROTO_WIDTH + x] = ink;
        }
    }
}

/* Render one open spiral as a closed variable-width tube (round base cap,
 * tapered point). Its centreline is precisely the input Wolfram equation.
 * The centreline occupies t=0..t_limit, not the full .5 range at all times. */
static void ribbon_cell(ProtoFrame *frame, float cell_start, float cell_span,
                        float amplitude, float full_width, float t_limit) {
    if (t_limit <= 0.00001f || full_width <= 0.0f) return;
    const float scale = fminf(fmaxf(0.0f, amplitude), 0.39f * cell_span);
    const float centre = cell_start + 0.5f * cell_span;
    const float b = fminf(1.0f, t_limit / 0.045f);
    const float fade = b*b*(3.0f-2.0f*b);
    const float half_width = 0.5f * full_width * fade;
    const int samples = 128;
    float prev_x = 0.0f, prev_y = 0.0f, prev_r = 0.0f;
    for (int k = 0; k <= samples; ++k) {
        const float u = (float)k / (float)samples;
        const float t = t_limit * u;
        const float radius = (1.0f-t)*(1.0f-t);
        const float angle = 6.0f * PI * t;
        const float tangent = centre + scale * radius * cosf(angle);
        const float normal = scale * radius * sinf(angle);
        const float sx = PROTO_WIDTH * 0.5f +
                         tangent * frame->cos_angle - normal * frame->sin_angle;
        const float sy = PROTO_HEIGHT * 0.5f +
                         tangent * frame->sin_angle + normal * frame->cos_angle;
        const float width = half_width * powf(1.0f-u, 1.25f);
        if (k > 0)
            ribbon_capsule(frame, prev_x, prev_y, prev_r, sx, sy, width, 1);
        prev_x = sx;
        prev_y = sy;
        prev_r = width;
    }
}

/* Builds the filled cyan tentacle mask separately from the open world divider.
 * The inside of the tube never changes colour when a curl overlaps another. */
/* Two independent, genuinely coloured ribbons that coil around a common
 * centre. They are opposite-phase arms, not a cyan spiral with a white edge.
 * A short root ties the cyan arm to the cyan half and the violet arm to the
 * violet half. Each arm tapers to the animated endpoint (not a fixed tip). */
static void double_ribbon_point(float u, float t_limit, float centre,
                                float scale, float side, float *x, float *y) {
    const float t = t_limit * u;
    const float radius = scale * (1.0f-t)*(1.0f-t);
    const float angle = 6.0f * PI * t + side * (0.5f * PI);
    *x = centre + radius * cosf(angle);
    *y = radius * sinf(angle);
}

static void double_ribbon_world_to_screen(const ProtoFrame *frame,
                                          float tx, float ty,
                                          float *sx, float *sy) {
    *sx = PROTO_WIDTH * 0.5f + tx * frame->cos_angle - ty * frame->sin_angle;
    *sy = PROTO_HEIGHT * 0.5f + tx * frame->sin_angle + ty * frame->cos_angle;
}

static void double_ribbon_cell(ProtoFrame *frame, float cell_start,
                               float cell_span, float amplitude,
                               float ribbon_width, float t_limit) {
    if (t_limit <= 0.00001f) return;
    const float scale = fminf(fmaxf(amplitude, 0.0f), 0.35f * cell_span);
    if (scale <= 0.0f) return;
    const float centre = cell_start + 0.50f * cell_span;
    const float opacity = fminf(1.0f, t_limit / 0.045f);
    const float fade = opacity * opacity * (3.0f - 2.0f * opacity);
    const float half_width = 0.5f * ribbon_width * fade;
    const int segments = 96;
    float prev_x[2], prev_y[2], prev_w[2];
    float next_x[2], next_y[2], next_w[2];

    for (int arm = 0; arm < 2; ++arm) {
        const float side = arm == 0 ? +1.0f : -1.0f;
        float tx, ty;
        double_ribbon_point(0.0f, t_limit, centre, scale, side, &tx, &ty);
        double_ribbon_world_to_screen(frame, tx, ty,
                                       &prev_x[arm], &prev_y[arm]);
        prev_w[arm] = half_width;
        /* Attach each coloured ribbon to the matching side of the
         * separating line. This root lies just inside its own half-plane. */
        const float root_x = centre - 0.72f * scale;
        const float root_y = side * 0.55f;
        float sx, sy;
        double_ribbon_world_to_screen(frame, root_x, root_y, &sx, &sy);
        ribbon_capsule(frame, sx, sy, half_width * 0.55f,
                       prev_x[arm], prev_y[arm], half_width,
                       arm == 0 ? 2 : 3);
    }

    for (int k = 1; k <= segments; ++k) {
        const float u = (float)k / (float)segments;
        const float radius_factor = powf(1.0f-u, 1.15f);
        for (int arm = 0; arm < 2; ++arm) {
            float tx, ty;
            const float side = arm == 0 ? +1.0f : -1.0f;
            double_ribbon_point(u, t_limit, centre, scale, side, &tx, &ty);
            double_ribbon_world_to_screen(frame, tx, ty,
                                           &next_x[arm], &next_y[arm]);
            next_w[arm] = half_width * radius_factor;
        }
        /* Change the foreground strand at each half-turn, creating a
         * genuine over/under weave at intersections rather than one colour
         * always obscuring the other. */
        const int half_turn = (int)floorf(6.0f * t_limit * u);
        const int first = (half_turn & 1) ? 0 : 1;
        for (int pass = 0; pass < 2; ++pass) {
            const int arm = pass == 0 ? first : 1-first;
            ribbon_capsule(frame, prev_x[arm], prev_y[arm], prev_w[arm],
                           next_x[arm], next_y[arm], next_w[arm],
                           arm == 0 ? 2 : 3);
        }
        for (int arm = 0; arm < 2; ++arm) {
            prev_x[arm] = next_x[arm];
            prev_y[arm] = next_y[arm];
            prev_w[arm] = next_w[arm];
        }
    }
}

/* Rope-like deformation: a straight segment is pinched at the middle and
 * twisted, producing two mirrored semi-spirals around the pinch. Each colour
 * is offset from the same rope centreline using the local normal, so the
 * result reads as one twisted rope rather than two unrelated spirals. */
static void pinched_rope_centre_point(float q, float twist, float centre,
                                      float half_span, float amplitude,
                                      float direction, float *x, float *y) {
    const float advance = q * half_span;
    const float envelope = twist * q * powf(fmaxf(0.0f, 1.0f - q), 1.10f);
    const float radius = amplitude * 0.82f * envelope;
    const float angle = direction * (1.05f * PI + 5.1f * PI * twist * q);
    *x = centre + direction * advance + direction * radius * cosf(angle);
    *y = radius * sinf(angle);
}

static void pinched_rope_world_to_screen(const ProtoFrame *frame,
                                         float tx, float ty,
                                         float *sx, float *sy) {
    *sx = PROTO_WIDTH * 0.5f + tx * frame->cos_angle - ty * frame->sin_angle;
    *sy = PROTO_HEIGHT * 0.5f + tx * frame->sin_angle + ty * frame->cos_angle;
}

static void pinched_rope_cell(ProtoFrame *frame, float cell_start,
                              float cell_span, float amplitude,
                              float ribbon_width, float t_limit) {
    if (t_limit <= 0.00001f) return;
    const float centre = cell_start + 0.5f * cell_span;
    const float half_span = 0.5f * cell_span;
    const float scale = fminf(fmaxf(amplitude, 0.0f), 0.24f * cell_span);
    if (scale <= 0.0f) return;
    const float fade = t_limit <= 0.045f ?
        (t_limit / 0.045f) * (t_limit / 0.045f) * (3.0f - 2.0f * (t_limit / 0.045f)) : 1.0f;
    const float base_width = 0.5f * ribbon_width * fade;
    const int segments = 72;

    for (int side = -1; side <= 1; side += 2) {
        float prev_cx, prev_cy, prev_tx, prev_ty;
        pinched_rope_centre_point(0.0f, t_limit, centre, half_span, scale,
                                  (float)side, &prev_cx, &prev_cy);
        pinched_rope_centre_point(1.0f/segments, t_limit, centre, half_span, scale,
                                  (float)side, &prev_tx, &prev_ty);
        float prev_dx = prev_tx - prev_cx;
        float prev_dy = prev_ty - prev_cy;
        float prev_len = hypotf(prev_dx, prev_dy);
        if (prev_len < 1e-5f) { prev_dx = (float)side; prev_dy = 0.0f; prev_len = 1.0f; }
        float prev_nx = -prev_dy / prev_len;
        float prev_ny =  prev_dx / prev_len;
        float prev_sep = base_width * (0.30f + 0.60f * (1.0f - 0.0f));
        float prev_hw  = base_width * (0.35f + 0.65f * (1.0f - 0.0f));

        float prev_ax, prev_ay, prev_bx, prev_by;
        pinched_rope_world_to_screen(frame,
                                     prev_cx + prev_nx * prev_sep,
                                     prev_cy + prev_ny * prev_sep,
                                     &prev_ax, &prev_ay);
        pinched_rope_world_to_screen(frame,
                                     prev_cx - prev_nx * prev_sep,
                                     prev_cy - prev_ny * prev_sep,
                                     &prev_bx, &prev_by);

        for (int k = 1; k <= segments; ++k) {
            const float q = (float)k / (float)segments;
            float cx, cy;
            pinched_rope_centre_point(q, t_limit, centre, half_span, scale,
                                      (float)side, &cx, &cy);
            float fx, fy;
            if (k < segments) {
                pinched_rope_centre_point((float)(k+1) / (float)segments, t_limit,
                                          centre, half_span, scale, (float)side,
                                          &fx, &fy);
            } else {
                fx = cx + (float)side;
                fy = cy;
            }
            float dx = fx - cx;
            float dy = fy - cy;
            float len = hypotf(dx, dy);
            if (len < 1e-5f) { dx = (float)side; dy = 0.0f; len = 1.0f; }
            float nx = -dy / len;
            float ny =  dx / len;

            const float edge_falloff = powf(fmaxf(0.0f, 1.0f - q), 0.42f);
            const float sep = base_width * (0.30f + 0.60f * edge_falloff);
            const float hw  = base_width * (0.35f + 0.65f * edge_falloff);

            float ax, ay, bx, by;
            pinched_rope_world_to_screen(frame, cx + nx * sep, cy + ny * sep, &ax, &ay);
            pinched_rope_world_to_screen(frame, cx - nx * sep, cy - ny * sep, &bx, &by);

            const int weave = (int)floorf(4.0f * t_limit * q * 4.0f);
            if (weave & 1) {
                ribbon_capsule(frame, prev_bx, prev_by, prev_hw, bx, by, hw, 3);
                ribbon_capsule(frame, prev_ax, prev_ay, prev_hw, ax, ay, hw, 2);
            } else {
                ribbon_capsule(frame, prev_ax, prev_ay, prev_hw, ax, ay, hw, 2);
                ribbon_capsule(frame, prev_bx, prev_by, prev_hw, bx, by, hw, 3);
            }
            prev_ax = ax; prev_ay = ay;
            prev_bx = bx; prev_by = by;
            prev_hw = hw;
        }
    }
}

void proto_wolfram_ribbon_prepare(ProtoFrame *frame,
                                   const ProtoConfig *config,
                                   const ProtoAnimation *animation) {
    memset(frame->ribbon_mask, 0, sizeof frame->ribbon_mask);
    const int count = config->section_count;
    const float visible = PROTO_WIDTH * 0.5f * fabsf(frame->cos_angle) +
                          PROTO_HEIGHT * 0.5f * fabsf(frame->sin_angle);
    for (int side = 0; side < count; ++side) {
        const ProtoSection *section = &config->sections[side];
        if (section->formula != PROTO_WOLFRAM_RIBBON &&
            section->formula != PROTO_WOLFRAM_DOUBLE_RIBBON &&
            section->formula != PROTO_PINCHED_ROPE) continue;
        const float section_span = 2.0f*RANGE / count;
        const float section_start = -RANGE + side*section_span;
        float start = section_start, span = section_span;
        if (count == 2 && section->repeat_visible) {
            start = side == 0 ? -visible : 0.0f;
            span = visible;
        }
        int cells = (int)lroundf(section->cycles);
        if (cells < 1) cells = 1;
        if (cells > 16) cells = 16;
        const float cell = span / cells;
        const float gap = fminf(fmaxf(0.0f, section->gap_pixels), 0.65f * cell);
        const float width = section->ribbon_width > 0.0f ?
            section->ribbon_width : 7.0f;
        const float t_limit = section->phase_speed == 0.0f ? 0.5f :
            0.25f*(1.0f-cosf(2.0f*PI*section->phase_speed*animation->time));
        for (int i = 0; i < cells; ++i) {
            const float cell_start = start+i*cell+0.5f*gap;
            if (section->formula == PROTO_WOLFRAM_DOUBLE_RIBBON)
                double_ribbon_cell(frame, cell_start, cell-gap,
                                   section->amplitude, width, t_limit);
            else if (section->formula == PROTO_PINCHED_ROPE)
                pinched_rope_cell(frame, cell_start, cell-gap,
                                  section->amplitude, width, t_limit);
            else
                ribbon_cell(frame, cell_start, cell-gap,
                            section->amplitude, width, t_limit);
        }
    }
}

/* True two-arm Fermat spiral: polar r = +/- a*sqrt(theta).
 * The growing theta_max simultaneously increases turns and outside radius.
 * Rotating by -theta_max fixes both outer endpoints on the baseline: the
 * complete path left edge -> negative arm -> centre -> positive arm ->
 * right edge is continuous. No ribbons or white strokes are involved. */
/* Patch 0021: two separate smoothstep position curves per cycle.
 * Their velocity, 6u(1-u), is parabolic: slow -> fast -> slow on BOTH halves.
 * With phase_speed == 0, show the fully extended shape for inspection. */
float proto_fermat_parabolic_progress(float time, float phase_speed) {
    if (phase_speed == 0.0f) return 1.0f;
    float phase = fmodf(time * phase_speed, 1.0f);
    if (phase < 0.0f) phase += 1.0f;
    const float u = phase < 0.5f ? 2.0f * phase : 2.0f * (1.0f - phase);
    return u * u * (3.0f - 2.0f * u);
}

void proto_fermat_spiral_point(float p, float cell_start, float cell_span,
                               float amplitude, float progress,
                               float *x, float *y) {
    p = fmaxf(0.0f, fminf(1.0f, p));
    progress = fmaxf(0.0f, fminf(1.0f, progress));
    const float centre = cell_start + 0.5f * cell_span;
    const float max_radius = fminf(fmaxf(0.0f, amplitude), 0.49f * cell_span);
    const float outer_radius = max_radius * powf(progress, 0.58f);
    const float theta_max = 7.0f * PI * powf(progress, 1.18f);
    const float lead = 0.12f, trail = 0.88f;

    *y = 0.0f;
    if (progress <= 0.0001f || theta_max <= 0.0001f || outer_radius <= 0.0001f) {
        *x = cell_start + p * cell_span;
        return;
    }

    if (p <= lead) {
        *x = cell_start + (centre - outer_radius - cell_start) * (p / lead);
        return;
    }
    if (p >= trail) {
        const float u = (p - trail) / (1.0f - trail);
        *x = centre + outer_radius +
             (cell_start + cell_span - centre - outer_radius) * u;
        return;
    }

    float arm_u, side;
    if (p <= 0.5f) {
        arm_u = (p - lead) / (0.5f - lead);      /* 0 at base, 1 at tip */
        side = -1.0f;
    } else {
        arm_u = (trail - p) / (trail - 0.5f);    /* 0 at base, 1 at tip */
        side = 1.0f;
    }
    arm_u = fmaxf(0.0f, fminf(1.0f, arm_u));

    /* Broad near the base, strongly tapered near the tip. */
    const float radius = outer_radius * powf(1.0f - arm_u, 0.58f);

    /* Curl is concentrated near the tip to avoid a teardrop silhouette. */
    /* Quintic shoulder blends the flat connector into the coil:
     * curvature starts at zero rather than turning abruptly. */
    const float shoulder_t = fminf(1.0f, arm_u / 0.22f);
    const float shoulder = shoulder_t * shoulder_t * shoulder_t *
        (shoulder_t * (6.0f * shoulder_t - 15.0f) + 10.0f);
    const float turn_mix = powf(arm_u, 2.35f) * shoulder;
    const float phase = -theta_max * turn_mix;

    *x = centre + side * radius * cosf(phase);
    *y = side * radius * sinf(phase);
}


/* Patch 0022: make the motion noticeably faster in the middle of EACH
 * expansion and retraction half. The velocity envelope is the cube of the
 * normalized parabola 4u(1-u), rather than a constant-rate or sine ease.
 * Integrated exact position: 35u^4 - 84u^5 + 70u^6 - 20u^7. */
float proto_fermat_twist_progress(float time, float phase_speed) {
    if (phase_speed == 0.0f) return 1.0f;
    const double cycles = (double)time * fabs((double)phase_speed);
    double phase = cycles - floor(cycles);
    if (phase < 0.0) phase += 1.0;
    const double u = phase <= 0.5 ? 2.0 * phase : 2.0 * (1.0 - phase);
    const double u2 = u * u;
    const double u4 = u2 * u2;
    return (float)(u4 * (35.0 + u * (-84.0 + u * (70.0 - 20.0 * u))));
}

/* The full 0->100->0 loop (5 s in the supplied profile) keeps one direction.
 * The NEXT loop is mirrored. Switch only when the shape is fully flat. */
float proto_fermat_twist_direction(float time, float phase_speed) {
    if (phase_speed == 0.0f) return 1.0f;
    const double cycle = floor((double)time * fabs((double)phase_speed));
    return fmod(cycle, 2.0) == 0.0 ? 1.0f : -1.0f;
}

/* Same softened Fermat-tentacle outline as patch 0021, but with separate
 * independent controls for physical growth and winding development.
 *  - progress: physical base grows identically for ALL FOUR cells
 *  - winding: .25 / .50 / .75 / 1.00 of the total tip turns
 *  - direction: +/- to mirror the rotation on alternating cycles
 * This is a continuous two-arm dividing curve, not an overlaid tube. */
void proto_fermat_twist_point(float p, float cell_start, float cell_span,
                              float amplitude, float progress, float winding,
                              float direction, float *x, float *y) {
    p = fmaxf(0.0f, fminf(1.0f, p));
    progress = fmaxf(0.0f, fminf(1.0f, progress));
    winding = fmaxf(0.0f, fminf(1.0f, winding));
    const float centre = cell_start + 0.5f * cell_span;
    const float max_radius = fminf(fmaxf(amplitude, 0.0f), 0.49f * cell_span);
    const float outer_radius = max_radius * powf(progress, 0.58f);
    const float theta_max = 7.0f * PI * powf(progress, 1.18f) * winding;
    const float lead = 0.12f, trail = 0.88f;
    *y = 0.0f;

    if (progress <= 0.0001f || theta_max <= 0.0001f || outer_radius <= 0.0001f) {
        *x = cell_start + p * cell_span;
        return;
    }
    if (p <= lead) {
        *x = cell_start + (centre - outer_radius - cell_start) * (p / lead);
        return;
    }
    if (p >= trail) {
        const float u = (p-trail)/(1.0f-trail);
        *x = centre + outer_radius +
             (cell_start + cell_span - centre - outer_radius) * u;
        return;
    }

    float arm_u, side;
    if (p <= 0.5f) {
        arm_u = (p-lead)/(0.5f-lead);
        side = -1.0f;
    } else {
        arm_u = (trail-p)/(trail-0.5f);
        side = 1.0f;
    }
    arm_u = fmaxf(0.0f, fminf(1.0f, arm_u));
    const float radius = outer_radius * powf(1.0f-arm_u, 0.58f);
    const float shoulder_t = fminf(1.0f, arm_u/0.22f);
    const float shoulder = shoulder_t * shoulder_t * shoulder_t *
        (shoulder_t*(6.0f*shoulder_t-15.0f)+10.0f);
    const float tip_turn = powf(arm_u, 2.35f) * shoulder;
    const float angle = -(direction < 0.0f ? -1.0f : 1.0f) * theta_max * tip_turn;

    *x = centre + side * radius * cosf(angle);
    *y = side * radius * sinf(angle);
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
