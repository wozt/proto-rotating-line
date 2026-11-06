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
    /* Patch 0010: compact centred tentacle spiral with strong taper. */
    {
        const float span = 28.0f;
        const float amplitude = 8.0f;
        float px = 0.0f, py = 0.0f;
        float max_y = -1.0e6f, min_y = 1.0e6f;
        float max_radius = 0.0f;
        float last_radius = 1.0e6f;
        bool entered_coil = false;
        bool left_axis = false;

        for (int j = 0; j <= 24; ++j) {
            float u = (float)j / 24.0f;
            float x, y;
            proto_spiral_point(u, 0.0f, span, amplitude, 1.0f, &x, &y);
            if (u >= 0.20f && u <= 0.86f) {
                float radius = hypotf(x - 0.50f * span, y);
                if (!entered_coil) {
                    entered_coil = true;
                    max_radius = radius;
                } else {
                    assert(radius <= last_radius + 0.45f);
                }
                last_radius = radius;
                if (fabsf(y) > 0.4f) left_axis = true;
            }
            if (y > max_y) max_y = y;
            if (y < min_y) min_y = y;
            px = x; py = y;
        }
        assert(entered_coil);
        assert(left_axis);
        assert(max_radius < 5.2f);
        assert(max_y > 0.65f && min_y < -1.50f);
        assert(fabsf(py) < 0.002f && fabsf(px - span) < 0.002f);

        for (int j = 0; j <= 20; ++j) {
            float u = (float)j / 20.0f;
            float x, y;
            proto_spiral_point(u, 0.0f, span, amplitude, 0.0f, &x, &y);
            assert(fabsf(x - u * span) < 0.002f);
            assert(fabsf(y) < 0.002f);
        }
    }

    /* Patch 0011: all four portions are distinct, on/off together. */
    {
        ProtoConfig spatial;
        ProtoAnimation phase;
        ProtoFrame begin, peak, midway, returning, finish;
        proto_config_defaults(&spatial);
        assert(proto_parse_formula("spatial_wave", &formula));
        assert(formula == PROTO_SPATIAL_WAVE);
        assert(proto_parse_formula("spatial_lightning", &formula));
        assert(formula == PROTO_SPATIAL_LIGHTNING);
        spatial.section_count = 2;
        spatial.rotation_speed = 0.0f;
        spatial.sections[0].formula = PROTO_SPATIAL_LIGHTNING;
        spatial.sections[1].formula = PROTO_SPATIAL_WAVE;
        for (int side = 0; side < 2; ++side) {
            ProtoSection *s = &spatial.sections[side];
            s->amplitude = side == 0 ? 11.0f : 8.0f;
            s->cycles = 4.0f;
            s->repeat_visible = true;
            s->gap_pixels = 0.5f;
            s->phase_speed = 1.0f / 18.0f;
            s->animated = false;
            s->transition_count = 0;
        }
        proto_animation_init(&phase, &spatial);
        phase.time = 0.0f;
        proto_make_frame(&spatial, &phase, &begin);
        phase.time = 9.0f;
        proto_make_frame(&spatial, &phase, &peak);
        phase.time = 4.5f;
        proto_make_frame(&spatial, &phase, &midway);
        phase.time = 13.5f;
        proto_make_frame(&spatial, &phase, &returning);
        phase.time = 18.0f;
        proto_make_frame(&spatial, &phase, &finish);
        assert(peak.parametric);
        float wave_max[4] = {0}, bolt_max[4] = {0};
        int wave_folds[4] = {0};
        const float sample = 336.0f / (PROTO_PROFILE_SAMPLES - 1);
        for (int k = 1; k < PROTO_PROFILE_SAMPLES; ++k) {
            float baseline = -168.0f + k * sample;
            assert(fabsf(begin.offsets[k]) < 0.002f);
            assert(fabsf(finish.offsets[k]) < 0.002f);
            assert(fabsf(peak.offsets[k]) < 48.0f);
            assert(fabsf(midway.offsets[k] - returning.offsets[k]) < 0.002f);
            assert(fabsf(midway.x_coords[k] - returning.x_coords[k]) < 0.002f);
            if (baseline >= 0.0f && baseline < 120.0f) {
                int i = (int)(baseline / 30.0f);
                if (i > 3) i = 3;
                wave_max[i] = fmaxf(wave_max[i], fabsf(peak.offsets[k]));
                if (peak.x_coords[k] + 0.02f < peak.x_coords[k-1])
                    ++wave_folds[i];
            } else if (baseline > -120.0f && baseline < 0.0f) {
                int i = (int)((baseline + 120.0f) / 30.0f);
                if (i > 3) i = 3;
                bolt_max[i] = fmaxf(bolt_max[i], fabsf(peak.offsets[k]));
            }
        }
        for (int i = 0; i < 4; ++i) {
            assert(wave_max[i] > 1.2f);
            assert(bolt_max[i] > 0.5f);
        }
        assert(wave_folds[0] == 0 && wave_folds[3] > 0);
        assert(bolt_max[0] > bolt_max[3] * 1.3f);
        for (int y = 1; y < PROTO_HEIGHT; y += 5)
            for (int x = 1; x < PROTO_WIDTH; x += 7)
                assert(isfinite(proto_signed_side(&peak, x + .5f, y + .5f)));
    }

    /* Patch 0013: outward scale and seam continuity. */
    {
        ProtoConfig grown;
        ProtoAnimation anim13;
        ProtoFrame flat, peak, end, middle;
        proto_config_defaults(&grown);
        grown.section_count = 2;
        grown.rotation_speed = 0.0f;
        grown.sections[0].formula = PROTO_SPATIAL_LIGHTNING;
        grown.sections[1].formula = PROTO_SPATIAL_WAVE;
        for (int side = 0; side < 2; ++side) {
            grown.sections[side].amplitude = side == 0 ? 7.5f : 6.2f;
            grown.sections[side].phase_speed = 1.0f / 18.0f;
            grown.sections[side].cycles = 4.0f;
            grown.sections[side].gap_pixels = 0.35f;
            grown.sections[side].repeat_visible = true;
            grown.sections[side].animated = false;
            grown.sections[side].transition_count = 0;
        }
        proto_animation_init(&anim13, &grown);
        anim13.time = 0.0f;
        proto_make_frame(&grown, &anim13, &flat);
        anim13.time = 9.0f;
        proto_make_frame(&grown, &anim13, &peak);
        anim13.time = 4.5f;
        proto_make_frame(&grown, &anim13, &middle);
        anim13.time = 18.0f;
        proto_make_frame(&grown, &anim13, &end);
        float wave_max[4] = {0}, bolt_max[4] = {0};
        const float step13 = 336.0f / (PROTO_PROFILE_SAMPLES - 1);
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            const float base = -168.0f + k * step13;
            assert(fabsf(flat.x_coords[k] - base) < 0.002f);
            assert(fabsf(end.x_coords[k] - base) < 0.002f);
            assert(fabsf(flat.offsets[k]) < 0.002f);
            assert(fabsf(end.offsets[k]) < 0.002f);
            assert(isfinite(peak.x_coords[k]) && isfinite(peak.offsets[k]));
            assert(isfinite(middle.x_coords[k]) && isfinite(middle.offsets[k]));
            /* No seam jumps at any of the 30px boundaries, including x=0. */
            for (int joint = -120; joint <= 120; joint += 30) {
                if (fabsf(base - (float)joint) < 0.045f) {
                    assert(fabsf(peak.offsets[k]) < 0.25f);
                    assert(fabsf(peak.x_coords[k] - base) < 0.25f);
                }
            }
            if (base >= 0.0f && base < 120.0f) {
                int cell = (int)(base / 30.0f);
                if (cell > 3) cell = 3;
                wave_max[cell] = fmaxf(wave_max[cell], fabsf(peak.offsets[k]));
            } else if (base > -120.0f && base < 0.0f) {
                int cell = (int)((base + 120.0f) / 30.0f);
                if (cell > 3) cell = 3;
                bolt_max[cell] = fmaxf(bolt_max[cell], fabsf(peak.offsets[k]));
            }
        }
        assert(wave_max[3] > 1.8f * wave_max[0]);
        assert(bolt_max[0] > 1.8f * bolt_max[3]);
        assert(wave_max[0] > 0.6f && bolt_max[3] > 0.6f);
        for (int y = 1; y < PROTO_HEIGHT; y += 5)
            for (int x = 1; x < PROTO_WIDTH; x += 7)
                assert(isfinite(proto_signed_side(&peak, x + 0.5f, y + 0.5f)));
    }

    /* Patch 0014: exact Wolfram t=0..0.5 arc, centred with straight joins. */
    {
        ProtoFormula found;
        assert(proto_parse_formula("wolfram_halfturn", &found));
        assert(found == PROTO_WOLFRAM_HALFTURN);
        const float cell = 28.0f, radius = 6.0f;
        float x, y;
        proto_wolfram_halfturn_point(0.0f, 0, cell, radius, 1, &x, &y);
        assert(fabsf(x) < 1.0e-4f && fabsf(y) < 1.0e-4f);
        proto_wolfram_halfturn_point(1.0f, 0, cell, radius, 1, &x, &y);
        assert(fabsf(x-cell) < 1.0e-4f && fabsf(y) < 1.0e-4f);
        proto_wolfram_halfturn_point(0.12f, 0, cell, radius, 1, &x, &y);
        assert(fabsf(x - (0.5f*cell + radius)) < 1.0e-3f);
        assert(fabsf(y) < 1.0e-3f);
        proto_wolfram_halfturn_point(0.88f, 0, cell, radius, 1, &x, &y);
        assert(fabsf(x - (0.5f*cell - 0.25f*radius)) < 1.0e-3f);
        assert(fabsf(y) < 1.0e-3f);
        const float t = 0.25f;
        const float p = 0.12f + (0.88f-0.12f) * (t / 0.5f);
        proto_wolfram_halfturn_point(p, 0, cell, radius, 1, &x, &y);
        assert(fabsf(x - (0.5f*cell + radius*0.5625f*cosf(6.0f*3.14159265f*t))) < 0.001f);
        assert(fabsf(y - (radius*0.5625f*sinf(6.0f*3.14159265f*t))) < 0.001f);
        for (int k = 0; k <= 100; ++k) {
            const float sample = (float)k / 100;
            proto_wolfram_halfturn_point(sample, 0, cell, radius, 0, &x, &y);
            assert(fabsf(x - cell*sample) < 0.001f && fabsf(y) < 0.001f);
        }
        ProtoConfig test_cfg;
        ProtoAnimation test_anim;
        ProtoFrame test_frame;
        proto_config_defaults(&test_cfg);
        test_cfg.section_count = 2;
        test_cfg.sections[0].formula = PROTO_STRAIGHT;
        test_cfg.sections[1].formula = PROTO_WOLFRAM_HALFTURN;
        test_cfg.sections[1].amplitude = 8;
        test_cfg.sections[1].cycles = 4;
        test_cfg.sections[1].gap_pixels = 0.35f;
        test_cfg.sections[1].repeat_visible = true;
        test_cfg.sections[1].animated = false;
        test_cfg.sections[1].phase_speed = 0;
        proto_animation_init(&test_anim, &test_cfg);
        proto_make_frame(&test_cfg, &test_anim, &test_frame);
        assert(test_frame.parametric);
        for (int y0 = 0; y0 < PROTO_HEIGHT; y0 += 9)
            for (int x0 = 0; x0 < PROTO_WIDTH; x0 += 9)
                assert(isfinite(proto_signed_side(&test_frame, x0 + .5f, y0 + .5f)));
    }
    /* Patch 0015: moving t-bound and a second offset strand. */
    {
        ProtoFormula selected;
        assert(proto_parse_formula("wolfram_tentacle", &selected));
        assert(selected == PROTO_WOLFRAM_TENTACLE);
        const float span = 28.0f, radius = 6.0f, offset = 1.08f;
        float x, y, a_x, a_y, b_x, b_y;
        for (int k = 0; k <= 100; ++k) {
            const float p = (float)k / 100.0f;
            proto_wolfram_tentacle_point(p, 3.0f, span, radius,
                                         0.0f, offset, &x, &y);
            assert(fabsf(x - (3.0f + p * span)) < 0.0001f);
            assert(fabsf(y) < 0.0001f);
        }
        proto_wolfram_tentacle_point(0.0f, 0, span, radius, 0.5f, offset, &x, &y);
        assert(fabsf(x) < 0.0001f && fabsf(y) < 0.0001f);
        proto_wolfram_tentacle_point(1.0f, 0, span, radius, 0.5f, offset, &x, &y);
        assert(fabsf(x - span) < 0.0001f && fabsf(y) < 0.0001f);
        /* Both strands reach the same moving tip, without a gap. */
        proto_wolfram_tentacle_point(0.5f, 0, span, radius, 0.25f,
                                     offset, &a_x, &a_y);
        const float tip_y = radius * 0.75f * 0.75f * sinf(1.5f * 3.14159265358979323846f);
        assert(fabsf(a_x - span * 0.5f) < 0.002f);
        assert(fabsf(a_y - tip_y) < 0.002f);
        proto_wolfram_tentacle_point(0.50001f, 0, span, radius, 0.25f,
                                     offset, &b_x, &b_y);
        assert(hypotf(b_x - a_x, b_y - a_y) < 0.01f);
        /* At full extension, the moving tip is t=0.5 on Wolfram's arc. */
        proto_wolfram_tentacle_point(0.5f, 0, span, radius, 0.5f,
                                     offset, &x, &y);
        assert(fabsf(x - (span * 0.5f - 0.25f * radius)) < 0.002f);
        assert(fabsf(y) < 0.002f);
        /* Two separate strands at their outer base. */
        proto_wolfram_tentacle_point(0.12f, 0, span, radius, 0.5f,
                                     offset, &a_x, &a_y);
        proto_wolfram_tentacle_point(0.88f, 0, span, radius, 0.5f,
                                     offset, &b_x, &b_y);
        assert(hypotf(a_x - b_x, a_y - b_y) > 0.35f);
        /* Repeated line follows exactly the same 0->0.5->0 clock. */
        ProtoConfig cfg15;
        ProtoAnimation anim15;
        ProtoFrame flat, middle, peak, return_frame;
        proto_config_defaults(&cfg15);
        cfg15.section_count = 2;
        cfg15.sections[0].formula = PROTO_STRAIGHT;
        cfg15.sections[1].formula = PROTO_WOLFRAM_TENTACLE;
        cfg15.sections[1].amplitude = 8.0f;
        cfg15.sections[1].cycles = 4.0f;
        cfg15.sections[1].gap_pixels = 0.35f;
        cfg15.sections[1].repeat_visible = true;
        cfg15.sections[1].phase_speed = 1.0f / 18.0f;
        cfg15.sections[1].animated = false;
        proto_animation_init(&anim15, &cfg15);
        anim15.time = 0.0f;
        proto_make_frame(&cfg15, &anim15, &flat);
        anim15.time = 4.5f;
        proto_make_frame(&cfg15, &anim15, &middle);
        anim15.time = 9.0f;
        proto_make_frame(&cfg15, &anim15, &peak);
        anim15.time = 13.5f;
        proto_make_frame(&cfg15, &anim15, &return_frame);
        assert(peak.parametric);
        bool visible = false;
        for (int k = 1; k < PROTO_PROFILE_SAMPLES; ++k) {
            assert(fabsf(flat.offsets[k]) < 0.002f);
            if (fabsf(peak.offsets[k]) > 1.0f) visible = true;
            assert(fabsf(middle.offsets[k] - return_frame.offsets[k]) < 0.002f);
            assert(fabsf(middle.x_coords[k] - return_frame.x_coords[k]) < 0.002f);
        }
        assert(visible);
        for (int iy = 0; iy < PROTO_HEIGHT; iy += 9)
            for (int ix = 0; ix < PROTO_WIDTH; ix += 9)
                assert(isfinite(proto_signed_side(&peak, ix + 0.5f, iy + 0.5f)));
    }
    /* Patch 0016: a *filled*, tapered ribbon, not two crossing strokes. */
    {
        ProtoFormula selected;
        assert(proto_parse_formula("wolfram_ribbon", &selected));
        assert(selected == PROTO_WOLFRAM_RIBBON);
        ProtoConfig cfg16;
        ProtoAnimation anim16;
        ProtoFrame flat, peak, mid, back;
        proto_config_defaults(&cfg16);
        cfg16.section_count = 2;
        cfg16.rotation_speed = 0.0f;
        cfg16.sections[0].formula = PROTO_STRAIGHT;
        cfg16.sections[1].formula = PROTO_WOLFRAM_RIBBON;
        cfg16.sections[1].amplitude = 8.0f;
        cfg16.sections[1].ribbon_width = 7.5f;
        cfg16.sections[1].cycles = 4.0f;
        cfg16.sections[1].repeat_visible = true;
        cfg16.sections[1].gap_pixels = 0.35f;
        cfg16.sections[1].phase_speed = 1.0f/18.0f;
        cfg16.sections[1].animated = false;
        proto_animation_init(&anim16, &cfg16);
        anim16.time = 0.0f;
        proto_make_frame(&cfg16, &anim16, &flat);
        anim16.time = 9.0f;
        proto_make_frame(&cfg16, &anim16, &peak);
        anim16.time = 4.5f;
        proto_make_frame(&cfg16, &anim16, &mid);
        anim16.time = 13.5f;
        proto_make_frame(&cfg16, &anim16, &back);
        int nflat=0, npeak=0, nmid=0, nback=0, upper=0;
        for (int y=0; y<PROTO_HEIGHT; ++y) {
            for (int x=0; x<PROTO_WIDTH; ++x) {
                int k=y*PROTO_WIDTH+x;
                nflat+=flat.ribbon_mask[k]!=0;
                npeak+=peak.ribbon_mask[k]!=0;
                nmid+=mid.ribbon_mask[k]!=0;
                nback+=back.ribbon_mask[k]!=0;
                if (peak.ribbon_mask[k] && y<PROTO_HEIGHT/2 && x>PROTO_WIDTH/2) {
                    ++upper;
                    assert(proto_signed_side(&peak, x+0.5f, y+0.5f)>=0.0f);
                }
            }
        }
        assert(nflat==0);
        assert(npeak>200 && upper>8);
        assert(nmid>0 && nback>0);
        assert(abs(nmid-nback)<12);
        /* Increasing width must increase the filled area. */
        cfg16.sections[1].ribbon_width=11.0f;
        proto_make_frame(&cfg16, &anim16, &flat); /* time=13.5 */
        int thick=0;
        for (int k=0; k<PROTO_WIDTH*PROTO_HEIGHT; ++k)
            thick+=flat.ribbon_mask[k]!=0;
        assert(thick>nback);
        /* A 90-degree rotation should preserve an actual filled mask. */
        anim16.rotation_degrees=90.0f;
        proto_make_frame(&cfg16, &anim16, &flat);
        int rotated=0;
        for (int k=0; k<PROTO_WIDTH*PROTO_HEIGHT; ++k)
            rotated+=flat.ribbon_mask[k]!=0;
        assert(rotated>50);
    }

    /* Patch 0017: cyan and violet are actually distinct filled strands. */
    {
        ProtoFormula parsed;
        assert(proto_parse_formula("wolfram_double_ribbon", &parsed));
        assert(parsed == PROTO_WOLFRAM_DOUBLE_RIBBON);
        ProtoConfig cfg17;
        ProtoAnimation animation17;
        ProtoFrame flat17, expanded17, midway17, returning17;
        proto_config_defaults(&cfg17);
        cfg17.section_count = 2;
        cfg17.rotation_speed = 0.0f;
        cfg17.show_divider = false;
        cfg17.sections[0].formula = PROTO_STRAIGHT;
        cfg17.sections[1].formula = PROTO_WOLFRAM_DOUBLE_RIBBON;
        cfg17.sections[1].amplitude = 10.5f;
        cfg17.sections[1].ribbon_width = 5.0f;
        cfg17.sections[1].cycles = 4.0f;
        cfg17.sections[1].gap_pixels = 0.35f;
        cfg17.sections[1].repeat_visible = true;
        cfg17.sections[1].phase_speed = 1.0f/18.0f;
        cfg17.sections[1].animated = false;
        proto_animation_init(&animation17, &cfg17);
        animation17.time = 0.0f;
        proto_make_frame(&cfg17, &animation17, &flat17);
        animation17.time = 9.0f;
        proto_make_frame(&cfg17, &animation17, &expanded17);
        animation17.time = 4.5f;
        proto_make_frame(&cfg17, &animation17, &midway17);
        animation17.time = 13.5f;
        proto_make_frame(&cfg17, &animation17, &returning17);
        int flat_count = 0, cyan = 0, violet = 0, cross_cyan = 0, cross_violet = 0;
        int cyan_cells[4] = {0}, violet_cells[4] = {0};
        for (int y = 0; y < PROTO_HEIGHT; ++y) {
            for (int x = 0; x < PROTO_WIDTH; ++x) {
                const int k = y*PROTO_WIDTH+x;
                flat_count += flat17.ribbon_mask[k] != 0;
                if (expanded17.ribbon_mask[k] == 2) {
                    cyan++;
                    if (y < PROTO_HEIGHT/2) cross_cyan++;
                    int n = (x-PROTO_WIDTH/2)/30;
                    if (n>=0 && n<4) cyan_cells[n]++;
                    assert(proto_signed_side(&expanded17, x+0.5f, y+0.5f) > 0.0f);
                } else if (expanded17.ribbon_mask[k] == 3) {
                    violet++;
                    if (y > PROTO_HEIGHT/2) cross_violet++;
                    int n = (x-PROTO_WIDTH/2)/30;
                    if (n>=0 && n<4) violet_cells[n]++;
                    assert(proto_signed_side(&expanded17, x+0.5f, y+0.5f) < 0.0f);
                }
                assert(midway17.ribbon_mask[k] == returning17.ribbon_mask[k]);
            }
        }
        assert(flat_count == 0);
        assert(cyan > 60 && violet > 60);
        assert(cross_cyan > 0 && cross_violet > 0);
        for (int i=0; i<4; ++i) assert(cyan_cells[i] > 2 && violet_cells[i] > 2);
        /* The static original 0016 mode is preserved, still mask=1. */
        cfg17.sections[1].formula = PROTO_WOLFRAM_RIBBON;
        proto_make_frame(&cfg17, &animation17, &flat17);
        int legacy = 0;
        for (int i=0; i<PROTO_WIDTH*PROTO_HEIGHT; ++i) {
            assert(flat17.ribbon_mask[i] != 2 && flat17.ribbon_mask[i] != 3);
            if (flat17.ribbon_mask[i] == 1) ++legacy;
        }
        assert(legacy > 20);
    }

    /* Patch 0019: genuine two-arm Fermat, continuous joins and 18s cycle. */
    {
        ProtoFormula parsed;
        assert(proto_parse_formula("fermat_spiral", &parsed));
        assert(parsed == PROTO_FERMAT_SPIRAL);
        const float start = 3.0f, span = 30.0f, amp = 11.0f;
        const float centre = start + 0.5f*span;
        float x, y, xother, yother;
        proto_fermat_spiral_point(0.0f, start, span, amp, 1.0f, &x, &y);
        assert(fabsf(x-start) < 0.0002f && fabsf(y) < 0.0002f);
        proto_fermat_spiral_point(1.0f, start, span, amp, 1.0f, &x, &y);
        assert(fabsf(x-(start+span)) < 0.0002f && fabsf(y) < 0.0002f);
        proto_fermat_spiral_point(0.12f, start, span, amp, 1.0f, &x, &y);
        assert(fabsf(x-(centre-amp)) < 0.0002f && fabsf(y) < 0.0002f);
        proto_fermat_spiral_point(0.88f, start, span, amp, 1.0f, &x, &y);
        assert(fabsf(x-(centre+amp)) < 0.0002f && fabsf(y) < 0.0002f);
        for (int k=0; k<=100; ++k) {
            float p = (float)k/100.0f;
            proto_fermat_spiral_point(p, start, span, amp, 0.0f, &x, &y);
            assert(isfinite(x) && fabsf(y) < 0.0002f);
            proto_fermat_spiral_point(p, start, span, amp, 1.0f, &x, &y);
            assert(isfinite(x) && isfinite(y));
        }
        for (int k=0; k<=40; ++k) {
            float p = 0.12f+(0.38f*k/40.0f);
            proto_fermat_spiral_point(p, start, span, amp, 0.65f, &x, &y);
            proto_fermat_spiral_point(1.0f-p, start, span, amp, 0.65f,
                                       &xother, &yother);
            assert(fabsf(x+xother-2.0f*centre) < 0.0004f);
            assert(fabsf(y+yother) < 0.0004f);
        }
        ProtoConfig fermat_cfg;
        ProtoAnimation fermat_anim;
        ProtoFrame at0, at9, at18, at45;
        proto_config_defaults(&fermat_cfg);
        fermat_cfg.section_count = 2;
        fermat_cfg.sections[0].formula = PROTO_STRAIGHT;
        fermat_cfg.sections[1].formula = PROTO_FERMAT_SPIRAL;
        fermat_cfg.sections[1].amplitude = 11.0f;
        fermat_cfg.sections[1].cycles = 4.0f;
        fermat_cfg.sections[1].phase_speed = 1.0f/18.0f;
        fermat_cfg.sections[1].repeat_visible = true;
        fermat_cfg.sections[1].gap_pixels = 0.25f;
        fermat_cfg.sections[1].animated = false;
        proto_animation_init(&fermat_anim, &fermat_cfg);
        fermat_anim.time = 0.0f;
        proto_make_frame(&fermat_cfg, &fermat_anim, &at0);
        fermat_anim.time = 9.0f;
        proto_make_frame(&fermat_cfg, &fermat_anim, &at9);
        fermat_anim.time = 18.0f;
        proto_make_frame(&fermat_cfg, &fermat_anim, &at18);
        fermat_anim.time = 4.5f;
        proto_make_frame(&fermat_cfg, &fermat_anim, &at45);
        assert(at9.parametric && at0.parametric);
        float at9_max = 0.0f, at45_max = 0.0f;
        for (int i=0; i<PROTO_PROFILE_SAMPLES; ++i) {
            assert(fabsf(at0.offsets[i]) < 0.002f);
            assert(fabsf(at18.offsets[i]) < 0.002f);
            at9_max = fmaxf(at9_max, fabsf(at9.offsets[i]));
            at45_max = fmaxf(at45_max, fabsf(at45.offsets[i]));
        }
        assert(at9_max > at45_max && at45_max > 1.0f);
        int changed = 0;
        for (int iy=38; iy<122; iy+=2)
            for (int ix=120; ix<240; ix+=2) {
                float flat = proto_signed_side(&at0, ix+0.5f, iy+0.5f);
                float curl = proto_signed_side(&at9, ix+0.5f, iy+0.5f);
                assert(isfinite(flat) && isfinite(curl));
                changed += (flat < 0.0f) != (curl < 0.0f);
            }
        assert(changed > 20);
    }

    /* Patch 0020: tentacle profile keeps broad base and sharper curled tip. */
    {
        const float start = 3.0f, span = 30.0f, amp = 12.0f;
        float x, y;
        proto_fermat_spiral_point(0.20f, start, span, amp, 1.0f, &x, &y);
        const float near_base_y = fabsf(y);
        proto_fermat_spiral_point(0.44f, start, span, amp, 1.0f, &x, &y);
        const float near_tip_y = fabsf(y);
        assert(near_tip_y > near_base_y * 0.55f);
        proto_fermat_spiral_point(0.49f, start, span, amp, 1.0f, &x, &y);
        assert(hypotf(x - (start + 0.5f*span), y) < 2.0f);
    }
    /* Patch 0021: synchronised graded amplitudes and parabolic-speed clock. */
    {
        const float speed = 1.0f / 18.0f;
        const float times[] = {0, 2.25f, 4.5f, 6.75f, 9.0f,
                               11.25f, 13.5f, 15.75f, 18.0f};
        const float expected[] = {0, 0.15625f, 0.5f, 0.84375f, 1,
                                  0.84375f, 0.5f, 0.15625f, 0};
        for (int k = 0; k < 9; ++k)
            assert(fabsf(proto_fermat_parabolic_progress(times[k], speed) -
                         expected[k]) < 0.0001f);
        assert(proto_fermat_parabolic_progress(9.0f, 0.0f) == 1.0f);
        /* Three displacements over equal time intervals; middle is fastest. */
        assert(expected[2] - expected[1] > expected[1] - expected[0]);
        assert(expected[4] - expected[3] < expected[3] - expected[2]);

        const float span = 29.75f, amp = 15.0f, centre = span * 0.5f;
        float radii[4];
        for (int i = 0; i < 4; ++i) {
            float x, y;
            proto_fermat_spiral_point(0.12f, 0.0f, span,
                amp * (float)(i+1) * 0.25f, 1.0f, &x, &y);
            assert(fabsf(y) < 0.0001f);
            radii[i] = centre - x;
        }
        assert(radii[1] > 1.9f * radii[0]);
        assert(radii[2] > 1.4f * radii[1]);
        assert(radii[3] > 1.28f * radii[2]);
        assert(radii[3] > 0.47f * span && radii[3] < 0.5f * span);
        /* The connector-to-spiral tangent and curvature flatten at the root. */
        for (int k = 1; k <= 3; ++k) {
            float x, y;
            proto_fermat_spiral_point(0.12f + 0.0001f*k, 0.0f,
                                      span, amp, 1.0f, &x, &y);
            assert(fabsf(y) < 0.001f);
        }

        ProtoConfig cfg21;
        ProtoAnimation anim21;
        ProtoFrame flat21, peak21, middle21, returning21, end21;
        proto_config_defaults(&cfg21);
        cfg21.section_count = 2;
        cfg21.rotation_speed = 0.0f;
        cfg21.sections[0].formula = PROTO_STRAIGHT;
        cfg21.sections[1].formula = PROTO_FERMAT_SPIRAL;
        cfg21.sections[1].amplitude = amp;
        cfg21.sections[1].cycles = 4.0f;
        cfg21.sections[1].gap_pixels = 0.25f;
        cfg21.sections[1].repeat_visible = true;
        cfg21.sections[1].animated = false;
        cfg21.sections[1].phase_speed = speed;
        proto_animation_init(&anim21, &cfg21);
        anim21.time = 0.0f;
        proto_make_frame(&cfg21, &anim21, &flat21);
        anim21.time = 4.5f;
        proto_make_frame(&cfg21, &anim21, &middle21);
        anim21.time = 9.0f;
        proto_make_frame(&cfg21, &anim21, &peak21);
        anim21.time = 13.5f;
        proto_make_frame(&cfg21, &anim21, &returning21);
        anim21.time = 18.0f;
        proto_make_frame(&cfg21, &anim21, &end21);
        float cell_max[4] = {0};
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            const float base = -168.0f + 336.0f * k / (PROTO_PROFILE_SAMPLES - 1);
            assert(fabsf(flat21.offsets[k]) < 0.002f);
            assert(fabsf(end21.offsets[k]) < 0.002f);
            assert(fabsf(middle21.offsets[k] - returning21.offsets[k]) < 0.002f);
            assert(fabsf(middle21.x_coords[k] - returning21.x_coords[k]) < 0.002f);
            if (base >= 0.0f && base < 120.0f) {
                int i = (int)(base / 30.0f);
                if (i > 3) i = 3;
                cell_max[i] = fmaxf(cell_max[i], fabsf(peak21.offsets[k]));
            }
        }
        assert(cell_max[0] > 0.15f);
        for (int k = 1; k < 4; ++k)
            assert(cell_max[k] > cell_max[k-1] * 1.15f);
    }

    /* Patch 0025: six outward-growing coils and separate winding gradient. */
    {
        const float speed = 0.2f;
        const float length = 120.0f / 6.0f - 0.25f;
        const float full_amp = 15.0f;
        const float full_radius = fminf(full_amp, 0.49f * length);
        const float expected_size[6] = {.50f, .60f, .70f, .80f, .90f, 1.00f};
        const float expected_winding[6] = {.25f, .40f, .55f, .70f, .85f, 1.00f};
        float radius[6];
        for (int i = 0; i < 6; ++i) {
            const float outward = (float)i / 5.0f;
            const float grade = .50f + .50f * outward;
            const float winding = .25f + .75f * outward;
            const float direction = (i & 1) ? -1.0f : 1.0f;
            float x, y, rx, ry;
            assert(fabsf(grade - expected_size[i]) < .00001f);
            assert(fabsf(winding - expected_winding[i]) < .00001f);
            proto_fermat_twist_point(.20f, 0.0f, length, full_radius * grade,
                                      1.0f, winding, direction, &x, &y);
            radius[i] = .5f * length - x;
            assert(fabsf(y) < .0001f);
            assert(fabsf(radius[i] / full_radius - expected_size[i]) < .0002f);
            if (i > 0) assert(radius[i] > radius[i-1]);
            /* Index parity mirrors the curl without changing its size. */
            proto_fermat_twist_point(.37f, 0.0f, length, full_radius * grade,
                                      1.0f, winding, direction, &x, &y);
            proto_fermat_twist_point(.37f, 0.0f, length, full_radius * grade,
                                      1.0f, winding, -direction, &rx, &ry);
            assert(fabsf(x-rx) < .0002f && fabsf(y+ry) < .0002f);
        }
        assert(fabsf(radius[0] / radius[5] - .50f) < .0002f);
        assert(fabsf(radius[4] / radius[5] - .90f) < .0002f);

        /* Preserve the fast, strongly eased 5-second cycle and time parity. */
        assert(fabsf(proto_fermat_twist_progress(0.0f, speed)) < .0001f);
        assert(fabsf(proto_fermat_twist_progress(1.25f, speed) - .5f) < .0001f);
        assert(fabsf(proto_fermat_twist_progress(2.50f, speed) - 1.0f) < .0001f);
        assert(fabsf(proto_fermat_twist_progress(3.75f, speed) - .5f) < .0001f);
        assert(fabsf(proto_fermat_twist_progress(5.00f, speed)) < .0001f);
        assert(proto_fermat_twist_direction(2.5f, speed) == 1.0f);
        assert(proto_fermat_twist_direction(7.5f, speed) == -1.0f);

        ProtoConfig cfg25;
        ProtoAnimation anim25;
        ProtoFrame flat25, full25, inverted25, end25;
        proto_config_defaults(&cfg25);
        cfg25.section_count = 2;
        cfg25.rotation_speed = 0.0f;
        cfg25.sections[0].formula = PROTO_STRAIGHT;
        cfg25.sections[1].formula = PROTO_FERMAT_TWIST;
        cfg25.sections[1].amplitude = full_amp;
        cfg25.sections[1].cycles = 6.0f;
        cfg25.sections[1].gap_pixels = .25f;
        cfg25.sections[1].repeat_visible = true;
        cfg25.sections[1].animated = false;
        cfg25.sections[1].phase_speed = speed;
        proto_animation_init(&anim25, &cfg25);
        anim25.time = 0.0f;
        proto_make_frame(&cfg25, &anim25, &flat25);
        anim25.time = 2.5f;
        proto_make_frame(&cfg25, &anim25, &full25);
        anim25.time = 7.5f;
        proto_make_frame(&cfg25, &anim25, &inverted25);
        anim25.time = 10.0f;
        proto_make_frame(&cfg25, &anim25, &end25);
        assert(full25.parametric && inverted25.parametric);
        int deformed = 0;
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            assert(isfinite(full25.x_coords[k]) && isfinite(full25.offsets[k]));
            assert(fabsf(flat25.offsets[k]) < .003f);
            assert(fabsf(end25.offsets[k]) < .003f);
            assert(fabsf(full25.x_coords[k] - inverted25.x_coords[k]) < .012f);
            assert(fabsf(full25.offsets[k] + inverted25.offsets[k]) < .012f);
            if (fabsf(full25.offsets[k]) > .08f) ++deformed;
        }
        assert(deformed > 24);

        /* Verify the actual dispatcher for each of the six cell roots.
         * Do not infer radius from maximal pixel Y, which oscillates with
         * winding angle and is NOT a reliable measurement of base size. */
        const float cell = 120.0f / 6.0f;
        for (int i = 0; i < 6; ++i) {
            const float cell_start = (float)i * cell + .125f;
            const float x_root = cell_start + .20f * length;
            const int k = (int)lroundf((x_root + 168.0f) *
                                         (PROTO_PROFILE_SAMPLES - 1) / 336.0f);
            const float base = -168.0f + 336.0f * (float)k /
                                               (PROTO_PROFILE_SAMPLES - 1);
            const float p = (base - cell_start) / length;
            float expected_x, expected_y;
            proto_fermat_twist_point(p, cell_start, length,
                    full_radius * expected_size[i], 1.0f,
                    expected_winding[i], (i & 1) ? -1.0f : 1.0f,
                    &expected_x, &expected_y);
            assert(fabsf(full25.x_coords[k] - expected_x) < .06f);
            assert(fabsf(full25.offsets[k] - expected_y) < .06f);
        }
        for (int y = 0; y < PROTO_HEIGHT; y += 13)
            for (int x = 0; x < PROTO_WIDTH; x += 11) {
                assert(isfinite(proto_signed_side(&full25, x+.5f, y+.5f)));
                assert(isfinite(proto_signed_side(&inverted25, x+.5f, y+.5f)));
            }
    }

    /* Patch 0027: eight size-weighted centres meet exactly at full growth. */
    {
        const float speed = 0.2f;
        const float span = 120.0f;
        const float size_step = exp2f(1.0f/7.0f);
        const float winding_step = 0.75f / 7.0f;
        float previous_end = 0.0f;
        float previous_diameter = 0.0f;
        for (int i = 0; i < 8; ++i) {
            const float grade = 0.5f * powf(size_step, (float)i);
            const float winding = 0.25f + winding_step * (float)i;
            const float direction = (i & 1) ? -1.0f : 1.0f;
            float start, cell_span, x, y;
            proto_fermat_packed_cell(0.0f, span, 8, i, &start, &cell_span);
            const float end = start + cell_span;
            const float radius = 0.5f * cell_span;
            const float centre = start + radius;
            float sum = 0.0f;
            for (int j = 0; j < 8; ++j) sum += 0.5f * powf(size_step, (float)j);
            const float expected_diameter = span * grade / sum;
            assert(fabsf(cell_span - expected_diameter) < 0.0001f);
            if (i > 0) {
                assert(fabsf(start - previous_end) < 0.00003f);
                assert(cell_span > previous_diameter);
            }
            /* Real physical root positions, not inferred from pixel Y peaks. */
            proto_fermat_twist_point(0.20f, start, cell_span, radius,
                                      1.0f, winding, direction, &x, &y);
            assert(fabsf(x - start) < 0.0001f && fabsf(y) < 0.0001f);
            proto_fermat_twist_point(0.80f, start, cell_span, radius,
                                      1.0f, winding, direction, &x, &y);
            assert(fabsf(x - end) < 0.0001f && fabsf(y) < 0.0001f);
            /* Each spiral has a distinct winding angle at the same clock. */
            assert(fabsf(winding - (0.25f + winding_step*i)) < 0.00001f);
            assert(fabsf(centre - (start + 0.5f*cell_span)) < 0.00001f);
            /* Both clockwise and counterclockwise traces are exact mirrors. */
            {
                float ax, ay, bx, by;
                proto_fermat_twist_point(0.43f, start, cell_span, radius,
                    1.0f, winding, direction, &ax, &ay);
                proto_fermat_twist_point(0.43f, start, cell_span, radius,
                    1.0f, winding, -direction, &bx, &by);
                assert(fabsf(ax-bx) < 0.0003f);
                assert(fabsf(ay+by) < 0.0003f);
            }
            previous_end = end;
            previous_diameter = cell_span;
        }
        assert(fabsf(previous_end - span) < 0.0001f);
        assert(fabsf(proto_fermat_twist_progress(2.5f, speed) - 1.0f) < .0001f);
        assert(proto_fermat_twist_direction(2.5f, speed) == 1.0f);
        assert(proto_fermat_twist_direction(7.5f, speed) == -1.0f);

        ProtoConfig cfg26;
        ProtoAnimation anim26;
        ProtoFrame flat26, peak26, reversed26, end26;
        proto_config_defaults(&cfg26);
        cfg26.section_count = 2;
        cfg26.rotation_speed = 0.0f;
        cfg26.sections[0].formula = PROTO_STRAIGHT;
        cfg26.sections[1].formula = PROTO_FERMAT_TWIST;
        cfg26.sections[1].amplitude = 15.0f;
        cfg26.sections[1].cycles = 8.0f;
        cfg26.sections[1].gap_pixels = 0.0f;
        cfg26.sections[1].repeat_visible = true;
        cfg26.sections[1].phase_speed = speed;
        cfg26.sections[1].animated = false;
        proto_animation_init(&anim26, &cfg26);
        anim26.time = 0.0f;
        proto_make_frame(&cfg26, &anim26, &flat26);
        anim26.time = 2.5f;
        proto_make_frame(&cfg26, &anim26, &peak26);
        anim26.time = 7.5f;
        proto_make_frame(&cfg26, &anim26, &reversed26);
        anim26.time = 10.0f;
        proto_make_frame(&cfg26, &anim26, &end26);
        assert(peak26.parametric && reversed26.parametric);
        int nonflat = 0;
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            assert(isfinite(peak26.x_coords[k]) && isfinite(peak26.offsets[k]));
            assert(fabsf(flat26.offsets[k]) < .002f);
            assert(fabsf(end26.offsets[k]) < .002f);
            assert(fabsf(peak26.x_coords[k] - reversed26.x_coords[k]) < .005f);
            assert(fabsf(peak26.offsets[k] + reversed26.offsets[k]) < .005f);
            nonflat += fabsf(peak26.offsets[k]) > 0.08f;
        }
        assert(nonflat > 25);
        /* Check root positions on the actual frame, away from sample joins. */
        for (int i = 0; i < 8; ++i) {
            float start, length, ex, ey;
            proto_fermat_packed_cell(0.0f, 120.0f, 8, i, &start, &length);
            const float probe = start + 0.20f*length;
            const int k = (int)lroundf((probe + 168.0f) *
                (PROTO_PROFILE_SAMPLES - 1) / 336.0f);
            const float base = -168.0f + 336.0f*(float)k /
                (PROTO_PROFILE_SAMPLES - 1);
            const float local_p = (base-start)/length;
            proto_fermat_twist_point(local_p, start, length,
                0.5f*length, 1.0f, .25f+.75f*i/7.0f,
                (i & 1) ? -1.0f : 1.0f, &ex, &ey);
            assert(fabsf(peak26.x_coords[k]-ex) < .08f);
            assert(fabsf(peak26.offsets[k]-ey) < .08f);
        }
        for (int yy=0; yy<PROTO_HEIGHT; yy+=13)
            for (int xx=0; xx<PROTO_WIDTH; xx+=11) {
                assert(isfinite(proto_signed_side(&peak26, xx+.5f, yy+.5f)));
                assert(isfinite(proto_signed_side(&reversed26, xx+.5f, yy+.5f)));
            }
    }

    puts("curve_math: ok");
    /* Patch 0028: exponential touching widths and delayed synchronized peaks. */
    {
        const float half = 120.0f, speed = 0.2f;
        float sizes[8], last = 0.0f;
        for (int i = 0; i < 8; ++i) {
            float x, w;
            proto_fermat_packed_cell(0.0f, half, 8, i, &x, &w);
            assert(fabsf(x-last) < .00004f);
            if (i > 0) assert(w > sizes[i-1]);
            sizes[i] = w;
            last = x+w;
            /* They still all touch at the common peak. */
            assert(fabsf(proto_fermat_stagger_progress(2.5f, speed, i, .075f)-1.0f) < .00001f);
            assert(fabsf(proto_fermat_stagger_progress(0.0f, speed, i, .075f)) < .00001f);
            assert(isfinite(proto_fermat_stagger_progress(1.5f, speed, i, .075f)));
            assert(proto_fermat_stagger_progress(1.0f, speed, i, .075f) <=
                   proto_fermat_stagger_progress(1.0f, speed, 0, .075f)+.00001f);
        }
        assert(fabsf(last-half) < .00005f);
        assert(fabsf(sizes[0]/sizes[7]-0.5f) < .00002f);
        assert(sizes[4]/sizes[3] > sizes[1]/sizes[0]-0.0001f);
        assert(fabsf(proto_metroid_lightning_value(6.0f)-
            ((1.0f/(1.0f+expf(-15.0f))) * (0.15f*3.0f+0.5f)*(-0.5f))) < .0002f);
        assert(isfinite(proto_metroid_lightning_value(8.75f)));

        ProtoConfig cfg28;
        ProtoAnimation anim28;
        ProtoFrame start28, peak28;
        proto_config_defaults(&cfg28);
        cfg28.section_count = 2;
        cfg28.sections[0].formula = PROTO_METROID_LIGHTNING;
        cfg28.sections[0].amplitude = 8.2f;
        cfg28.sections[0].cycles = 4.0f;
        cfg28.sections[0].repeat_visible = true;
        cfg28.sections[0].phase_speed = speed;
        cfg28.sections[1].formula = PROTO_FERMAT_TWIST;
        cfg28.sections[1].amplitude = 15.0f;
        cfg28.sections[1].cycles = 8.0f;
        cfg28.sections[1].repeat_visible = true;
        cfg28.sections[1].phase_speed = speed;
        proto_animation_init(&anim28, &cfg28);
        anim28.time = 0;
        proto_make_frame(&cfg28, &anim28, &start28);
        anim28.time = 2.5f;
        proto_make_frame(&cfg28, &anim28, &peak28);
        assert(peak28.parametric);
        int flashes = 0;
        for (int k = 0; k < PROTO_PROFILE_SAMPLES; ++k) {
            assert(isfinite(peak28.x_coords[k]) && isfinite(peak28.offsets[k]));
            if (k < PROTO_PROFILE_SAMPLES/2)
                flashes += fabsf(peak28.offsets[k]) > .2f;
            assert(fabsf(start28.offsets[k]) < .003f);
        }
        assert(flashes > 25);
    }

    /* Patch 0029: transient pulses change their respective side only. */
    {
        ProtoConfig p29;
        ProtoAnimation a29;
        ProtoFrame base29, aria29, metroid29;
        proto_config_defaults(&p29);
        p29.section_count = 2;
        p29.sections[0].formula = PROTO_METROID_LIGHTNING;
        p29.sections[0].amplitude = 8.2f;
        p29.sections[0].cycles = 4.0f;
        p29.sections[0].repeat_visible = true;
        p29.sections[0].phase_speed = 0.2f;
        p29.sections[1].formula = PROTO_FERMAT_TWIST;
        p29.sections[1].amplitude = 15.0f;
        p29.sections[1].cycles = 8.0f;
        p29.sections[1].repeat_visible = true;
        p29.sections[1].phase_speed = 0.2f;
        proto_animation_init(&a29, &p29);
        assert(a29.aria_pulse_age < 0 && a29.metroid_pulse_age < 0);
        a29.time = 1.6f;
        proto_make_frame(&p29, &a29, &base29);
        a29.aria_pulse_age = 0.11f;
        proto_make_frame(&p29, &a29, &aria29);
        a29.aria_pulse_age = -1.0f;
        a29.metroid_pulse_age = 0.11f;
        proto_make_frame(&p29, &a29, &metroid29);
        int changed_aria = 0, changed_metroid = 0;
        for (int i = 0; i < PROTO_PROFILE_SAMPLES; ++i) {
            float base = -168.0f+336.0f*i/(PROTO_PROFILE_SAMPLES-1);
            if (base >= 0.0f) {
                changed_aria += fabsf(base29.offsets[i]-aria29.offsets[i]) > 0.0005f ||
                    fabsf(base29.x_coords[i]-aria29.x_coords[i]) > 0.0005f;
                assert(fabsf(base29.offsets[i]-metroid29.offsets[i]) < 0.0002f);
            } else {
                changed_metroid += fabsf(base29.offsets[i]-metroid29.offsets[i]) > 0.0005f;
                assert(fabsf(base29.offsets[i]-aria29.offsets[i]) < 0.0002f);
            }
        }
        assert(changed_aria > 8 && changed_metroid > 8);
    }

    /* Patch 0030: true 8-period lightning, tall curls, bidirectional delay. */
    {
        ProtoFormula m30, a30;
        assert(proto_parse_formula("metroid_eight", &m30) && m30 == PROTO_METROID_EIGHT);
        assert(proto_parse_formula("fermat_tall", &a30) && a30 == PROTO_FERMAT_TALL);
        const float speed = 0.2f;
        for (int k = 0; k < 8; ++k) {
            const float wave_peak = proto_fermat_tall_reference_peak(k);
            assert(wave_peak > 0.2f && wave_peak < 1.1f);
            assert(fabsf(proto_fermat_bothways_progress(2.5f, speed, k, 0.2f)-1.0f) < .00001f);
            assert(fabsf(proto_fermat_bothways_progress(5.0f, speed, k, 0.2f)) < .00001f);
            assert(fabsf(proto_fermat_bothways_progress(0.0f, speed, k, 0.2f)) < .00001f);
            const float at_one = proto_fermat_bothways_progress(1.0f, speed, k, .2f);
            const float before_fold = proto_fermat_bothways_progress(3.0f, speed, k, .2f);
            assert(isfinite(at_one) && isfinite(before_fold));
            if (k) assert(at_one <= proto_fermat_bothways_progress(1.0f, speed, k-1, .2f)+.00001f);
        }
        assert(proto_fermat_bothways_progress(1.0f, speed, 7, .2f) == 0.0f);
        assert(fabsf(proto_fermat_bothways_progress(3.0f, speed, 7, .2f)-1.0f) < .00001f);
        assert(proto_fermat_bothways_progress(3.0f, speed, 0, .2f) < 1.0f);
        assert(proto_fermat_bothways_progress(4.1f, speed, 7, .2f) < 1.0f);

        ProtoConfig cfg30;
        ProtoAnimation ani30;
        ProtoFrame flat30, peak30, delayed30, reverse30;
        proto_config_defaults(&cfg30);
        cfg30.section_count = 2;
        cfg30.rotation_speed = 0.0f;
        cfg30.sections[0].formula = PROTO_METROID_EIGHT;
        cfg30.sections[0].amplitude = 24.0f;
        cfg30.sections[0].cycles = 8.0f;
        cfg30.sections[0].repeat_visible = true;
        cfg30.sections[0].phase_speed = speed;
        cfg30.sections[1].formula = PROTO_FERMAT_TALL;
        cfg30.sections[1].amplitude = 15.0f;
        cfg30.sections[1].cycles = 8.0f;
        cfg30.sections[1].repeat_visible = true;
        cfg30.sections[1].phase_speed = speed;
        proto_animation_init(&ani30, &cfg30);
        ani30.time = 0.0f;
        proto_make_frame(&cfg30, &ani30, &flat30);
        ani30.time = 1.0f;
        proto_make_frame(&cfg30, &ani30, &delayed30);
        ani30.time = 2.5f;
        proto_make_frame(&cfg30, &ani30, &peak30);
        ani30.time = 7.5f;
        proto_make_frame(&cfg30, &ani30, &reverse30);
        float max_y[8] = {0.0f};
        float metroid_y[8] = {0.0f};
        for (int j = 0; j < PROTO_PROFILE_SAMPLES; ++j) {
            const float base = -168.0f+336.0f*j/(PROTO_PROFILE_SAMPLES-1);
            assert(isfinite(peak30.x_coords[j]) && isfinite(peak30.offsets[j]));
            assert(fabsf(flat30.offsets[j]) < .005f);
            if (base >= 0.0f && base < 120.0f) {
                for (int k = 0; k < 8; ++k) {
                    float start, span;
                    proto_fermat_packed_cell(0.0f, 120.0f, 8, k, &start, &span);
                    if (base >= start && base < start+span) {
                        max_y[k] = fmaxf(max_y[k],fabsf(peak30.offsets[j]));
                        assert(fabsf(peak30.x_coords[j]-reverse30.x_coords[j]) < .01f);
                        assert(fabsf(peak30.offsets[j]+reverse30.offsets[j]) < .01f);
                        break;
                    }
                }
            } else if (base >= -120.0f && base < 0.0f) {
                int k = 7-(int)((base+120.0f)/15.0f);
                if (k < 0) k=0; if (k>7) k=7;
                metroid_y[k] = fmaxf(metroid_y[k],fabsf(peak30.offsets[j]));
            }
        }
        /* At zero rotation, last spiral reaches >= half the 160px screen. */
        assert(2.0f*max_y[7] >= 80.0f);
        assert(2.0f*max_y[7] <= 102.0f);
        for (int k = 0; k < 8; ++k) {
            assert(max_y[k] > 10.0f);
            assert(metroid_y[k] > 0.05f);
            if (k) assert(max_y[k] > max_y[k-1]);
        }
        /* Half-line starting sections react first on BOTH halves. */
        int lightning_early = 0, lightning_late = 0;
        for (int j = 0; j < PROTO_PROFILE_SAMPLES; ++j) {
            const float base = -168.0f+336.0f*j/(PROTO_PROFILE_SAMPLES-1);
            if (base >= -15.0f && base < 0.0f)
                lightning_early += fabsf(delayed30.offsets[j]) > .001f;
            if (base >= -120.0f && base < -105.0f)
                lightning_late += fabsf(delayed30.offsets[j]) > .001f;
        }
        assert(lightning_early > 3 && lightning_late == 0);
    }

    /* Patch 0031: no x-only/y-only stretching of the eight Aria coils. */
    {
        ProtoFormula f31;
        assert(proto_parse_formula("fermat_round", &f31));
        assert(f31 == PROTO_FERMAT_ROUND);
        const float sizes[8] = {.10f,.15f,.21f,.32f,.40f,.50f,.70f,1.0f};
        float centres[8], heights[8];
        for (int i = 0; i < 8; ++i) {
            assert(fabsf(proto_fermat_round_grade(i)-sizes[i]) < .00001f);
            float start, span;
            proto_fermat_packed_cell(0.0f, 120.0f, 8, i, &start, &span);
            centres[i] = proto_fermat_round_center(0.0f, 120.0f, i, 15.0f);
            if (i) assert(centres[i] > centres[i-1]);
            float max_y = 0.0f;
            for (int k = 0; k <= 2048; ++k) {
                const float p = (float)k/2048.0f;
                float x, y;
                proto_fermat_round_point(p,start,span,0.0f,120.0f,i,
                                         15.0f,1.0f,1.0f,&x,&y);
                assert(isfinite(x) && isfinite(y));
                max_y = fmaxf(max_y,fabsf(y));
                if (k == 0) assert(fabsf(x-start) < .0001f && fabsf(y) < .0001f);
                if (k == 2048) assert(fabsf(x-(start+span)) < .0001f && fabsf(y) < .0001f);
            }
            heights[i] = 2.0f*max_y;
            assert(fabsf(heights[i]-80.0f*sizes[i]) < .30f);
            if (i) assert(heights[i] > heights[i-1]);
            /* Full-width vs height of the original Fermat is preserved:
             * two sample displacement vectors get the same x/y scale. */
            float a1x,a1y,a2x,a2y,b1x,b1y,b2x,b2y;
            const float winding = .25f+.75f*i/7.0f;
            proto_fermat_twist_point(.35f,start,span,.5f*span,
                                     1.0f,winding,1.0f,&a1x,&a1y);
            proto_fermat_twist_point(.42f,start,span,.5f*span,
                                     1.0f,winding,1.0f,&a2x,&a2y);
            proto_fermat_round_point(.35f,start,span,0.0f,120.0f,
                                     i,15.0f,1.0f,1.0f,&b1x,&b1y);
            proto_fermat_round_point(.42f,start,span,0.0f,120.0f,
                                     i,15.0f,1.0f,1.0f,&b2x,&b2y);
            const float r = proto_fermat_round_radius(i,15.0f)/(.5f*span);
            assert(fabsf((b1x-b2x)-(a1x-a2x)*r) < .001f);
            assert(fabsf(b1y-a1y*r) < .001f);
            assert(fabsf(b2y-a2y*r) < .001f);
        }
        assert(heights[7] > 79.7f && heights[7] < 80.3f);
        assert(heights[0] < 9.0f);
        assert(centres[7]+proto_fermat_round_radius(7,15.0f) <= 120.001f);
        assert(centres[0]-proto_fermat_round_radius(0,15.0f) >= -.001f);
        for (int i=0; i<8; ++i) {
            const float speed=.2f;
            assert(fabsf(proto_fermat_bothways_progress(2.5f,speed,i,.2f)-1.0f)<.0001f);
            assert(fabsf(proto_fermat_bothways_progress(5.0f,speed,i,.2f))<.0001f);
        }
        /* Inactive means exactly straight; clockwise/counterclockwise mirrors. */
        float st, sp, ax, ay, bx, by;
        proto_fermat_packed_cell(0.0f,120.0f,8,7,&st,&sp);
        proto_fermat_round_point(.32f,st,sp,0.0f,120.0f,7,15.0f,
                                 0.0f,1.0f,&ax,&ay);
        assert(fabsf(ax-(st+.32f*sp))<.0001f && fabsf(ay)<.0001f);
        proto_fermat_round_point(.38f,st,sp,0.0f,120.0f,7,15.0f,
                                 1.0f,1.0f,&ax,&ay);
        proto_fermat_round_point(.38f,st,sp,0.0f,120.0f,7,15.0f,
                                 1.0f,-1.0f,&bx,&by);
        assert(fabsf(ax-bx)<.001f && fabsf(ay+by)<.001f);
    }

    /* Patch 0032: 4 angle-independent layouts, 20%-visible last cell. */
    {
        const float halfdiag=0.5f*hypotf((float)PROTO_WIDTH,(float)PROTO_HEIGHT);
        const float largest=80.0f/proto_fermat_tall_reference_peak(7);
        assert(largest>80.0f && largest<135.0f);
        for(int v=1;v<=4;++v) {
            float lastend=0.0f,smallest=0.0f;
            for(int i=0;i<8;++i) {
                float left,width;
                proto_fermat_diagonal_cell(0,halfdiag,v,i,15.0f,&left,&width);
                if(i) assert(fabsf(left-lastend)<.0002f);
                if(i==0) smallest=width;
                if(i==7) {
                    assert(width>smallest*1.5f);
                    assert(fabsf((halfdiag-left)/width-.20f)<.0002f);
                    assert(fabsf(width-largest)<.002f);
                    assert(fabsf(2*.5f*width*proto_fermat_tall_reference_peak(i)-80.0f)<.02f);
                }
                assert(width>0.0f);
                float x0,y0,x1,y1;
                proto_fermat_diagonal_point(0,left,width,1,1,1,&x0,&y0);
                proto_fermat_diagonal_point(1,left,width,1,1,1,&x1,&y1);
                assert(fabsf(x0-left)<.001f && fabsf(y0)<.001f);
                assert(fabsf(x1-(left+width))<.001f && fabsf(y1)<.001f);
                lastend=left+width;
            }
        }
        /* Real stagger, not the old 2.25s clamp: 7*0.4 = 2.8 seconds. */
        assert(proto_fermat_offset_progress(2.5f,.2f,7,.4f)==0.0f);
        assert(proto_fermat_offset_progress(2.8f,.2f,7,.4f)==0.0f);
        assert(fabsf(proto_fermat_offset_progress(5.3f,.2f,7,.4f)-1.0f)<.0001f);
        assert(fabsf(proto_fermat_offset_progress(7.8f,.2f,7,.4f))<.0001f);
        ProtoConfig cfg32; ProtoAnimation ani32;
        proto_config_defaults(&cfg32);
        cfg32.section_count=2; cfg32.rotation_speed=0;
        cfg32.sections[0].formula=PROTO_METROID_EIGHT;
        cfg32.sections[0].phase_speed=.2f; cfg32.sections[0].stagger_delay=.4f;
        cfg32.sections[1].formula=PROTO_FERMAT_DIAGONAL;
        cfg32.sections[1].cycles=8; cfg32.sections[1].phase_speed=.2f;
        cfg32.sections[1].amplitude=15; cfg32.sections[1].stagger_delay=.4f;
        cfg32.sections[1].repeat_visible=true;
        cfg32.sections[0].repeat_visible=true;
        proto_animation_init(&ani32,&cfg32);
        ProtoFrame a32,b32;
        ani32.time=2.5f; ani32.rotation_degrees=0;
        proto_make_frame(&cfg32,&ani32,&a32);
        ani32.rotation_degrees=60;
        proto_make_frame(&cfg32,&ani32,&b32);
        /* Section geometry remains anchored to fixed baseline coordinates;
         * one can sample outside the spiral region to verify the same shape
         * and equal x/y coordinates before global rotation. */
        assert(a32.parametric && b32.parametric);
        for(int k=0;k<PROTO_PROFILE_SAMPLES;++k) {
            assert(isfinite(a32.x_coords[k]) && isfinite(a32.offsets[k]));
            assert(fabsf(a32.x_coords[k]-b32.x_coords[k])<.005f);
            assert(fabsf(a32.offsets[k]-b32.offsets[k])<.005f);
        }
        const float w0=proto_fermat_offset_progress(1.0f,.2f,0,.4f);
        const float w7=proto_fermat_offset_progress(1.0f,.2f,7,.4f);
        assert(w0>0.0f && w7==0.0f);
    }

    return 0;
}
