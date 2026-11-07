# Patch 0035 — Horizontal layout A, bounded outside spiral, staged beats

Patch 0035 supersedes the rolled-back patch 0034. It includes the same complete
geometry and input changes, with a one-microsecond boundary tolerance on pulse
termination: 3.7f - 7*0.3f may round to 1.5999999f in single precision. The
1.6-second beat is inactive exactly at its theoretical end, and tests now cover
both sides of that boundary.

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json
```

## Geometry

- Keep horizontal layout A. Its fixed half-line reference is 156 logical px.
- Seven inner cells end at x=111; the last anchor cell occupies [111,156].
  Only 9/45 (20%) of its parameter interval reaches the horizontal edge at 120.
- Last curl: maximum full transverse height **160/3 = 53.333 px**,
  rather than 80 px. Curves are not stretched independently along x and y.
- The *real inward x envelope* of the last spiral is bounded by x=111.
  Its center shifts outward by precisely what is required to prevent the coil
  from doubling back into the previous seven cells. The core can extend
  outside x=156 (off screen); the exterior connector returns to its fixed end.
- The center shift is proportional to the animated radius, so the return to
  the flat line is smooth rather than jumping at the start of deployment.
- Old B/C/D horizontal JSON files are removed. Legacy formula support remains
  in code so other older configurations are not broken.

## Input and pulses

- X keyboard / north gamepad: Metroid electric feedback and an independently
  generated *0 -> 100 -> 0* lightning motion.
- Y keyboard / west gamepad: Aria heart feedback and the same full motion on
  circular two-arm Fermat spirals.
- Each element's individual beat lasts 1.6s, starting 0.3s after its
  predecessor. Thus the eighth starts at 2.1s and ends at 3.7s.
- Holding a button still triggers every 0.5s. Eight concurrent pulses are
  retained, enough to preserve the complete delayed wave instead of cutting
  older pulses short. Long-press game launching remains unchanged.
- Audio remains shorter than 0.3s, separate from visual animation duration.
- Normal non-button animation stays on the 5s per-element clock with the
  previous 0.4s normal stagger.

The 20% criterion refers to the *last anchor interval*, not 20% of all pixels
of its curved outline: the visible amount of the actual coil varies with
angle, clipping and current deployment.
