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
                                   "chirp", "trochoid", "harmonic_fractal", "spiral", "custom"};
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
            animation->morph[i] = clamp01(animation->morph[i] + PROTO_FIXED_TIMESTEP / PROTO_MORPH_SECONDS);
            /* Each transition entry is the NEXT target, not the current one.
             * Execute entry zero first and never repeat the final non-loop step. */
            const ProtoSection *s = &config->sections[i];
            if (s->animated && s->transition_count > 0 && !animation->finished[i] &&
                animation->morph[i] >= 1.0f) {
                animation->transition_time[i] += PROTO_FIXED_TIMESTEP;
                const ProtoTransition *trans = &s->transitions[animation->current_transition[i]];
                if (animation->transition_time[i] + 1.0e-5f >= trans->duration) {
                    animation->transition_time[i] = 0.0f;
                    bool changed = animation->current_formula[i] != trans->formula ||
                       (trans->formula == PROTO_CUSTOM &&
                        strcmp(animation->current_expression[i], trans->expression));
                    if (changed) {
                        ProtoSection prior = *s;
                        prior.formula = animation->current_formula[i];
                        memcpy(prior.expression, animation->current_expression[i],
                               sizeof prior.expression);
                        animation->previous[i] = prior;
                        animation->current_formula[i] = trans->formula;
                        memcpy(animation->current_expression[i], trans->expression,
                               sizeof trans->expression);
                        animation->active_easing[i] = trans->easing;
                        animation->morph[i] = 0.0f;
                    }
                    ++animation->current_transition[i];
                    if (animation->current_transition[i] >= s->transition_count) {
                        if (s->loop) animation->current_transition[i] = 0;
                        else {
                            animation->finished[i] = true;
                            animation->current_transition[i] = s->transition_count-1;
                        }
                    }
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

void proto_make_frame(const ProtoConfig *config, const ProtoAnimation *animation,
                      ProtoFrame *frame) {
    const float radians = animation->rotation_degrees * (PROTO_PI / 180.0f);
    frame->cos_angle = cosf(radians);
    frame->sin_angle = sinf(radians);
    frame->parametric = false;
    const int count = config->section_count;
    for (int i = 0; i < PROTO_PROFILE_SAMPLES; ++i) {
        float along = (float)i / (PROTO_PROFILE_SAMPLES - 1) * count;
        int index = (int)along;
        if (index >= count) index = count - 1;
        float u = along - index;
        const float span = PROTO_SPLIT_SPAN / count;
        const float start = -PROTO_HALF_SPAN + index * span;
        const float base = start + u * span;
        const ProtoSection *section = &config->sections[index];
        ProtoSection next = *section;
        if (section->animated) {
            next.formula = animation->current_formula[index];
            if (next.formula == PROTO_CUSTOM) {
                memcpy(next.expression, animation->current_expression[index],
                       sizeof next.expression);
            }
        }
        const ProtoSection *previous = &animation->previous[index];
        float xn=base, yn=0.0f, xp=base, yp=0.0f;
        if (next.formula == PROTO_SPIRAL)
            proto_spiral_point(u, start, span, next.amplitude, &xn, &yn);
        else yn = formula_value(&next, u, animation->time);
        if (previous->formula == PROTO_SPIRAL)
            proto_spiral_point(u, start, span, previous->amplitude, &xp, &yp);
        else yp = formula_value(previous, u, animation->time);
        float blend = smoothstep(animation->morph[index]);
        if (section->animated && section->transition_count > 0)
            blend = apply_easing(animation->morph[index], animation->active_easing[index]);
        frame->x_coords[i] = xp + (xn-xp)*blend;
        frame->offsets[i] = yp + (yn-yp)*blend;
        if (next.formula == PROTO_SPIRAL ||
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
