# Patch 0031: circular eight-stage Aria curls

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_round_8x_5s.json
```

The `fermat_round` formula replaces the height-only transformation used by
`fermat_tall`. Each curve's *coiled portion* uses one uniform 2D scale factor.
This preserves the original Fermat/tentacle geometry without squashing circles
into vertical ovals.

### Eight size levels, from the center toward the exterior

10%, 15%, 21%, 32%, 40%, 50%, 70%, 100%.

The user listed seven percentages for eight motifs. 40% is inserted between 32% and 50%, keeping 10% as the minimum.

The highest level is normalized to 80 logical pixels total VERTICAL extent at
zero global rotation (from -40 to +40 around the line). `amplitude=15.0`
selects this default; other values scale all eight levels proportionally.
The winding depths stay 25%..100% across the eight positions.

### Constraint: large round motifs must overlap

Eight round motifs of these proportions cannot all be tangent without overlap
inside one 120px half-line: their full projected widths add up to more than
240px even before accounting for the Fermat geometry. The patch preserves
circle-like undistorted curves, orders their centers from the line middle
outwards, and lets their coiled areas overlap. Each flat connector remains
continuous at its original baseline boundary. At the maximum curl, the outer
motif stays inside the 120px half-line at zero global rotation; rotation can
change how much of the shape is visible near the screen edge.

The previous `fermat_tall` mode and profiles are untouched, as is the Metroid
8-stage waveform. This profile boosts its amplitude from 24 to 30. All 5-second clocks, index/cycle direction alternation,
0.2s deployment/retraction delays and X/Y sound/launch interactions remain.
