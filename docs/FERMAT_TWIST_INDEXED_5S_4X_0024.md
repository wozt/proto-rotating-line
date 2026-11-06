# Indexed Fermat twists — patch 0024

```sh
./build/proto_rotating_line --config config/prototype_fermat_twist_indexed_5s_4x_0024.json
```

This patch refines the `fermat_twist` mode from patch 0022 (patch 0023 not required).

## What changes

- The four spirals now keep the **same development clock** and the same 5-second
  expand/retract timing, but they no longer all have the same physical size.
- From the centre toward the exterior on the spiral half, their **maximum size**
  is now:
  - 100%
  - 85%
  - 70%
  - 55%
- Their **winding development** still grows independently on the same time span:
  - 25%
  - 50%
  - 75%
  - 100%
- Rotation direction alternates **in time AND by index parity**:
  each 5-second cycle flips the global winding direction, and in each cycle:
  - cell 0: one direction
  - cell 1: opposite direction
  - cell 2: first direction again
  - cell 3: opposite direction again
- The root/connector shoulder is made more generous so the transition from the
  straight line into the spiral is visibly rounder and less abrupt.

The 5-second cycle remains in place (`phase_speed = 0.2`):
- 2.5 s expansion
- 2.5 s retraction

The old 0022 profile remains usable, but this new profile is the recommended one.
The 5-second speed curve and its sharp mid-phase acceleration are unchanged.
