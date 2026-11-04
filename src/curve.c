/* SPDX-License-Identifier: MIT */
#include "curve.h"
#include "expression.h"
#include "parametric.h"

#include <math.h>
#include <string.h>

#define PROTO_PI 3.14159265358979323846f
#define PROTO_HALF_SPAN 168.0f
#define PROTO_MORPH_SECONDS 0.55f
/* Double-armed Archimedean spiral separator. */
#define PROTO_SPLIT_SPAN (2.0f * PROTO_HALF_SPAN)

static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float fractf_pos(float v) { return v - floorf(v); }
static float smoothstep(float v) { v = clamp01(v); return v * v * (3.0f - 2.0f * v); }
static float apply_easing(float t, ProtoEasing easing) {
    t = clamp01(t);
    switch (easing) {
        case PROTO_EASE_LINEAR: return t;
        case PROTO_EASE_SMOOTHSTEP: return t * t * (3.0f - 2.0f * t);
        case PROTO_EASE_QUAD_IN: return t * t;
        case PROTO_EASE_QUAD_OUT: return t * (2.0f - t);
        case PROTO_EASE_QUAD_IN_OUT: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case PROTO_EASE_CUBIC_IN: return t * t * t;
        case PROTO_EASE_CUBIC_OUT: { float f = t - 1.0f; return f * f * f + 1.0f; }
        case PROTO_EASE_CUBIC_IN_OUT: return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        default: return t;
    }
}

bool proto_valid_section_count(int count) {
    return count == 1 || count == 2 || count == 4 || count == 8 || count == 16;
}

const char *proto_formula_name(ProtoFormula formula) {
    static const char *names[] = {"straight", "sine", "lightning", "fractal", "fourier",
                                   "chirp", "trochoid", "harmonic_fractal", "spiral", "triangle", "custom"};
    return formula >= 0 && formula < PROTO_FORMULA_COUNT ? names[formula] : "unknown";
}

bool proto_parse_formula(const char *name, ProtoFormula *out) {
    if (!name || !out) return false;
    for (int k = PROTO_STRAIGHT; k < PROTO_CUSTOM; ++k) {
        if (!strcmp(name, proto_formula_name((ProtoFormula)k))) {
            *out = (ProtoFormula)k;
            return true;
        }
    }
    return false;
}

void proto_config_defaults(ProtoConfig *config) {
    memset(config, 0, sizeof *config);
    config->section_count = 4;
    config->rotation_speed = 10.0f;
    config->line_width = 1.0f;
    config->show_divider = true;
    memcpy(config->violet, (unsigned char[]){135, 55, 205, 255}, 4);
    memcpy(config->cyan, (unsigned char[]){25, 219, 232, 255}, 4);
    memcpy(config->divider_color, (unsigned char[]){248, 248, 253, 255}, 4);
    const ProtoFormula patterns[] = {PROTO_FOURIER, PROTO_CHIRP, PROTO_TROCHOID, PROTO_HARMONIC_FRACTAL,
                                     PROTO_LIGHTNING, PROTO_SINE, PROTO_FRACTAL, PROTO_STRAIGHT};
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
        config->sections[i] = (ProtoSection){.formula = patterns[i % 8],
                                             .amplitude = 14.0f,
                                             .cycles = 3.0f,
                                             .phase_speed = 0.14f,
                                             .animated = false,
                                             .loop = false,
                                             .transition_count = 0};
    }
}

