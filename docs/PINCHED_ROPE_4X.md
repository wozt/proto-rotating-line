# Pinched rope double-ribbon (patch 0018)

```sh
./build/proto_rotating_line --config config/prototype_pinched_rope_4x.json
```

This mode replaces the previous interwoven twin spiral with a **pinched rope**
shape:

- imagine a straight rope segment
- pinch it at the middle of each cell
- rotate the pinched point
- two mirrored semi-spirals appear simultaneously, one to the left and one to
  the right of the pinch

The implementation still uses **two coloured strands** (cyan and violet), but
both now follow the same rope geometry instead of behaving like two independent
spirals. They are offset around a common centreline using the local normal.

The cell stays attached to the flat separator at both ends. The deformation is
maximum near the middle and vanishes at the cell boundaries.

Animation:
- shared cycle of 18 seconds
- `t_limit` evolves `0 -> 0.5 -> 0`
- the rope twists out from the pinch and then retracts back to a flat line

Useful tuning values:
- `amplitude`: how large the semi-spirals become
- `ribbon_width`: thickness of each coloured strand
- `gap_pixels`: flat spacing between cells
