# Compact filiform wave (patch 0009)

Run: `./build/proto_rotating_line --config config/prototype_compact_filiforme_0009.json`

This is an additional profile. It keeps the four lightning shapes and four curling
waves, the 2-pixel flat gap, 10 degrees/s rotation, and the accelerated timing of
patch 0007 (7.843 seconds for lightning morphs, 11.765 seconds for wave morphs).

Only the **wave** geometry is reduced. Lightning stays at amplitude 11.
The wave changes from amplitude 14 to 10 and uses `spiral_scale: 0.45`.
`spiral_scale` contracts both the horizontal span and the amplitude of the
parametric wave **within each existing motif cell**. The unused part of the cell
is exactly straight, and the entire section remains continuously connected.

The new key `spiral_scale` is optional on every section, ranges from 0.35 to 1,
and defaults to 1 for backwards compatibility. It applies to the `spiral`
preset only. For even smaller waves experiment with 0.40; for a slightly more
visible curl try 0.60. At the fixed 240x160 logical resolution, very small tips
will disappear into the 1-pixel raster; this cannot be resolved by further
shrinking the parametric curve without changing render resolution.