void proto_animation_init(ProtoAnimation *animation, const ProtoConfig *config) {
    memset(animation, 0, sizeof *animation);
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
        animation->previous[i] = config->sections[i];
        animation->morph[i] = 1.0f;
        animation->morph_seconds[i] = PROTO_MORPH_SECONDS;
        animation->active_easing[i] = PROTO_EASE_SMOOTHSTEP;
        animation->current_transition[i] = 0;
        animation->transition_time[i] = 0.0f;
        animation->current_formula[i] = config->sections[i].formula;
        strncpy(animation->current_expression[i], config->sections[i].expression, PROTO_EXPRESSION_LENGTH);
        animation->current_expression[i][PROTO_EXPRESSION_LENGTH - 1] = '\0';
    }
}
void proto_animation_tick(ProtoAnimation *animation, const ProtoConfig *config, float dt) {
    if (animation->paused || !isfinite(dt) || dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;
    animation->accumulator += dt;
    while (animation->accumulator >= PROTO_FIXED_TIMESTEP) {
        animation->time += PROTO_FIXED_TIMESTEP;
        animation->rotation_degrees = fmodf(animation->rotation_degrees +
                                            config->rotation_speed * PROTO_FIXED_TIMESTEP, 360.0f);
        for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
            float seconds = animation->morph_seconds[i];
            if (!(seconds > 0.0f)) seconds = PROTO_MORPH_SECONDS;
            animation->morph[i] = clamp01(animation->morph[i] + PROTO_FIXED_TIMESTEP / seconds);
            const ProtoSection *s = &config->sections[i];
            if (!s->animated || s->transition_count == 0 || animation->finished[i] ||
                animation->morph[i] < 1.0f) continue;
            animation->transition_time[i] += PROTO_FIXED_TIMESTEP;
            const ProtoTransition *trans = &s->transitions[animation->current_transition[i]];
            if (animation->transition_time[i] + 1.0e-5f < trans->duration) continue;
            animation->transition_time[i] = 0.0f;
            bool changed = animation->current_formula[i] != trans->formula ||
                (trans->formula == PROTO_CUSTOM &&
                 strcmp(animation->current_expression[i], trans->expression) != 0);
            if (changed) {
                ProtoSection prior = *s;
                prior.formula = animation->current_formula[i];
                memcpy(prior.expression, animation->current_expression[i], sizeof prior.expression);
                animation->previous[i] = prior;
                animation->current_formula[i] = trans->formula;
                memcpy(animation->current_expression[i], trans->expression,
                       sizeof animation->current_expression[i]);
                animation->active_easing[i] = trans->easing;
                animation->morph_seconds[i] = trans->morph_duration > 0.0f ?
                    trans->morph_duration : PROTO_MORPH_SECONDS;
                animation->morph[i] = 0.0f;
            }
            ++animation->current_transition[i];
            if (animation->current_transition[i] >= s->transition_count) {
                if (s->loop) animation->current_transition[i] = 0;
                else {
                    animation->finished[i] = true;
                    animation->current_transition[i] = s->transition_count - 1;
                }
            }
        }
        animation->accumulator -= PROTO_FIXED_TIMESTEP;
    }
}

void proto_animation_change_formula(ProtoAnimation *animation, ProtoConfig *config,
                                    int index, ProtoFormula formula) {
    if (index < 0 || index >= PROTO_MAX_SECTIONS || formula < 0 ||
        formula >= PROTO_CUSTOM || config->sections[index].formula == formula) return;
    animation->previous[index] = config->sections[index];
    animation->morph[index] = 0.0f;
    config->sections[index].formula = formula;
}

static float formula_value(const ProtoSection *s, float u, float time) {
    if (s->formula == PROTO_STRAIGHT) return 0.0f;
    float edge = sinf(PROTO_PI * u);
    edge *= edge; /* Each segment returns to baseline at its endpoints. */
    float phase = time * s->phase_speed;
    float a = s->cycles * u;
    float v = 0.0f;
    switch (s->formula) {
    case PROTO_SINE:
        v = sinf(2.0f * PROTO_PI * (a + phase));
        break;
    case PROTO_LIGHTNING:
        v = 1.0f - 4.0f * fabsf(fractf_pos(a + phase + 0.25f) - 0.5f);
        /* Stationary integer-cycle lightning has uniform teeth and joins at zero. */
        if (s->phase_speed == 0.0f && fabsf(s->cycles - roundf(s->cycles)) < 1e-5f)
            return s->amplitude * v;
        break;
    case PROTO_FRACTAL: {
        float sum = 0.0f, weight = 1.0f, total = 0.0f, multiplier = 1.0f;
        for (int i = 0; i < 4; ++i) {
            sum += weight * sinf(2.0f * PROTO_PI *
                                  (a * multiplier + phase * (1.0f + 0.17f * i)));
            total += weight;
            weight *= 0.50f;
            multiplier *= 2.0f;
        }
        v = sum / total;
        break;
    }
    case PROTO_FOURIER: {
        /* Superposition of sine waves with harmonic frequencies */
        float sum = 0.0f;
        sum += 1.0f * sinf(2.0f * PROTO_PI * (a + phase));
        sum += 0.5f * sinf(2.0f * PROTO_PI * (2.0f * a + 1.5f * phase));
        sum += 0.25f * sinf(2.0f * PROTO_PI * (3.0f * a + 2.0f * phase));
        sum += 0.125f * sinf(2.0f * PROTO_PI * (4.0f * a + 2.5f * phase));
        v = sum / 1.875f; /* Normalize by sum of weights */
        break;
    }
    case PROTO_CHIRP: {
        /* Frequency varies linearly with position: f(u) = f0 + k*u */
        float f0 = 1.0f;
        float k = 2.0f;
        /* Integral of frequency over position gives phase */
        float chirp_phase = 2.0f * PROTO_PI * (f0 * u + 0.5f * k * u * u + phase);
        v = sinf(chirp_phase);
        break;
    }
    case PROTO_TROCHOID: {
        /* Cycloid-like curve: creates loops when amplitude > radius */
        float R = 1.0f; /* Rolling circle radius */
        float r = 0.5f; /* Point offset from center */
        float theta = 2.0f * PROTO_PI * (a + phase);
        /* Trochoid: x = R*theta - r*sin(theta), y = R - r*cos(theta) */
        /* We only use the y component for vertical displacement */
        v = (R - r * cosf(theta)) / (R + r); /* Normalize to [-1, 1] */
        break;
    }
    case PROTO_HARMONIC_FRACTAL: {
        /* More controlled harmonic series with finer detail */
        float sum = 0.0f, weight = 1.0f, total = 0.0f;
        for (int i = 0; i < 6; ++i) {
            float freq = (float)(i + 1);
            float phase_offset = phase * (1.0f + 0.2f * i);
            sum += weight * sinf(2.0f * PROTO_PI * (a * freq + phase_offset));
            total += weight;
            weight *= 0.6f; /* Slower decay for more detail */
        }
        v = sum / total;
        break;
    }
    case PROTO_TRIANGLE:
        v = 0.38f * (1.0f - 4.0f * fabsf(fractf_pos(a + 0.25f) - 0.5f));
        break;
    case PROTO_SPIRAL:
        return 0.0f; /* Handled by parametric x(u),y(u), not y=f(x). */
    case PROTO_CUSTOM: {
        bool ok = false;
        v = proto_expression_evaluate(s->expression, u, time, s->amplitude,
                                      s->cycles, s->phase_speed, &ok);
        if (!ok) v = 0.0f;
        break;
    }
    default: break;
    }
    if (!isfinite(v)) return 0.0f;
    /* Limit extremely large formulas to a predictable screen-space radius. */
    if (v < -1.0f) v = -1.0f;
    if (v > 1.0f) v = 1.0f;
    return s->amplitude * edge * v;
}

