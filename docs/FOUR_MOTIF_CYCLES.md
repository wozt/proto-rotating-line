# Four tapered curls and angular lightning bolts

Run `./build/proto_rotating_line --config config/prototype_lizard_4x.json`.
The 4 motifs per visible half use `cycles: 4` with 2 px flat gaps.

Both halves start with the finished shape **visible immediately**, so it is
possible to compare lightning and spiral without waiting for the first morph.

- Negative half: lightning -> small triangle -> straight -> lightning.
- Positive half: single-arm tapered spiral -> sine -> straight -> sine -> spiral.
- `morph_duration`: 20-22.5 seconds for primary stages (6 s for brief sine).
- `duration`: dwell before the next morph, not morph duration.

The spiral now uses a single inward arm (never the previous twin-arm shape).
Its radius decreases as a power law to a mathematical tip; the separate
straight exit connector keeps each repeat a continuous two-region divider.
The `curl` parameter winds and unwinds the angle and offset from the baseline,
without applying the morph factor twice. The appearance is intentionally
pixel-scale at logical resolution 240x160 and is best inspected without
interpolation, with `line_width: 1.0` and nearest-neighbour scaling.
