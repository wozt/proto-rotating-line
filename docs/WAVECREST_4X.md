# Wavecrest / lightning synchronized 4x profile (patch 0006)

Launch from the repository root:

```sh
./build/proto_rotating_line --config config/prototype_windwave_4x.json
```

This configuration keeps two independent halves, each with **four** motifs,
separated by **2 logical pixels** of straight line. The overall rotation is
10 degrees/second, with a fixed 60 Hz simulation and SDL3 presentation.

The left side starts with angular three-scale lightning; the right starts
with stylized curling wavecrests (the existing `spiral` preset). The wavecrest
is a mathematical parametric separator, not an external texture:

1. A broad S-shaped rising front takes the crest to its maximum height.
2. A single inward coil follows `r(v) = r0 * (1-v)^1.55`, winding through
   `2.20*pi` radians. Radius **decreases to zero** to form a point.
3. A short return segment reconnects the tip to the baseline.
4. `curl` smoothly blends the whole parametric shape against a straight line.

Both halves follow **identical timing**: 0.6 seconds held at each state,
followed by a 20-second smoothstep morph. Each four-step cycle takes about
82.4 seconds. This avoids the 20+ seconds the lightning previously spent
near a completely flat separator while the wave was still moving.

| Step | Lightning side | Wave side |
| ---- | -------------- | --------- |
| 0 | Lightning | Curled wavecrest |
| 1 | Small triangle | Sinusoid |
| 2 | Straight | Straight |
| 3 | Small triangle | Sinusoid |
| 4 | Lightning (loop) | Curled wavecrest (loop) |

Change `amplitude` to change maximum crest height (the wavecrest stays bounded
by its cell); `cycles` to change repeats (use 4); `gap_pixels` for straight
spacing (1..5); `duration` for holding at each stage; and `morph_duration`
for the time spent **actually transforming** from one shape to another.

The original `config/prototype.json` and `config/prototype_lizard_4x.json`
remain unchanged. Press **R** to reload the active config while running.
