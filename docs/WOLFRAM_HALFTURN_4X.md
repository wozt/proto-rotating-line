# Wolfram spiral (patch 0014)

Launch:

```bash
./build/proto_rotating_line --config config/prototype_wolfram_halfturn_4x.json
```

The right half-line uses a true parametric curve, unlike a y=f(x) `custom`
expression:

```
x(t) = (1-t)^2 * cos(6*pi*t)
y(t) = (1-t)^2 * sin(6*pi*t)
0 <= t <= 0.5
```

One copy of that **exact arc** is scaled by `amplitude` and translated to the
middle of each cell. To keep the divider connected, short horizontal lead-in
and exit segments are added; they are not part of the Wolfram equation.
The scale is capped to 42% of cell width to avoid overlapping adjacent cells.

- `amplitude: 8` sets the outer radius in logical pixels (up to cell cap).
- `cycles: 4` repeats four arcs on the visible right half-line; use 1 for one.
- `phase_speed: 0` shows the full curve continuously, ideal for shape checks.
- For a 18-second unfold/refold animation, set `phase_speed: 0.05555556`.
- `gap_pixels: 0.35` leaves short flat joins.

The left half-line retains the existing spatial-lightning formula. Rotation
remains 10 degrees/second. Existing JSON profiles are unchanged.
