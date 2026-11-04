# Formula configuration (C11 math only)

The default configuration lives in [`config/prototype.json`](../config/prototype.json).
The file is **strict JSON**, so there are no `//` or `/* */` comments inside it.
Put examples here instead. Edit the JSON while the window is open, then press
**R** to reload; **S** writes the current settings back to JSON (overwriting any
manual edits not yet reloaded).

## Global settings

```json
{
  "section_count": 4,
  "rotation_speed": 10.0,
  "line_width": 1.0,
  "show_divider": true,
  "colors": {
    "violet": "#8737CD",
    "cyan": "#19DBE8",
    "divider": "#FFFFFF"
  },
  "sections": [
    {"formula": "sine", "amplitude": 14, "cycles": 3, "phase_speed": 0.16}
  ]
}
```

- `section_count` is exactly **1, 2, 4, 8 or 16**. `sections` must contain at
  least that many entries; the starter file contains all 16, with index 0
  representing the first section from one end of the divider.
- `rotation_speed`: **-360..360 degrees/second**; negative reverses direction.
- `line_width`: **0..4 logical pixels**; `show_divider: false` hides the center
  line entirely, without affecting the split into two colors.
- `colors.violet`, `.cyan`, `.divider`: CSS-like hexadecimal `#RRGGBB`
  (optional `#RRGGBBAA` alpha). These are the two *region* colors, not
  colors for individual sections. Name `violet` refers to the negative side
  of the mathematical divider; `cyan` to the positive side.
- `amplitude`: **0..48 logical pixels**; `cycles`: **0.25..12**;
  `phase_speed`: **-4..4 cycles/second**. Each section has independent values.

## Preset formulas

The `formula` string may be `straight`, `sine`, `lightning`, or `fractal`.
They correspond to these mathematically-defined unit displacements:

| Name | Underlying displacement (before envelope/amplitude) |
| --- | --- |
| `straight` | `0` |
| `sine` | `sin(2*pi*(cycles*u + speed*t))` |
| `lightning` | `2*fract(cycles*u + speed*t)-1` (regular saw teeth) |
| `fractal` | Four weighted sine harmonics with frequency 1, 2, 4, 8 |

## Custom mathematical expressions

Instead of a preset, set `formula` to an **expression string**.
This expression is evaluated from **C math only**, not JavaScript or Python.
Each value is multiplied by `amplitude` and by the endpoint envelope
`sin(pi*u)^2`, and clipped to [-1, 1] before that multiplication. This keeps
sections connected and prevents gaps; it also softens the first and last
teeth of a lightning segment.

The expression has these variables:

| Variable | Meaning |
| --- | --- |
| `u` | Local section coordinate, from 0 to 1 |
| `t` | Animation time in seconds (frozen when paused) |
| `pi` | 3.14159265... |
| `amp` | The section's amplitude; generally leave it out (already applied outside the expression) |
| `cycles` | The section's cycles value |
| `speed` | The section's phase_speed value |

Operators: `+`, `-`, `*`, `/`, `^` (power), parentheses, unary minus.
Functions: `sin(x)`, `cos(x)`, `abs(x)`, `sqrt(x)`, `floor(x)`,
`fract(x)`, `tri(x)`, `min(x,y)`, `max(x,y)`, `pow(x,y)`.
`fract(x) = x - floor(x)` and `tri(x) = 1 - 4*abs(fract(x)-0.5)`.
Expressions are limited to 159 characters and must be valid for the sample
coordinates checked on load. Non-finite values in subsequent animation
frames fall back to zero, rather than crashing the viewer.

Copy any example below into the `formula` field of a section:

```json
{"formula": "0", "amplitude": 0, "cycles": 1, "phase_speed": 0}
```

```json
{"formula": "sin(2*pi*(cycles*u+speed*t))", "amplitude": 14, "cycles": 3, "phase_speed": 0.2}
```

```json
{"formula": "tri(cycles*u+speed*t)", "amplitude": 14, "cycles": 5, "phase_speed": 0}
```

```json
{"formula": "2*fract(cycles*u+speed*t)-1", "amplitude": 14, "cycles": 4, "phase_speed": 0.1}
```

```json
{"formula": "0.5*sin(2*pi*cycles*u+speed*t)+0.25*sin(8*pi*cycles*u+2*t)+0.125*sin(16*pi*cycles*u+3*t)", "amplitude": 20, "cycles": 2, "phase_speed": 0.1}
```

```json
{"formula": "sin(2*pi*cycles*u+2*t)*cos(4*pi*cycles*u-0.5*t)", "amplitude": 16, "cycles": 2, "phase_speed": 0.1}
```

Important limitation: the current viewer represents the divider as a
**single-valued graph** relative to its rotating tangent. A genuine inward
spiral/escargot folds back on itself; that requires a later **parametric
curve with region classification**, not a graph expression that only controls
normal displacement. This first JSON step does not pretend otherwise.
