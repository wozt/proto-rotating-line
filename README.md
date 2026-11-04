# Rotating Line — mathematical split prototype

A tiny **C11 + SDL3** experiment for the Metroidvania launcher, independent
of both games. Two solid colours (purple / cyan) share a **240 × 160** logical
screen in a resizable **960 × 640** SDL window, using nearest-neighbour pixels
and aspect-ratio-preserving letterboxing. No textures, shaders, sprites,
external assets or random geometry are involved: the separator is computed
from math functions.

## Build on Debian 13

```sh
sudo apt install build-essential cmake ninja-build libsdl3-dev
cmake -S . -B build -G Ninja
cmake --build build -j8
ctest --test-dir build --output-on-failure
./build/proto_rotating_line
```

You can start with `--sections 2`, `--sections 4` or `--sections 8`, set
`--speed 10` (degrees per second; negative reverses direction), and provide
`--config path/to/file.cfg`. The default file is `config/prototype.cfg` if
present; otherwise built-in defaults are used. Command-line overrides take
precedence over the file regardless of argument order.

## Controls

| Key | Action |
| --- | --- |
| Tab | Select the next section |
| F | Change the selected section's formula (0.55 s cross-fade) |
| 2 / 4 / 8 | Select the number of sections |
| Up / Down | Selected section's amplitude (0–48 logical px) |
| + / - | Rotation speed, in degrees per second |
| Space | Pause the rotation, phases and cross-fades |
| S | Save all section settings to the active configuration file |
| Escape | Quit |

Each section uses `sectionN = formula, amplitude, cycles, phase_speed`.
Allowed formulas are `straight`, `sine`, `lightning` (regular sawtooth) and
`fractal` (four deterministic harmonic octaves). `cycles` is the wave count
within one section; `phase_speed` moves the phase in cycles per second.
Sections join continuously at zero displacement. Phase animation and rotation
are independent. Colour selection is based on the sign of the curve in its
rotated local frame.

## Next experiments

This skeleton deliberately implements **single-valued graph sections**.
They can always separate the screen cleanly into two regions without
flood-fill; even fairly intricate harmonic deformations stay mathematically
well defined. A **real Archimedean spiral** folds back on itself and cannot
be represented as one `normal_offset(tangent)` function. It will need a
separate parametric-curve representation and a corresponding region classifier
(or a rasterized mask). That is the next stage, not a fake spiral hidden in
this first version.