/* Periodic motifs are repeated along the visible half-line. A flat gap
 * occupies the requested number of logical pixels between successive cells.
 * Keeping motif endpoints on y=0 preserves the region boundary. */
static float triangle_unit(float p) {
    return 1.0f - 4.0f * fabsf(fractf_pos(p + 0.25f) - 0.5f);
}

/* Deterministic, angular three-scale lightning: large hooked stroke first,
 * two nested zigzags second/third.  Unlike a triangular wave it contains
 * several sharp, asymmetric changes of direction inside each motif. */
static float lightning_unit(float p) {
    const float at[] = {0.0f, 0.11f, 0.22f, 0.33f, 0.43f,
                        0.55f, 0.65f, 0.76f, 0.88f, 1.0f};
    const float h[] = {0.0f, -0.18f, 0.96f, 0.12f, 0.38f,
                       -1.00f, -0.10f, 0.42f, -0.22f, 0.0f};
    float large = 0.0f;
    for (int i = 0; i < 9; ++i) {
        if (p <= at[i + 1]) {
            float q = (p - at[i]) / (at[i + 1] - at[i]);
            large = h[i] + (h[i + 1] - h[i]) * q;
            break;
        }
    }
    return 0.86f * large + 0.10f * triangle_unit(3.0f * p) +
           0.04f * triangle_unit(9.0f * p);
}

