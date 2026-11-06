# Wolfram double spiral / tentacle — patch 0015

```bash
./build/proto_rotating_line --config config/prototype_wolfram_tentacle_4x.json
```

The right half of the divider repeats four copies of the same continuous
**two-strand** spiral. The two arms are slightly offset, meet at the moving
inner tip, and then rejoin the horizontal separator through straight links.
The left half retains the spatial lightning effect.

The source formula is unchanged:

```
x(t) = (1-t)^2 cos(6*pi*t)
y(t) = (1-t)^2 sin(6*pi*t)
```

The *drawn parameter interval*, rather than merely the whole curve's scale,
is animated: `t_max = 0.25 * (1 - cos(2*pi*phase_speed*time))`.
Thus `t_max` goes 0 -> 0.5 -> 0, and each arc is sampled on 0 <= t <= t_max.
The second arm follows the same spiral in reverse with a small normal offset
that tapers to **zero at t_max**. This creates a pointed, connected outline.

- `phase_speed = 1/18`: 9 seconds to extend, 9 seconds to retract.
- `phase_speed = 0`: fixed full shape for visual inspection.
- `amplitude = 8`: scale of one copy in logical pixels.
- `cycles = 4`: four tentacles on the visible half-line.
- `gap_pixels = 0.35`: flat joins between motifs.
- The normal offset is 18% of the amplitude, capped at 1.25 logical pixels.

This mode is separate from `wolfram_halfturn`; previous profiles are unchanged.
