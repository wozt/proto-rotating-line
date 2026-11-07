# Patch 0036 — Pulses as additive overlays and bipolar Metroid lightning

Run `./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json`.

- The **normal animation is never overridden** by a button press.
- **X** (Metroid, north gamepad): adds a separate animated electric waveform.
  **Y** (Aria, west gamepad): adds a second circular Fermat curl displacement.
- Each independent motif's pulse goes 0 → 1 → 0 in **1.6 seconds**.
  Across eight motifs, the button wave starts **0.05 s** later per step:
  the last starts at 0.35 s and finishes at 1.95 s.
- The 5-second automatic animation retains its independent 0.4-second
  stagger and continues while button pulses are drawn on top.
- A held button emits new 1.6-second waves every 0.5 s; up to eight recent
  pulses coexist. The 1-second long-press still attempts the configured game.
- Metroid teeth 1 and 2 remain relatively simple. **Starting at tooth 3**,
  a two-frequency, high-energy electrical component is added. Each tooth's
  waveform has its DC bias removed while preserving zero-valued joins, then
  **positive and negative peaks are normalized separately** to equal height.
  The pulse adds a moving electrical pattern; it does not multiply the
  old static waveform's amplitude.
- The horizontal A layout, fixed ±156px positions, the outer 53.33px curl,
  and its inward containment bound are unchanged.
- This patch does not touch existing JSON presets or remove any files.
