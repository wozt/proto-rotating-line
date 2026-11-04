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

    puts("curve_math: ok");
    return 0;
}
