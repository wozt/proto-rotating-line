/* SPDX-License-Identifier: MIT */
#include "curve.h"
#include "expression.h"
#include "parametric.h"

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
    assert(proto_parse_formula("triangle", &formula) && formula == PROTO_TRIANGLE);
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
    /* Eight actual coiled, pointed motifs on the visible right half. */
    cfg.section_count = 2;
    for (int i = 0; i < 2; ++i) {
        cfg.sections[i].animated = false;
        cfg.sections[i].gap_pixels = 2.0f;
        cfg.sections[i].repeat_visible = true;
        cfg.sections[i].cycles = 8.0f;
        cfg.sections[i].amplitude = 8.0f;
    }
    cfg.sections[0].formula = PROTO_LIGHTNING;
    cfg.sections[1].formula = PROTO_SPIRAL;
    proto_animation_init(&anim, &cfg);
    proto_make_frame(&cfg, &anim, &f);
    assert(f.parametric);
    const float logical_step = 336.0f / (PROTO_PROFILE_SAMPLES - 1);
    for (int cell = 0; cell < 8; ++cell) {
        bool curled = false;
        float left = cell * 15.0f, right = left + 15.0f;
        float max_height = 0.0f;
        for (int i = 1; i < PROTO_PROFILE_SAMPLES; ++i) {
            float baseline = -168.0f + i * logical_step;
            if (baseline <= left || baseline >= right) continue;
            if (f.x_coords[i] < f.x_coords[i - 1] - 0.005f) curled = true;
            if (fabsf(f.offsets[i]) > max_height) max_height = fabsf(f.offsets[i]);
        }
        assert(curled);
        assert(max_height < 8.0f); /* Wider crests remain within compact 8x bounds. */
    }
    /* Tip approaches zero radius and every motif joins flat segments. */
    for (int n = 1; n < 8; ++n) {
        float at = n * 15.0f;
        int index = (int)((at + 168.0f) / logical_step + 0.5f);
        assert(fabsf(f.offsets[index]) < 0.05f);
    }
    /* Configurable morph time actually controls timing, not only hold time. */
    cfg.sections[0].formula = PROTO_STRAIGHT;
    cfg.sections[0].animated = true;
    cfg.sections[0].transition_count = 2;
    cfg.sections[0].loop = true;
    cfg.sections[0].transitions[0] = (ProtoTransition){
        .formula = PROTO_LIGHTNING, .duration = 0.1f, .morph_duration = 8.0f,
        .easing = PROTO_EASE_CUBIC_IN_OUT};
    cfg.sections[0].transitions[1] = (ProtoTransition){
        .formula = PROTO_STRAIGHT, .duration = 0.1f, .morph_duration = 8.0f,
        .easing = PROTO_EASE_CUBIC_IN_OUT};
    proto_animation_init(&anim, &cfg);
    for (int step = 0; step < 7; ++step)
        proto_animation_tick(&anim, &cfg, PROTO_FIXED_TIMESTEP);
    assert(anim.current_formula[0] == PROTO_LIGHTNING);
    assert(anim.morph[0] < 0.02f);
    for (int step = 0; step < 240; ++step)
        proto_animation_tick(&anim, &cfg, PROTO_FIXED_TIMESTEP);
    assert(anim.morph[0] > 0.45f && anim.morph[0] < 0.60f);
    for (int step = 0; step < 260; ++step)
        proto_animation_tick(&anim, &cfg, PROTO_FIXED_TIMESTEP);
    assert(anim.current_formula[0] == PROTO_STRAIGHT);
    assert(anim.morph[0] < 0.10f);
    /* Regression: four *visible* motifs, not one giant curl. */
    proto_config_defaults(&cfg);
    cfg.section_count = 2;
    cfg.rotation_speed = 0.0f;
    cfg.sections[0].formula = PROTO_LIGHTNING;
    cfg.sections[0].amplitude = 11.0f;
    cfg.sections[0].cycles = 4.0f;
    cfg.sections[0].gap_pixels = 2.0f;
    cfg.sections[0].repeat_visible = true;
    cfg.sections[1].formula = PROTO_SPIRAL;
    cfg.sections[1].amplitude = 9.0f;
    cfg.sections[1].cycles = 4.0f;
    cfg.sections[1].gap_pixels = 2.0f;
    cfg.sections[1].repeat_visible = true;
    proto_animation_init(&anim, &cfg);
    proto_make_frame(&cfg, &anim, &f);
    assert(f.parametric);
    /* Four cells at approximately 30 logical pixels each. */
    int curl_cells = 0, bolt_cells = 0;
    for (int cell = 0; cell < 4; ++cell) {
        float xl = -(cell + 0.5f) * 30.0f;
        float xr = (cell + 0.5f) * 30.0f;
        float max_left = 0.0f, max_right = 0.0f;
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            float x = f.x_coords[k];
            if (fabsf(x - xl) < 11.0f && x < 0.0f)
                max_left = fmaxf(max_left, fabsf(f.offsets[k]));
            if (fabsf(x - xr) < 11.0f && x > 0.0f)
                max_right = fmaxf(max_right, fabsf(f.offsets[k]));
        }
        if (max_left > 2.0f) ++bolt_cells;
        if (max_right > 2.0f) ++curl_cells;
    }
    assert(bolt_cells == 4 && curl_cells == 4);
    /* The tapered single arm has its inner tip at the coil centre. */
    for (int j = 0; j <= 10; ++j) {
        float x, y;
        proto_spiral_point((float)j / 10.0f, 0.0f, 28.0f, 9.0f,
                           0.0f, &x, &y);
        assert(fabsf(x - 2.8f * j) < 0.0001f);
        assert(fabsf(y) < 0.0001f);
    }
    for (int y = 0; y < PROTO_HEIGHT; y += 4)
        for (int x = 0; x < PROTO_WIDTH; x += 4)
            assert(isfinite(proto_signed_side(&f, x + .5f, y + .5f)));
    /* Patch 0006: broad curling wavecrest, narrowing to a sharp point. */
    {
        float cx = 0.70f * 28.0f, cy = 0.63f * fminf(0.85f * 14.0f, 0.34f * 28.0f);
        float crest_x, crest_y, tip_x, tip_y;
        proto_spiral_point(0.37f, 0.0f, 28.0f, 14.0f, 1.0f, &crest_x, &crest_y);
        assert(crest_y > 9.0f); /* Slim rising body while retaining a crest. */
        float last_r = 1.0e6f;
        for (int j = 0; j <= 10; ++j) {
            float u = 0.37f + (0.88f - 0.37f) * (float)j / 10.0f;
            float px, py;
            proto_spiral_point(u, 0.0f, 28.0f, 14.0f, 1.0f, &px, &py);
            float r = hypotf(px - cx, py - cy);
            assert(r <= last_r + 0.001f);
            last_r = r;
        }
        proto_spiral_point(0.88f, 0.0f, 28.0f, 14.0f, 1.0f, &tip_x, &tip_y);
        assert(hypotf(tip_x - cx, tip_y - cy) < 0.002f);
        assert(crest_y > tip_y * 1.5f); /* Silhouette tapers strongly. */
        for (int j = 0; j <= 20; ++j) {
            float u = (float)j / 20.0f, px, py;
            proto_spiral_point(u, 0.0f, 28.0f, 14.0f, 0.0f, &px, &py);
            assert(fabsf(px - u * 28.0f) < 0.002f);
            assert(fabsf(py) < 0.002f);
        }
        proto_spiral_point(0.0f, 0.0f, 28.0f, 14.0f, 1.0f, &tip_x, &tip_y);
        assert(fabsf(tip_y) < 0.002f);
        proto_spiral_point(1.0f, 0.0f, 28.0f, 14.0f, 1.0f, &tip_x, &tip_y);
        assert(fabsf(tip_y) < 0.002f && fabsf(tip_x - 28.0f) < 0.002f);
    }
    /* Patch 0007: a genuine slender inward taper, with no detached tip. */
    {
        const float span = 28.0f, amplitude = 14.0f;
        const float crest_y = fminf(0.85f * amplitude, 0.34f * span);
        const float cx = 0.70f * span, cy = 0.63f * crest_y;
        float x, y;
        proto_spiral_point(0.37f, 0.0f, span, amplitude, 1.0f, &x, &y);
        assert(y > 9.0f && y < 10.0f);
        float outer, inner;
        proto_spiral_point(0.37f + (0.88f - 0.37f) * 0.55f,
                           0.0f, span, amplitude, 1.0f, &x, &y);
        outer = hypotf(x - cx, y - cy);
        proto_spiral_point(0.37f + (0.88f - 0.37f) * 0.88f,
                           0.0f, span, amplitude, 1.0f, &x, &y);
        inner = hypotf(x - cx, y - cy);
        assert(outer > 2.0f && inner < outer * 0.20f);
        proto_spiral_point(0.88f, 0.0f, span, amplitude, 1.0f, &x, &y);
        assert(hypotf(x - cx, y - cy) < 0.002f);
    }
    puts("curve_math: ok");
    return 0;
}
