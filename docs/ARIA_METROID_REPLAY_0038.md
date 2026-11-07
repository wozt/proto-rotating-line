# Patch 0038 — Replay natural motion and correct obsolete pulse tests

- Fixed the obsolete 0035 test: it now triggers `aria_wave_age` and `metroid_wave_age` instead of old transient ages, while still asserting both sides are active and unchanged by rotation.
- This patch directly incorporates all 0037 changes; do NOT re-apply the failed 0037 script.
- **X / Metroid** and **Y / Aria** replay the original grow/retract curve.
  A button wave temporarily takes over the *time progression* of the existing
  curve; it does **not** add a second wave, spiral, rotation or x/y offset.
- The normal 5-second clock advances underneath. At the end of the replay,
  a short, smooth crossfade restores its current state.
- Each motif takes 1.6s for a complete beat, with 0.05s between motif starts
  (1.95s for the entire wave). The automatic animation retains 0.4s stagger.
- Held buttons still send sounds every 0.5s; visual waves are **queued** and
  played consecutively (max 8 pending), never stacked into the same geometry.
- Metroid teeth 1–8 have a clear *bipolar triangle* with positive and negative
  peaks of equal strength. Teeth 3–8 add restrained high-frequency detail;
  the source expression's triangular profile is no longer averaged away.
- Fixed horizontal A layout, outer 53.33px curl and no inward spill unchanged.

Test with:

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json
```
