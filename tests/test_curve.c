/* SPDX-License-Identifier: MIT */
#include "curve.h"
#include "expression.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    assert(proto_parse_formula("fourier", &formula) && formula == PROTO_FOURIER);
    assert(proto_parse_formula("chirp", &formula) && formula == PROTO_CHIRP);
    assert(proto_parse_formula("trochoid", &formula) && formula == PROTO_TROCHOID);
    assert(proto_parse_formula("harmonic_fractal", &formula) && formula == PROTO_HARMONIC_FRACTAL);
    assert(proto_parse_formula("spiral", &formula) && formula == PROTO_SPIRAL);
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
    /* Test fixed timestep: multiple small deltas should accumulate */
    anim.time = 0.0f;
    for (int i = 0; i < 10; ++i) proto_animation_tick(&anim, &cfg, 0.005f);
    assert(anim.time >= PROTO_FIXED_TIMESTEP);
    /* Test animation transitions setup */
    cfg.sections[0].animated = true;
    cfg.sections[0].loop = true;
    cfg.sections[0].transition_count = 2;
    cfg.sections[0].transitions[0] = (ProtoTransition){.formula = PROTO_SINE, .duration = 1.0f, .easing = PROTO_EASE_SMOOTHSTEP};
    cfg.sections[0].transitions[1] = (ProtoTransition){.formula = PROTO_STRAIGHT, .duration = 1.0f, .easing = PROTO_EASE_SMOOTHSTEP};
    proto_animation_init(&anim, &cfg);
    assert(anim.current_formula[0] == cfg.sections[0].formula);
    assert(anim.current_transition[0] == 0);
    assert(anim.transition_time[0] == 0.0f);
    /* Verify non-animated sections don't have transitions */
    assert(cfg.sections[1].animated == false);
    assert(cfg.sections[1].transition_count == 0);
    /* Test new formula types produce valid output */
    cfg.sections[0].formula = PROTO_FOURIER;
    cfg.sections[0].amplitude = 10.0f;
    proto_animation_init(&anim, &cfg);
    check_sides(&cfg, &anim);
    cfg.sections[0].formula = PROTO_CHIRP;
    check_sides(&cfg, &anim);
    cfg.sections[0].formula = PROTO_TROCHOID;
    check_sides(&cfg, &anim);
    cfg.sections[0].formula = PROTO_HARMONIC_FRACTAL;
    check_sides(&cfg, &anim);
    /* Test custom expression in transition */
    cfg.sections[0].animated = true;
    cfg.sections[0].transition_count = 1;
    cfg.sections[0].transitions[0].formula = PROTO_CUSTOM;
    strncpy(cfg.sections[0].transitions[0].expression, "sin(2*pi*u)", PROTO_EXPRESSION_LENGTH);
    cfg.sections[0].transitions[0].expression[PROTO_EXPRESSION_LENGTH - 1] = '\0';
    cfg.sections[0].formula = PROTO_STRAIGHT;
    proto_animation_init(&anim, &cfg);
    assert(anim.current_formula[0] == PROTO_STRAIGHT);
    /* Verify expression is stored correctly in transition */
    assert(strcmp(cfg.sections[0].transitions[0].expression, "sin(2*pi*u)") == 0);
    /* A true spiral folds back in tangent x, unlike the single-valued graph.
     * Tests also verify that both regions remain classified and finite. */
    cfg.section_count = 2;
    cfg.sections[0].animated = false;
    cfg.sections[1].animated = false;
    cfg.sections[0].formula = PROTO_LIGHTNING;
    cfg.sections[1].formula = PROTO_SPIRAL;
    cfg.sections[1].amplitude = 30.0f;
    proto_animation_init(&anim, &cfg);
    proto_make_frame(&cfg, &anim, &f);
    assert(f.parametric);
    bool folds = false;
    for (int k = PROTO_PROFILE_SAMPLES/2+1; k < PROTO_PROFILE_SAMPLES; ++k)
        if (f.x_coords[k] < f.x_coords[k-1] - 0.001f) folds = true;
    assert(folds);
    assert(proto_signed_side(&f, 120.5f, 8.5f) < 0.0f);
    assert(proto_signed_side(&f, 120.5f, 150.5f) > 0.0f);
    for (int y = 0; y < PROTO_HEIGHT; y += 3)
        for (int x = 0; x < PROTO_WIDTH; x += 3)
            assert(isfinite(proto_signed_side(&f, x+0.5f,y+0.5f)));
    /* First automatic entry is executed first, with its target easing. */
    cfg.sections[0].formula = PROTO_STRAIGHT;
    cfg.sections[0].animated = true;
    cfg.sections[0].loop = true;
    cfg.sections[0].transition_count = 2;
    cfg.sections[0].transitions[0] = (ProtoTransition){
        .formula=PROTO_LIGHTNING, .duration=0.1f, .easing=PROTO_EASE_CUBIC_IN_OUT};
    cfg.sections[0].transitions[1] = (ProtoTransition){
        .formula=PROTO_STRAIGHT, .duration=0.1f, .easing=PROTO_EASE_SMOOTHSTEP};
    proto_animation_init(&anim, &cfg);
    for (int step=0; step<8; ++step)
        proto_animation_tick(&anim, &cfg, PROTO_FIXED_TIMESTEP);
    assert(anim.current_formula[0] == PROTO_LIGHTNING);
    assert(anim.active_easing[0] == PROTO_EASE_CUBIC_IN_OUT);
    puts("curve_math: ok");
    return 0;
}
