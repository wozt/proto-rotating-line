# Spatial gradient: four progressive motifs on each half-line

Run `./build/proto_rotating_line --config config/prototype_spatial_gradient_4x.json`.

This is **one 18-second continuous expansion/contraction cycle**, not a queue of
`spiral -> sine -> straight` transitions.  All four motifs are drawn in each
half-line *at the same time*, using one global clock. The final portion is
always the most elaborate; the first is a simple undulation.

At maximum extension, left-to-right along the four cells:

| Portion | Wave half-line | Lightning half-line |
|---|---|---|
| 1 | Simple sine | Simple triangular stroke |
| 2 | Slightly curling wave | Moderate angular kink |
| 3 | More pronounced tentacle curl | Several sharp bends |
| 4 | Centered, thin, pointed spiral | Full multi-scale lightning |

From `t=0` to `t=9 s` the common deformation grows from flat through a sine /
soft triangle and progressively into the detailed shapes. From `t=9` to
`t=18 s` this process reverses continuously; there is no jump between modes
and no separate on/off transition per type or portion.

The lightning deformation reaches its full intensity earlier, at 2/3 of the
wave's expansion, but both sides share the same 18 s loop and flat endpoints.
The main rotation remains 10 degrees/second.

Implementation notes:
- `spatial_wave` and `spatial_lightning` are explicit JSON formulas.
- `cycles=4` means exactly four different levels, not four identical copies.
- `phase_speed=1/18` sets one progression cycle per 18 seconds.
- The tentacle has a centred, single continuous curl with a radius tending to
  zero at the tip, and interpolates to its sine baseline instead of switching.
- `gap_pixels=0.5` preserves narrow flat joins without large spaces.
