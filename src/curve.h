/* SPDX-License-Identifier: MIT */
#ifndef PROTO_CURVE_H
#define PROTO_CURVE_H

#include <stdbool.h>

#define PROTO_WIDTH 240
#define PROTO_HEIGHT 160
#define PROTO_MAX_SECTIONS 8
#define PROTO_PROFILE_SAMPLES 2049

typedef enum {
    PROTO_STRAIGHT,
    PROTO_SINE,
    PROTO_LIGHTNING,
    PROTO_FRACTAL,
    PROTO_FORMULA_COUNT
} ProtoFormula;

typedef struct {
    ProtoFormula formula;
    float amplitude;       /* Pixels, measured perpendicular to the split. */
    float cycles;          /* Oscillations within this section. */
    float phase_speed;     /* Cycles per second; may be negative. */
} ProtoSection;

typedef struct {
    int section_count;    /* 2, 4 or 8. */
    float rotation_speed; /* Degrees per second, clockwise. */
    float line_width;     /* Full divider width in logical pixels. */
    ProtoSection sections[PROTO_MAX_SECTIONS];
} ProtoConfig;

typedef struct {
    float time;
    float rotation_degrees;
    bool paused;
    ProtoFormula previous[PROTO_MAX_SECTIONS];
    float morph[PROTO_MAX_SECTIONS]; /* 0 = old formula; 1 = new formula. */
} ProtoAnimation;

typedef struct {
    float cos_angle, sin_angle;
    float offsets[PROTO_PROFILE_SAMPLES];
} ProtoFrame;

const char *proto_formula_name(ProtoFormula formula);
bool proto_parse_formula(const char *name, ProtoFormula *out);
void proto_config_defaults(ProtoConfig *config);
void proto_animation_init(ProtoAnimation *animation, const ProtoConfig *config);
void proto_animation_tick(ProtoAnimation *animation, const ProtoConfig *config, float dt);
void proto_animation_change_formula(ProtoAnimation *animation, ProtoConfig *config,
                                    int index, ProtoFormula formula);
void proto_make_frame(const ProtoConfig *config, const ProtoAnimation *animation,
                      ProtoFrame *frame);
float proto_signed_side(const ProtoFrame *frame, float x, float y);
/* < 0: violet; > 0: cyan. The sign is independent of line width. */

#endif
