/* SPDX-License-Identifier: MIT */
#ifndef PROTO_CURVE_H
#define PROTO_CURVE_H

#include <stdbool.h>

#define PROTO_WIDTH 240
#define PROTO_HEIGHT 160
#define PROTO_MAX_SECTIONS 16
#define PROTO_PROFILE_SAMPLES 2049
#define PROTO_EXPRESSION_LENGTH 160

typedef enum {
    PROTO_STRAIGHT,
    PROTO_SINE,
    PROTO_LIGHTNING,
    PROTO_FRACTAL,
    PROTO_CUSTOM,
    PROTO_FORMULA_COUNT
} ProtoFormula;

typedef struct {
    ProtoFormula formula;
    float amplitude;     /* Pixels perpendicular to the original divider. */
    float cycles;        /* Repetitions within one section. */
    float phase_speed;   /* Cycles per second, signed. */
    char expression[PROTO_EXPRESSION_LENGTH]; /* For formula = custom. */
} ProtoSection;

typedef struct {
    int section_count;       /* 1, 2, 4, 8 or 16. */
    float rotation_speed;    /* Clockwise degrees per second. */
    float line_width;        /* Logical pixels, 0..4. */
    bool show_divider;
    unsigned char violet[4]; /* RGBA. */
    unsigned char cyan[4];
    unsigned char divider_color[4];
    ProtoSection sections[PROTO_MAX_SECTIONS];
} ProtoConfig;

typedef struct {
    float time;
    float rotation_degrees;
    bool paused;
    ProtoSection previous[PROTO_MAX_SECTIONS];
    float morph[PROTO_MAX_SECTIONS];
} ProtoAnimation;

typedef struct {
    float cos_angle, sin_angle;
    float offsets[PROTO_PROFILE_SAMPLES];
} ProtoFrame;

bool proto_valid_section_count(int count);
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
/* Negative: violet; positive: cyan. Independent of line width. */

#endif
