# Eight packed Fermat tentacles — patch 0027

```sh
./build/proto_rotating_line --config config/prototype_fermat_twist_packed_8x_5s.json
```

## Geometry — actual variable positions, not eight fixed slots

With 8 motifs on the spiral half, the peak base radius is proportional to
`size(i) = 0.50 + 0.50*i/7` (index 0 at the screen centre, 7 at the outside).
The 8 weights sum to exactly 6.0. Every cell gets a width proportional to
its weight, so it owns a correspondingly sized spiral:

`width(i) = visible_half_span * size(i) / 6`.

`start(i) = visible_half_start + sum(width(j), j < i)`.

`centre(i) = start(i) + width(i)/2`.

At the fully deployed frame (time 2.5 s) the base radius equals **width/2**.
Therefore each rightmost root is exactly the next motif's leftmost root:
`centre(i) + radius(i) = centre(i+1) - radius(i+1)`.
No gaps and no overlaps are introduced by fixed centre spacing. The last
motif ends precisely at the visible half-line end. When retracted, the
radii shrink within those allocated, non-overlapping segments.

At horizontal rotation the visible half is 120 logical pixels, yielding
peak diameters of 10, 11.43, 12.86, 14.29, 15.71, 17.14, 18.57, 20 pixels.
Compared to the six-motif mode, even the smallest and largest end motifs
are about 3.3% bigger, which is the maximum simultaneous increase that fits
an additional two motifs into the same fixed half-line without overlap.

## Independent winding and direction

Winding percentage: `0.25 + 0.75*i/7` (25% to 100% in 8 regular steps).
This changes the curl angle, never the radius or centre. The 5-second,
strongly eased expand/retract clock is common to all eight motifs, while
rotation direction alternates by BOTH index parity and cycle parity.

The new variable-packing behaviour applies only when `fermat_twist` is set
to `cycles: 8`. Older 4/6-motif Fermat profiles and other formula modes are
unchanged. `gap_pixels` is set to 0 because the peak endpoints touch.
The older 0025 six-spiral config remains available.
