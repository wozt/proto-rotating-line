# Filiform wave / accelerated lightning (patch 0007)

Use `./build/proto_rotating_line --config config/prototype_filiforme_4x.json`.

Four motifs per half-line, 2 logical pixels of straight spacing, 10 degrees/s
rotation, and unchanged 60 Hz fixed simulation. The wave preset uses a taller,
slender rising contour and a single curl with a mathematically narrowing tip.
The inner radius follows `(1-v)^1.25` multiplied by a smooth fade from
1.0 to 0.35 near the tip; at v=1 its radius is exactly zero. All configurations
that use the built-in `spiral` preset will inherit this geometric refinement.

Both phase sequences remain unchanged:
- Lightning -> triangle -> straight -> triangle -> lightning
- Curled wave -> sine -> straight -> sine -> curled wave

The previous profile held each stage for 0.6 s and morphed for 20 s.
This profile accelerates **both hold and morph times by 1.7**:

| Parameter | Wave | Lightning |
| --- | ---: | ---: |
| Before morph speed-up | 20.00 s | 20.00 s |
| After x1.7 speed-up | 11.765 s | 7.843 s |
| Relative morph time | 3/3 | 2/3 |
| Hold per stage | 0.353 s | 0.353 s |

Because lightning has deliberately shorter morphs, its own complete cycle
is now shorter than the wave cycle; the two shapes no longer remain phase-locked.
The rotation speed remains **10 degrees/s**. For a synchronized but shortened
lightning morph, a later profile could allocate the remaining time to holds.

This is an **additional** JSON profile; previous JSON files are preserved.
The image remains a mathematically evaluated, pixelated 240x160 separator.
