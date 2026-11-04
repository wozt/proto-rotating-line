/* SPDX-License-Identifier: MIT */
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

static void overwrite(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    assert(f);
    assert(fputs(text, f) >= 0);
    assert(fclose(f) == 0);
}

int main(void) {
    const char *path = "proto_curve_test.json";
    ProtoConfig input, loaded;
    char error[256];
    proto_config_defaults(&input);
    assert(proto_config_load(PROTO_SAMPLE_CFG, &input, error, sizeof error));
    assert(input.section_count == 2);
    assert(input.sections[0].animated && input.sections[1].animated);
    input.sections[0].gap_pixels = 3.0f;
    input.sections[0].repeat_visible = true;
    input.sections[0].transitions[0].morph_duration = 7.5f;
    input.section_count = 16;
    input.rotation_speed = -15.0f;
    input.show_divider = false;
    input.violet[0] = 0x12;
    input.violet[3] = 0xBB;
    input.cyan[1] = 0x34;
    input.sections[15].formula = PROTO_CUSTOM;
    strcpy(input.sections[15].expression, "sin(2*pi*(cycles*u+speed*t))");
    input.sections[1].spiral_scale = 0.58f;
    assert(proto_config_save(path, &input, error, sizeof error));
    proto_config_defaults(&loaded);
    assert(proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 16);
    assert(fabsf(loaded.sections[1].spiral_scale - 0.58f) < 0.001f);
    assert(loaded.rotation_speed == -15.0f);
    assert(fabsf(loaded.sections[0].gap_pixels - 3.0f) < 0.001f);
    assert(loaded.sections[0].repeat_visible);
    assert(fabsf(loaded.sections[0].transitions[0].morph_duration - 7.5f) < 0.001f);
    assert(!loaded.show_divider);
    assert(loaded.violet[0] == 0x12 && loaded.violet[3] == 0xBB && loaded.cyan[1] == 0x34);
    assert(loaded.sections[15].formula == PROTO_CUSTOM);
    assert(!strcmp(loaded.sections[15].expression, input.sections[15].expression));
    overwrite(path, "{\"section_count\":2,\"sections\":[{\"formula\":\"straight\",\"gap_pixels\":6},{\"formula\":\"straight\"}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":1,\"sections\":[{\"formula\":\"spiral\",\"spiral_scale\":0.2}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":3}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 16); /* No partial mutation. */
    overwrite(path, "{\"section_count\":1,\"sections\":[{\"formula\":\"tri(cycles*u)\"}]}");
    assert(proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 1 && loaded.sections[0].formula == PROTO_CUSTOM);
    overwrite(path, "{\"section_count\":4,\"sections\":[{\"formula\":\"straight\"}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":2,\"section_count\":4}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"colors\": {\"violet\":\"#XXYYZZ\"}}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"sections\":[{\"formula\":\"sin(bad*u)\"}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"show_divider\":\"false\"}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"extra\":123}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":1,}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    remove(path);
    /* Patch 0006: both sides cycle with exactly the same timings. */
#ifdef PROTO_WAVECREST_CFG
    {
        ProtoConfig synced;
        proto_config_defaults(&synced);
        assert(proto_config_load(PROTO_WAVECREST_CFG, &synced, error, sizeof error));
        assert(synced.section_count == 2);
        assert(synced.sections[0].formula == PROTO_LIGHTNING);
        assert(synced.sections[1].formula == PROTO_SPIRAL);
        assert(synced.sections[0].transition_count == 4);
        assert(synced.sections[1].transition_count == 4);
        for (int t = 0; t < 4; ++t) {
            const ProtoTransition *a = &synced.sections[0].transitions[t];
            const ProtoTransition *b = &synced.sections[1].transitions[t];
            assert(fabsf(a->duration - b->duration) < 0.0001f);
            assert(fabsf(a->morph_duration - b->morph_duration) < 0.0001f);
            assert(a->easing == b->easing);
        }
        ProtoAnimation sync;
        proto_animation_init(&sync, &synced);
        for (int step = 0; step < 5300; ++step) {
            proto_animation_tick(&sync, &synced, PROTO_FIXED_TIMESTEP);
            assert(sync.current_transition[0] == sync.current_transition[1]);
            assert(fabsf(sync.morph[0] - sync.morph[1]) < 0.0001f);
            if (step % 500 == 0) {
                ProtoFrame frame;
                proto_make_frame(&synced, &sync, &frame);
                assert(isfinite(proto_signed_side(&frame, 120.5f, 40.5f)));
            }
        }
    }
