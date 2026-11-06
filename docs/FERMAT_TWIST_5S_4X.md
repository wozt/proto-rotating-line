# Alternating Fermat twists — patch 0022

```sh
./build/proto_rotating_line --config config/prototype_fermat_twist_5s_4x.json
```

### The requested behaviour

- 4 equally large spiral bases, with the same `amplitude` and the same growth
  envelope. The base radius is capped at 49% of the available cell width.
- Winding depth, *not size*, increases from the centre toward the exterior:
  **25%, 50%, 75%, 100%** of the maximum curl angle. At full extension the
  four spirals complete 0.875, 1.75, 2.625, and 3.5 turns respectively.
- `phase_speed=0.2` means a 5-second expansion/retraction loop: 2.5 seconds
  outward, 2.5 seconds inward. Both halves use the same sharp bell-shaped
  acceleration/deceleration. The normalized speed is proportional to the
  **cube of a parabola** (`u^3 * (1-u)^3`), noticeably punchier in the middle
  than the previous simple parabolic velocity.
- On odd-numbered loops the curl angle is negated. Direction flips only when
  the spirals are completely flat, at the 5-second cycle boundary. The return
  motion of each loop reverses that loop's development automatically.
- Existing smoothed shoulders and pointed tips remain intact. No white ribbon
  overlays are introduced.

The old `fermat_spiral` mode and 0021 JSON are preserved.

### Parameters

`amplitude`: equal maximum outer radius for all four shapes (0..48, but capped
by half the motif width). `phase_speed=0.2`: 5 seconds; `0` freezes the fully
extended shape for inspection. `cycles=4`: the four motifs, not winding turns.
