# Wolfram double ribbon, cyan and violet (patch 0017)

```sh
./build/proto_rotating_line --config config/prototype_wolfram_double_ribbon_4x.json
```

The two ribbons are **different actual colours**, not a cyan tube outlined in
white. Cyan starts in the cyan half, violet starts in the violet half. Both
winds are generated as thick tapered, independently rasterized curves with
opposite starting phases. They spiral toward the same centre and alternate
painter's order at successive half-turns to look interwoven.

Curve model (scaled to pixels):

```
r(t)     = amplitude * (1-t)^2
angle(t) = 6*pi*t
cyan(t)  = centre + r(t) * (cos(angle(t)+pi/2), sin(angle(t)+pi/2))
violet(t)= centre + r(t) * (cos(angle(t)-pi/2), sin(angle(t)-pi/2))
```

Both strands taper toward the animated endpoint and have short roots extending
from their respective half-plane to the spiral. The variable `t_limit` varies
as `0 -> 0.5 -> 0` every 18 seconds; each curve is *actually sampled only up to*
that limit, rather than scaling the final spiral.

- `ribbon_width` is each strand's full width at the base, 0.5..12 pixels;
  try `6.0` for heavier ribbons or `3.5` for more detail.
- `amplitude` sets radius, clamped to fit the cell; try `8` or `12`.
- `cycles=4` gives four double-strand motifs on one half-line.
- `show_divider=false` removes the previous dominant white separator;
  the cyan and violet bodies define the shape directly.
- On the other half-line the spatial lightning is unchanged.

Existing presets (`wolfram_halfturn`, `wolfram_tentacle`, `wolfram_ribbon`) are
not modified. All mask processing happens in portable C11; no SDL dependency.
