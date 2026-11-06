# Patch 0028 — Aria / Metroid visual formulas

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_8x.json
```

- Eight Aria spirals, exponentially distributed full-extension diameters:
  `w[i] = 0.5 * 2^(i/7)` for i=0..7, normalized to sum to the visible half.
  Each right root exactly touches its neighbour's left root at full extension.
  This is 50% of the outside diameter at centre and 100% at the outside.
- The winding development remains separately graded 25..100%, with the
  existing odd/even direction and cycle parity reversal.
- Each spiral starts up to 0.525 seconds later than the preceding ones;
  they all reach 100% at 2.5 seconds so they meet simultaneously.
- Four Metroid bolts use the user's formula evaluated for x=0..9. A smooth
  zero-valued edge window reconnects the waveform to the straight divider.
  The bolts start one after another (0.13 s stagger), and join at t=2.5 s.
- Original Fermat modes and other lightning profiles are preserved.

Controls/audio and launch hooks are added separately in patch 0029.
