# Rotating Line — mathematical split prototype

A **C11 + SDL3** experiment for the Metroidvania launcher, independent of
both games. Two configurable colours share a **240 × 160** logical screen
in a resizable **960 × 640** SDL window, using nearest-neighbour pixels
and aspect-ratio-preserving letterboxing. Every section is evaluated
mathematically: no textures, sprites or external artwork.

## Build on Debian 13

```sh
sudo apt install build-essential cmake ninja-build libsdl3-dev
cmake -S . -B build -G Ninja
cmake --build build -j8
ctest --test-dir build --output-on-failure
./build/proto_rotating_line
```

## JSON configuration

Edit **`config/prototype.json`** and press **R** in the SDL window to reload it.
Colors, the optional white divider, 1/2/4/8/16 section counts, rotation speed
and each section's formula, amplitude, cycles and phase speed are editable.
You can use presets *or write your own mathematical expression* in a section.

See **[docs/FORMULAS.md](docs/FORMULAS.md)** for JSON syntax, formula examples,
valid ranges and the limits of the present graph-based geometry.

For overrides on launch:

```sh
./build/proto_rotating_line --sections 16 --speed -8
./build/proto_rotating_line --config path/to/custom.json
```

## Controls

| Key | Action |
| --- | --- |
| Tab | Select next section |
| F | Cycle built-in formula on selected section; custom -> straight |
| 1 / 2 / 4 / 8 | Set section count |
| 0 | Set section count to 16 |
| L | Toggle the divider line without changing the two regions |
| R | Reload JSON without restarting (preserves animation clock and angle) |
| Up / Down | Selected section's amplitude (0–48 logical px) |
| + / - | Rotation speed in degrees per second |
| Space | Pause rotation, phases and cross-fades |
| S | Save all current settings to JSON (atomic file replacement) |
| Escape | Quit |

`F` morphs between built-in formulas in 0.55 seconds. `R` reloads all settings
immediately; a malformed JSON file produces an error while keeping the prior
working configuration.

## Next experiments

The current geometry uses **single-valued displacement graph sections**;
these always divide the screen into two regions, even when rotated. Inward
spirals fold backward and require a parametric path and a separate region
classifier. The JSON structure leaves room for that later implementation.

## Dual-half animated demo

`config/prototype.json` now shows two independent loops: a regular geometric
lightning bolt on the first half (straight -> lightning -> straight) and a
mathematical double-armed spiral on the second half (straight -> sine ->
spiral -> sine -> straight). Both follow the existing 60 Hz fixed simulation.
The parametric spiral uses `(x(u,t), y(u,t))` and ray crossing classification;
it is not a graph or a trochoid approximation. The shape consists of two
interleaved Archimedean arms, so the boundary remains open and separates the
two colours all the way across the frame.

The `duration` in JSON is the wait before triggering the given transition;
actual morphing lasts 0.55 seconds. Use `R` in SDL to load changes.
