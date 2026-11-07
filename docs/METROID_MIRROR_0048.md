# Patch 0048 — Mirror the Metroid side by time and parity

The Metroid half now follows the same inversion logic as the Aria/Fermat half.
Each triangle cell uses a **polarity** multiplier:

- the time sign flips every complete 5 s cycle through `proto_fermat_twist_direction`,
- and neighbouring cells alternate sign through odd/even parity.

Formula:

`polarity = proto_fermat_twist_direction(t - delay*i, phase_speed) * ((i & 1) ? -1 : 1)`

This creates a mirror-like behaviour around the central line. For every
fully active wave (for example t=4 s and t=9 s for the default 0.4 s
stagger), neighbouring teeth point in opposite directions. Each tooth
reverses at the end of its own 5 s cycle, when it is flat, preventing an
instantaneous polarity jump on a deployed tooth. During the 2.8 s transition
window, neighbours can temporarily share orientation, as the staggered
reversals propagate outward. This matches the Aria-side direction formula.

Nothing else changes:
- same triangle silhouettes,
- same ambient timing,
- same X pulse system,
- same amplitudes and spatial layout.

Run:

```sh
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build --output-on-failure
```
