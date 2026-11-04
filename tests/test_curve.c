/* SPDX-License-Identifier: MIT */
#include "curve.h"
#include "expression.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void check_sides(const ProtoConfig *config, ProtoAnimation *animation) {
    ProtoFrame frame;
    proto_make_frame(config, animation, &frame);
    for (int y = 0; y < PROTO_HEIGHT; y += 7)
        for (int x = 0; x < PROTO_WIDTH; x += 5)
            assert(isfinite(proto_signed_side(&frame, x + 0.5f, y + 0.5f)));
    for (int i = 0; i < PROTO_PROFILE_SAMPLES; ++i) {
        assert(isfinite(frame.offsets[i]));
        assert(fabsf(frame.offsets[i]) <= 48.1f);
    }
    for (int i = 1; i < config->section_count; ++i) {
        int index = (PROTO_PROFILE_SAMPLES - 1) * i / config->section_count;
        assert(fabsf(frame.offsets[index]) < 0.001f);
    }
}

int main(void) {
    ProtoConfig cfg;
    proto_config_defaults(&cfg);
    ProtoAnimation anim;
    proto_animation_init(&anim, &cfg);
    ProtoFormula formula;
    assert(proto_parse_formula("lightning", &formula) && formula == PROTO_LIGHTNING);
    assert(!proto_parse_formula("spiral", &formula));
    for (int sections = 1; sections <= 16; sections *= 2) {
        assert(proto_valid_section_count(sections));
        cfg.section_count = sections;
        check_sides(&cfg, &anim);
    }
    assert(!proto_valid_section_count(3) && !proto_valid_section_count(0));
    bool ok = false;
    float a = proto_expression_evaluate("tri(3*u)", 1.0f / 6.0f, 0.0f,
                                         14.0f, 3.0f, 0.1f, &ok);
    assert(ok && fabsf(a - 1.0f) < 0.0001f);
    assert(proto_expression_valid("sin(2*pi*(cycles*u+speed*t))"));
    assert(proto_expression_valid("0.5*sin(2*pi*u)+0.25*sin(8*pi*u+2*t)"));
    assert(!proto_expression_valid("sin(nonexistent)"));
    assert(!proto_expression_valid("sin(2*pi*u"));
    assert(!proto_expression_valid("1/0"));
    assert(!proto_expression_valid("sqrt(-1)"));
    cfg.section_count = 16;
    cfg.sections[15].formula = PROTO_CUSTOM;
    strcpy(cfg.sections[15].expression, "sin(2*pi*(cycles*u+speed*t))");
    proto_animation_init(&anim, &cfg);
    check_sides(&cfg, &anim);
    for (int i = 0; i < PROTO_MAX_SECTIONS; ++i)
        cfg.sections[i].formula = PROTO_STRAIGHT;
    proto_animation_init(&anim, &cfg);
    cfg.section_count = 4;
    ProtoFrame f;
    proto_make_frame(&cfg, &anim, &f);
    assert(proto_signed_side(&f, 120.5f, 15.5f) < 0.0f);
    assert(proto_signed_side(&f, 120.5f, 145.5f) > 0.0f);
    anim.rotation_degrees = 90.0f;
    proto_make_frame(&cfg, &anim, &f);
    assert(proto_signed_side(&f, 220.5f, 80.5f) < 0.0f);
    assert(proto_signed_side(&f, 20.5f, 80.5f) > 0.0f);
    anim.rotation_degrees = 180.0f;
    proto_make_frame(&cfg, &anim, &f);
    assert(proto_signed_side(&f, 120.5f, 145.5f) < 0.0f);
    assert(proto_signed_side(&f, 120.5f, 15.5f) > 0.0f);
    proto_animation_change_formula(&anim, &cfg, 0, PROTO_FRACTAL);
    assert(anim.morph[0] == 0.0f);
    proto_animation_tick(&anim, &cfg, 0.1f);
    assert(anim.morph[0] > 0.0f && anim.morph[0] < 1.0f);
    check_sides(&cfg, &anim);
    float before = anim.time;
    anim.paused = true;
    proto_animation_tick(&anim, &cfg, 0.1f);
    assert(anim.time == before);
    anim.paused = false;
    proto_animation_tick(&anim, &cfg, 0.1f);
    assert(anim.time > before);
    puts("curve_math: ok");
    return 0;
}