#endif
    /* Patch 0007: loaded profile speeds up globally by x1.7, with a 2/3
     * morph-time ratio for lightning and four motifs on both halves. */
#ifdef PROTO_FILIFORM_CFG
    {
        ProtoConfig fast;
        proto_config_defaults(&fast);
        assert(proto_config_load(PROTO_FILIFORM_CFG, &fast, error, sizeof error));
        assert(fast.section_count == 2);
        assert(fast.sections[0].formula == PROTO_LIGHTNING);
        assert(fast.sections[1].formula == PROTO_SPIRAL);
        for (int side = 0; side < 2; ++side) {
            const ProtoSection *s = &fast.sections[side];
            assert(s->animated && s->loop && s->transition_count == 4);
            assert(s->repeat_visible && fabsf(s->cycles - 4.0f) < 0.0001f);
            assert(fabsf(s->gap_pixels - 2.0f) < 0.0001f);
            for (int t = 0; t < 4; ++t) {
                float expected = side == 0 ? 7.843f : 11.765f;
                assert(fabsf(s->transitions[t].morph_duration - expected) < 0.001f);
                assert(fabsf(s->transitions[t].duration - 0.353f) < 0.001f);
            }
        }
        assert(fabsf(fast.sections[0].transitions[0].morph_duration * 1.5f -
                     fast.sections[1].transitions[0].morph_duration) < 0.002f);
    }
#endif
    /* Patch 0011: the JSON loads spatial progression without transition lists. */
#ifdef PROTO_SPATIAL_CFG
    {
        ProtoConfig graded, again;
        proto_config_defaults(&graded);
        assert(proto_config_load(PROTO_SPATIAL_CFG, &graded, error, sizeof error));
        assert(graded.section_count == 2);
        assert(graded.sections[0].formula == PROTO_SPATIAL_LIGHTNING);
        assert(graded.sections[1].formula == PROTO_SPATIAL_WAVE);
        for (int side = 0; side < 2; ++side) {
            assert(graded.sections[side].transition_count == 0);
            assert(!graded.sections[side].animated);
            assert(graded.sections[side].repeat_visible);
            assert(fabsf(graded.sections[side].cycles - 4.0f) < 0.0001f);
            assert(fabsf(graded.sections[side].gap_pixels - 0.5f) < 0.0001f);
            assert(fabsf(graded.sections[side].phase_speed - (1.0f/18.0f)) < 0.0001f);
        }
        assert(proto_config_save(path, &graded, error, sizeof error));
        proto_config_defaults(&again);
        assert(proto_config_load(path, &again, error, sizeof error));
        assert(again.sections[0].formula == PROTO_SPATIAL_LIGHTNING);
        assert(again.sections[1].formula == PROTO_SPATIAL_WAVE);
        remove(path);
    }
#endif
    /* Patch 0013: outward-growth JSON preset. */
#ifdef PROTO_SPATIAL_GROWTH_0013
    {
        ProtoConfig growth;
        proto_config_defaults(&growth);
        assert(proto_config_load(PROTO_SPATIAL_GROWTH_0013,
                                 &growth, error, sizeof error));
        assert(growth.section_count == 2);
        assert(growth.sections[0].formula == PROTO_SPATIAL_LIGHTNING);
        assert(growth.sections[1].formula == PROTO_SPATIAL_WAVE);
        for (int side = 0; side < 2; ++side) {
            const ProtoSection *s = &growth.sections[side];
            assert(s->cycles == 4.0f && s->repeat_visible);
            assert(!s->animated && s->transition_count == 0);
            assert(fabsf(s->gap_pixels - 0.35f) < 0.001f);
            assert(fabsf(s->phase_speed - 1.0f / 18.0f) < 0.001f);
        }
        assert(growth.sections[1].amplitude == 6.2f);
        assert(growth.sections[0].amplitude == 7.5f);
    }
#endif
    puts("config_roundtrip: ok");
    return 0;
}
