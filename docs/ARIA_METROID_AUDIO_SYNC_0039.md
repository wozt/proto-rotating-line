# Patch 0039 — button waves synchronized to the 220ms sound

The original, automatic motif animation is unchanged: five seconds per motif,
with 0.4-second propagation from the centre toward the screen edges.

An **X (Metroid)** or **Y (Aria)** press temporarily plays the same *natural*
geometry at a much faster rate (no additional spirals/lightning added):

- Per-cell deploy: 0.075s
- Per-cell retract: 0.075s
- Cell-to-cell start stagger: 0.010s
- Whole wave from first to final cell: **0.150 + 7×0.010 = 0.220s**
- Synthetic effect duration in `src/pulse_audio.c`: 0.22s, unchanged
- Holding a button: one sound/wave every 0.5 seconds, unchanged
- 0.4s automatic stagger and the horizontal **A** layout unchanged
- Sound sample generation, X/Y button mapping, and long-press launch unchanged

A short 12ms transition at the beginning and end of each cell's replay
avoids snapping abruptly from the automatic clock. Once finished, the
normal clock continues from its uninterrupted elapsed time.

Launch: `./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json`
