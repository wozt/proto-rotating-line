/* SPDX-License-Identifier: MIT */
/* Timing from actual generated PCM frames, without opening a sound device. */
#include "pulse_audio.h"
#include "parametric.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void check_clip(bool aria) {
    const float duration = proto_pulse_audio_emit(NULL, aria);
    const float delay = proto_button_wave_delay(duration);
    const float cell = proto_button_wave_cell_duration(duration);
    assert(isfinite(duration) && duration > 0.0f);
    assert(fabsf(cell + 7.0f*delay - duration) < .000002f);
    assert(fabsf(proto_fermat_audio_progress(0.5f*cell, duration)-1.0f) < .0001f);
    for (int n = 0; n < 8; ++n) {
        const float start = n*delay;
        assert(proto_fermat_audio_progress(-0.0005f, duration)<0.0f);
        assert(fabsf(proto_fermat_audio_progress(0.0f, duration))<.00001f);
        const float at_peak = start + 0.5f*cell;
        assert(fabsf(proto_fermat_audio_progress(at_peak-start,duration)-1.0f) < .0001f);
        if (n == 7) assert(fabsf(start+cell-duration)<.000002f);
        assert(proto_fermat_audio_progress(cell - .001f,duration)>=0.0f);
        assert(proto_fermat_audio_progress(cell + .001f,duration)<0.0f);
    }
}

int main(void) {
    check_clip(true);
    check_clip(false);
    puts("pulse_audio_duration: ok");
    return 0;
}
