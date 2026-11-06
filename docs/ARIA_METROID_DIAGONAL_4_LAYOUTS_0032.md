# Patch 0032: fixed diagonal and four spiral layouts

**Run one of the four profiles without recompiling:**

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_diagonal_a.json
./build/proto_rotating_line --config config/prototype_aria_metroid_diagonal_b.json
./build/proto_rotating_line --config config/prototype_aria_metroid_diagonal_c.json
./build/proto_rotating_line --config config/prototype_aria_metroid_diagonal_d.json
```

- Coordinate reference is always the full screen diagonal `hypot(240,160)`;
  each half-line has constant length 144.222 pixels, independent of rotation.
- Layout `spiral_layout`: 1=A geometric; 2=B progressive;
  3=C accelerated; 4=D explosive.
- The largest spiral's unrotated Y envelope is **80 logical pixels**, not
  its along-axis width: actual outer cell width is `80 / max_y_unit_winding7`.
- The outer cell starts so that only its first **20 percent along its line
  axis** lies before the diagonal corner. Other parts may enter the viewport
  due to 2D rotation or curved extent; 20% is an anchor-axis target, not a
  universal pixel-clipping guarantee for every angle.
- The seven other cell widths are weighted and normalized to fill the length
  from the screen centre to the beginning of this outermost cell.
- Original Fermat 2D geometry is preserved (no unequal x/y scaling).
  Root connectors are shortened by an arc-length parameter remap so part of
  the enormous last spiral actually appears in the 20% visible interval.
- `stagger_delay: 0.4` applies independently to BOTH Aria and Metroid.
  Each of eight shapes has a 5-second rise-and-fall cycle, shifted by
  i * 0.4 seconds from the centre. The full first wave takes 7.8 seconds
  until the last shape retracts; it cannot all peak at 2.5s with this delay.
- Metroid source curve (eight sampled unit segments), X/Y pulses, synthesized
  sounds and game-launch long-press remain unchanged.
- Previous formulas, profiles and legacy `0.2 s` delay are untouched.

### Formula for the positions

`D = hypot(240,160)`, `L = D/2`, `W8 = 80 / unit_peak(7)`,
`X8 = L - 0.20*W8`, `W_i = X8*weight_i/sum(weight_1..weight_7)`.
The 8th shape spans `[X8,X8+W8]` in local screen-diagonal coordinates.

### Weights of the first seven shapes (centre -> outer)

| Layout | Weights |
|---|---|
| A | 3.0, 4.8, 7.6, 12.2, 19.5, 31.2, 50.0 |
| B | 6.0, 6.9, 8.9, 12.4, 18.4, 28.7, 46.9 |
| C | 8.4, 8.8, 10.1, 12.7, 17.6, 26.6, 44.0 |
| D | 11.6, 11.7, 12.2, 13.5, 16.7, 23.5, 39.2 |

Other `amplitude` values rescale the outer spiral but leave the reference
`80px` result at the JSON default `amplitude=15.0`.
