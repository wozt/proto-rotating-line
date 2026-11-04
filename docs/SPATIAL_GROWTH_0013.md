# Seam-safe spatial growth — patch 0013

Launch:

```bash
./build/proto_rotating_line --config config/prototype_spatial_growth_0013.json
```

Four **simultaneously visible** levels per half-line, not a queue of formulas.
Both sides share the same smooth 18-second cycle: flat -> developed -> flat.
The lightning reveals its detail earlier, but returns to flat with the spiral.
The rotation speed stays 10 degrees/second.

From the centre outward, size grows about **2.1×** and complexity increases:

| Position | Spiral side | Lightning side |
| --- | --- | --- |
| 1 (nearest centre) | Short sine | Small triangle |
| 2 | Larger, slightly curled wave | Sharper zigzag |
| 3 | Still larger, clearly coiled | More branches and angles |
| 4 (outer edge) | Large, pointed inward spiral | Largest multiscale lightning |

A local-axis push strengthens the outer spiral without shifting its endpoints.
Both (x,y) return to the separator at every cell boundary, even during the
morph. The seam checks and flat-clock checks are included in `test_curve`.

Unlike the earlier patch 0012 experiment, the sine baseline is multiplied by
`breath`, so the line is **exactly straight at t=0 and t=18 seconds**.
The farthest spiral has a wider radius limit but retains a decreasing radial
envelope toward a very fine tentacle-like tip. All geometry is calculated in C.
