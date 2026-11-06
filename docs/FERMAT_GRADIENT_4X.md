# Fermat tentacle with outward spatial growth (patch 0021)

Run:

```sh
./build/proto_rotating_line --config config/prototype_fermat_gradient_4x.json
```

The **same** two-arm Fermat-based tentacle continues to define the violet/cyan
boundary; no overlaid ribbons or outlines. Four cells on the spiral half-line
are driven in sync, with amplitude multipliers ordered from the centre to the
outside: **25%, 50%, 75%, 100%**.

The JSON `amplitude=15` means local target base radii of 3.75, 7.5, 11.25,
and 15 logical pixels, capped to **0.49 * (cell_width - gap)** so the last
base approaches its neighbouring cell without crossing its boundary. The base
now reaches closer to the section edges than patch 0020 (previous cap 0.45).

**Animation timing:** for each independent 9-second half-period, let
`u` progress from 0 to 1, and use

```
progress = 3*u*u - 2*u*u*u
velocity = 6*u*(1-u)  # parabolic in normalized half-cycle time
```

On the return half, use `u = 2*(1 - phase)`. Thus both halves independently
start and finish with zero speed, speeding up and slowing down in the middle.
The whole cycle is 18 seconds; all four coils share the same progress.

**Rounded joins:** the first 22% of each arm uses a quintic smootherstep
shoulder to introduce angular curvature gently (`C2` at the straight root).
The original 7*pi tip curl and `(1-arm_u)^0.58` point-like taper are kept.
Both ends remain exactly on the original separator.

Existing JSON presets are not changed; use `prototype_fermat_gradient_4x.json`
for the new amplitude. `phase_speed=0` freezes full extension.
