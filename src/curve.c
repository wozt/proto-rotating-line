/* SPDX-License-Identifier: MIT */
#include "curve.h"

#include <math.h>
#include <string.h>

#define PROTO_PI 3.14159265358979323846f
/* Longer than half the 240x160 diagonal: the graph always crosses the viewport. */
#define PROTO_HALF_SPAN 168.0f
#define PROTO_MORPH_SECONDS 0.55f

static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float fractf_pos(float v) { return v - floorf(v); }
static float smoothstep(float v) { v = clamp01(v); return v * v * (3.0f - 2.0f * v); }

const char *proto_formula_name(ProtoFormula formula) {
    static const char *names[] = {"straight", "sine", "lightning", "fractal"};
    return formula >= 0 && formula < PROTO_FORMULA_COUNT ? names[formula] : "unknown";
}

bool proto_parse_formula(const char *name, ProtoFormula *out) {
    if (!name || !out) return false;
    for (int k = 0; k < PROTO_FORMULA_COUNT; ++k) {
        if (strcmp(name, proto_formula_name((ProtoFormula)k)) == 0) {
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
    const ProtoFormula patterns[PROTO_MAX_SECTIONS] = {
        PROTO_LIGHTNING, PROTO_SINE, PROTO_FRACTAL, PROTO_STRAIGHT,
        PROTO_LIGHTNING, PROTO_SINE, PROTO_FRACTAL, PROTO_STRAIGHT
    };
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
        config->sections[i] = (ProtoSection){patterns[i], 14.0f, 3.0f, 0.14f};
    }
}

void proto_animation_init(ProtoAnimation *animation, const ProtoConfig *config) {
    memset(animation, 0, sizeof *animation);
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
        animation->previous[i] = config->sections[i].formula;
        animation->morph[i] = 1.0f;
    }
}

void proto_animation_tick(ProtoAnimation *animation, const ProtoConfig *config, float dt) {
    if (animation->paused || !isfinite(dt) || dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f; /* Avoid a huge step after a suspended window. */
    animation->time += dt;
    animation->rotation_degrees = fmodf(animation->rotation_degrees +
                                        config->rotation_speed * dt, 360.0f);
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i) {
        animation->morph[i] = clamp01(animation->morph[i] + dt / PROTO_MORPH_SECONDS);
    }
}

void proto_animation_change_formula(ProtoAnimation *animation, ProtoConfig *config,
                                    int index, ProtoFormula formula) {
    if (index < 0 || index >= PROTO_MAX_SECTIONS ||
        formula < 0 || formula >= PROTO_FORMULA_COUNT ||
        config->sections[index].formula == formula) return;
    animation->previous[index] = config->sections[index].formula;
    animation->morph[index] = 0.0f;
    config->sections[index].formula = formula;
}

/* An open graph with one value for each tangent coordinate; unlike a spiral,
 * this can be split into two sides analytically, with no flood-fill or images.
 * Every formula is windowed to zero at section endpoints: no cracks. */
static float formula_value(ProtoFormula formula, const ProtoSection *section,
                           float u, float time) {
    if (formula == PROTO_STRAIGHT) return 0.0f;
    float edge = sinf(PROTO_PI * u);
    edge *= edge;
    float phase = time * section->phase_speed;
    float a = section->cycles * u;
    float v = 0.0f;
    switch (formula) {
    case PROTO_SINE:
        v = sinf(2.0f * PROTO_PI * (a + phase));
        break;
    case PROTO_LIGHTNING:
        /* Uniform saw teeth: long rising ramp and an abrupt reset. */
        v = 2.0f * fractf_pos(a + phase) - 1.0f;
        break;
    case PROTO_FRACTAL: {
        /* Deterministic, bounded, four-octave harmonic fBm (no noise texture). */
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
    default: break;
    }
    return section->amplitude * edge * v;
}

void proto_make_frame(const ProtoConfig *config, const ProtoAnimation *animation,
                      ProtoFrame *frame) {
    const float radians = animation->rotation_degrees * (PROTO_PI / 180.0f);
    frame->cos_angle = cosf(radians);
    frame->sin_angle = sinf(radians);
    const int count = config->section_count;
    for (int i = 0; i < PROTO_PROFILE_SAMPLES; ++i) {
        float along = (float)i / (PROTO_PROFILE_SAMPLES - 1) * count;
        int section_index = (int)along;
        if (section_index >= count) section_index = count - 1;
        float u = along - section_index;
        const ProtoSection *s = &config->sections[section_index];
        float next = formula_value(s->formula, s, u, animation->time);
        float old = formula_value(animation->previous[section_index], s, u, animation->time);
        float blend = smoothstep(animation->morph[section_index]);
        frame->offsets[i] = old + (next - old) * blend;
    }
}

float proto_signed_side(const ProtoFrame *frame, float x, float y) {
    const float dx = x - PROTO_WIDTH * 0.5f;
    const float dy = y - PROTO_HEIGHT * 0.5f;
    const float tangent = dx * frame->cos_angle + dy * frame->sin_angle;
    const float normal = -dx * frame->sin_angle + dy * frame->cos_angle;
    float position = clamp01((tangent + PROTO_HALF_SPAN) / (2.0f * PROTO_HALF_SPAN));
    float sample = position * (PROTO_PROFILE_SAMPLES - 1);
    int index = (int)sample;
    if (index >= PROTO_PROFILE_SAMPLES - 1) index = PROTO_PROFILE_SAMPLES - 2;
    float blend = sample - index;
    float offset = frame->offsets[index] * (1.0f - blend) +
                   frame->offsets[index + 1] * blend;
    return normal - offset;
}