static void shape_point(const ProtoSection *s, float baseline, float u,
                        float start, float span, float t, float curl,
                        float *x, float *y) {
    *x = baseline;
    *y = 0.0f;
    if (u < 0.0f || u > 1.0f) return;
    bool repeated = s->formula == PROTO_SPIRAL ||
        ((s->repeat_visible || s->gap_pixels > 0.0f) &&
         (s->formula == PROTO_SINE || s->formula == PROTO_LIGHTNING ||
          s->formula == PROTO_TRIANGLE));
    if (!repeated) {
        *y = formula_value(s, u, t);
        return;
    }
    int n = (int)lroundf(s->cycles);
    if (n < 1) n = 1;
    if (n > 16) n = 16;
    float cell = span / n;
    float gap = fminf(s->gap_pixels, cell * 0.65f);
    float scaled = u * n;
    int index = (int)scaled;
    if (index >= n) index = n - 1;
    float local = u * span - index * cell;
    float start_local = gap * 0.5f;
    float length = cell - gap;
    if (local <= start_local || local >= start_local + length) return;
    float p = (local - start_local) / length;
    float cell_start = start + index * cell + start_local;
    if (s->formula == PROTO_SPIRAL) {
        proto_spiral_point(p, cell_start, length, s->amplitude, curl, x, y);
        return;
    }
    float wave;
    switch (s->formula) {
    case PROTO_LIGHTNING:
        wave = lightning_unit(p);
        break;
    case PROTO_TRIANGLE:
        /* One compact, single-sided triangular tooth per cell. */
        wave = 0.42f * (1.0f - fabsf(2.0f * p - 1.0f));
        break;

    case PROTO_SINE:
        wave = sinf(2.0f * PROTO_PI * (p + s->phase_speed * t));
        break;
    default:
        return;
    }
    /* Fade only within the last tenth of a motif: exact flat junctions. */
    float edge = smoothstep(fminf(p * 10.0f, (1.0f - p) * 10.0f));
    *y = s->amplitude * wave * edge;
}
void proto_make_frame(const ProtoConfig *config, const ProtoAnimation *animation,
                      ProtoFrame *frame) {
    const float radians = animation->rotation_degrees * (PROTO_PI / 180.0f);
    frame->cos_angle = cosf(radians);
    frame->sin_angle = sinf(radians);
    frame->parametric = false;
    const int count = config->section_count;
    /* Projected half-extent of the rotated 240x160 rectangle. With
     * repeat_visible enabled, eight motifs are spread over the actual
     * on-screen half-line rather than over its invisible off-screen tail. */
    const float visible = PROTO_WIDTH * 0.5f * fabsf(frame->cos_angle) +
                          PROTO_HEIGHT * 0.5f * fabsf(frame->sin_angle);
    for (int i = 0; i < PROTO_PROFILE_SAMPLES; ++i) {
        float along = (float)i / (PROTO_PROFILE_SAMPLES - 1) * count;
        int index = (int)along;
        if (index >= count) index = count - 1;
        const float local_u = along - index;
        const float section_span = PROTO_SPLIT_SPAN / count;
        const float section_start = -PROTO_HALF_SPAN + index * section_span;
        const float base = section_start + local_u * section_span;
        const ProtoSection *section = &config->sections[index];
        float start = section_start, span = section_span;
        if (count == 2 && section->repeat_visible) {
            start = index == 0 ? -visible : 0.0f;
            span = visible;
        }
        const float u = (base - start) / span;
        ProtoSection next = *section;
        if (section->animated) {
            next.formula = animation->current_formula[index];
            if (next.formula == PROTO_CUSTOM)
                memcpy(next.expression, animation->current_expression[index],
                       sizeof next.expression);
        }
        const ProtoSection *previous = &animation->previous[index];
        float blend = smoothstep(animation->morph[index]);
        if (section->animated && section->transition_count > 0)
            blend = apply_easing(animation->morph[index], animation->active_easing[index]);
        float xn, yn, xp, yp;
        shape_point(&next, base, u, start, span, animation->time,
                    next.formula == PROTO_SPIRAL ? blend : 1.0f, &xn, &yn);
        shape_point(previous, base, u, start, span, animation->time,
                    previous->formula == PROTO_SPIRAL ? 1.0f - blend : 1.0f, &xp, &yp);
        /* Spiral curl is already driven by morph. Applying the blend a
         * second time would square its radius and hide the actual coil. */
        if (next.formula == PROTO_SPIRAL && previous->formula != PROTO_SPIRAL) {
            frame->x_coords[i] = xn + (xp - base) * (1.0f - blend);
            frame->offsets[i] = yn + yp * (1.0f - blend);
        } else if (previous->formula == PROTO_SPIRAL && next.formula != PROTO_SPIRAL) {
            frame->x_coords[i] = xp + (xn - base) * blend;
            frame->offsets[i] = yp + yn * blend;
        } else {
            frame->x_coords[i] = xp + (xn - xp) * blend;
            frame->offsets[i] = yp + (yn - yp) * blend;
        }

        if ((next.formula == PROTO_SPIRAL && blend > 0.0f) ||
            (previous->formula == PROTO_SPIRAL && blend < 1.0f))
            frame->parametric = true;
    }
    if (frame->parametric) proto_parametric_prepare(frame);
}

float proto_signed_side(const ProtoFrame *frame, float x, float y) {
    const float dx = x - PROTO_WIDTH * 0.5f;
    const float dy = y - PROTO_HEIGHT * 0.5f;
    const float tangent = dx * frame->cos_angle + dy * frame->sin_angle;
    const float normal = -dx * frame->sin_angle + dy * frame->cos_angle;
    if (frame->parametric) return proto_parametric_side(frame, tangent, normal);
    float position = clamp01((tangent + PROTO_HALF_SPAN) / (2.0f * PROTO_HALF_SPAN));
    float sample = position * (PROTO_PROFILE_SAMPLES - 1);
    int index = (int)sample;
    if (index >= PROTO_PROFILE_SAMPLES - 1) index = PROTO_PROFILE_SAMPLES - 2;
    float blend = sample - index;
    float offset = frame->offsets[index] * (1.0f - blend) +
                   frame->offsets[index + 1] * blend;
    return normal - offset;
}
