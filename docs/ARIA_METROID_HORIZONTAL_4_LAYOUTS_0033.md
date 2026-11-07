# 0033 — Horizontal layout and independent beats

Run with one of the four JSON profiles, no recompilation between variants:

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_b.json
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_c.json
./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_d.json
```

### Stable geometry

- Logical screen: 240x160. Horizontal visible half-width = 120 px.
- Both sides are laid out over 156 px from their shared centre, i.e.
  120 * 1.30; they extend 36 px beyond both horizontal edges.
- The 8th cell spans from 111px to 156px: its width is 45px and
  9px of it are between 111px and the visible horizontal edge at 120px.
  Therefore exactly 20% of its anchor/parameter interval is visible
  when the divider is horizontal. This does not mean 20% of the *pixels*
  of the curved spiral, whose circular windings may extend inwards.
- The first seven cells occupy 111px with variants A/B/C/D retaining their
  relative exponential weights from patch 0032. Their ends abut exactly.
- The final Aria spiral keeps its circular two-arm Fermat geometry, with
  80px peak total transverse height. The 8th curl extends outside its
  45px anchor interval; x/y are both scaled uniformly in its curl core.
- The geometry does not shift when the divider rotates. Diagonal screen
  angles can reveal additional portions of the outermost cells.
- On both sides, regular motif development still has 0.4s delay per index.
  Individual 5-second cycles are staggered by 0.4s.

### Beat input

- X keyboard / north gamepad = Metroid electric sound, 1.1s
  waveform deployment *then retraction*, independent of normal phase.
- Y keyboard / west gamepad = Aria heartbeat sound, 1.1s
  two-dimensional Fermat expansion *then retraction*.
- Each physical press immediately starts a beat. Holding triggers a new
  beat every 0.5s and long hold (>=1s) uses the matching game's configured
  launcher environment variable.
- Up to three visual beat instances per side coexist, so the 0.5s repeat
  does not cut a 1.1s beat short. The visual state is the maximum active
  envelope, not an amplification of the current normal waveform.
- The sound effect length is unchanged (<0.3s).

The old diagonal modes and their four JSON profiles remain unchanged.
