# Patch 0030: tall Aria spirals, eight distinct Metroid teeth

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_tall_8x_5s.json
```

This introduces two new formulas, preserving the old `fermat_twist` and
`metroid_lightning` profiles.

- `fermat_tall`: 8 x-packed roots that still touch end-to-end at full extension.
  Their **vertical** developed height at rotation 0 runs approximately 48..96
  logical pixels (half-screen target is 80 pixels). The geometric height
  multiplier is normalized against the actual Fermat tip waveform. Gradients
  are exponential: height = 48 * 2^(index/7). Winding stays 25..100%.
- `metroid_eight`: 8 **different contiguous unit-period segments** from the
  exact user-supplied Metroid expression: [1,2], [2,3], ..., [8,9]. The left
  half starts at the center with [1,2] and ends outside with [8,9]. Each
  segment is baseline-detrended and smoothly windowed to keep the joins flat;
  the underlying shape is not replaced by synthetic lightning.
  The applied scale increases exponentially: 25% * 4^(index/7), and the source
  sigmoid also naturally increases the later teeth.
- On BOTH sides, unfolding AND retraction start at the centre and propagate
  outward with 0.2 seconds delay between each adjacent element. All elements
  still reach full deployment together at 2.5s; outer elements hold the full
  form while the centre begins retracting and then retract from 3.9s to 5.0s.
  Complete 5-second cycles remain synchronized, with direction alternations
  by element parity and cycle parity preserved on Aria.
- Keyboard/gamepad X and Y pulses, sounds and long-press launching from patch
  0029 still work with the two new formula modes.

Height targets refer to the unrotated 240x160 logical frame. Since the entire
separator rotates, apparent screen-vertical height necessarily varies with
rotation angle and can be clipped near the screen edges.

Parameter `amplitude` on `fermat_tall` scales the vertical height relative to
15.0; `amplitude` on `metroid_eight` scales the original signal.
