## Current geometry: parametric spiral demo

The `spiral` preset is now available in the two-half animated demo. It uses a double-armed Archimedean spiral with continuous connections to the straight separator on either side. A vertical ray crossing parity test keeps the violet/cyan regions distinct even when x folds back. The older graph-only limitations below describe **custom expressions**, which still use y=f(x) and cannot produce arbitrary parametric paths.

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

## Animation transitions (60 FPS automatic cycling)

Each section can now automatically cycle through multiple formulas using the
`animated` and `transitions` fields. The simulation runs at a fixed 60 FPS
timestep for deterministic animation, synchronized with SDL VSync.

### Transition structure

```json
{
  "formula": "sine",
  "amplitude": 14,
  "cycles": 2,
  "phase_speed": 0.16,
  "animated": true,
  "loop": true,
  "transitions": [
    {"formula": "sine", "duration": 2.0, "easing": "smoothstep"},
    {"formula": "straight", "duration": 2.0, "easing": "smoothstep"}
  ]
}
```

- `animated`: Enable automatic formula transitions for this section.
- `loop`: If true, transitions repeat cyclically; if false, animation stops at the last transition.
- `transitions`: Array of transition objects, each specifying:
  - `formula`: Target formula to transition to (preset name only, not custom expressions).
  - `duration`: Seconds to wait at the current formula before triggering the next transition (0.1..60).
  - `easing`: Transition smoothing function: `"linear"`, `"smoothstep"`, `"quad_in"`, `"quad_out"`, `"quad_in_out"`, `"cubic_in"`, `"cubic_out"`, or `"cubic_in_out"`.

The transition timing works as follows:
1. When `animated` is true and a morph completes, the section waits for `duration` seconds.
2. After the duration elapses, the formula smoothly morphs to the next transition's target.
3. The morph takes approximately 0.55 seconds (internal constant).
4. If `loop` is true, the cycle repeats from the first transition after the last one.

### Example: Cyclic straight → sine → straight

```json
{
  "formula": "straight",
  "amplitude": 15,
  "cycles": 1,
  "phase_speed": 0,
  "animated": true,
  "loop": true,
  "transitions": [
    {"formula": "sine", "duration": 2.0, "easing": "smoothstep"},
    {"formula": "straight", "duration": 2.0, "easing": "smoothstep"}
  ]
}
```

This configuration:
- Starts as a straight line.
- After 2 seconds, morphs to a sine wave (0.55s transition).
- Waits at sine for 2 seconds.
- Morphs back to straight (0.55s transition).
- Repeats indefinitely due to `loop: true`.

### Example: One-way transformation

```json
{
  "formula": "straight",
  "amplitude": 12,
  "cycles": 1,
  "phase_speed": 0,
  "animated": true,
  "loop": false,
  "transitions": [
    {"formula": "sine", "duration": 3.0, "easing": "smoothstep"},
    {"formula": "fractal", "duration": 4.0, "easing": "smoothstep"}
  ]
}
```

This configuration:
- Starts as straight.
- Transforms to sine after 3 seconds.
- Transforms to fractal after 4 more seconds.
- Stops at fractal (no loop).

### Example: Multi-section phase animation

Different sections can have different animation cycles, creating complex patterns:

```json
{
  "section_count": 4,
  "sections": [
    {
      "formula": "straight",
      "amplitude": 15,
      "cycles": 1,
      "phase_speed": 0,
      "animated": true,
      "loop": true,
      "transitions": [
        {"formula": "sine", "duration": 2.0, "easing": "smoothstep"},
        {"formula": "straight", "duration": 2.0, "easing": "smoothstep"}
      ]
    },
    {
      "formula": "sine",
      "amplitude": 14,
      "cycles": 2,
      "phase_speed": 0.16
    },
    {
      "formula": "straight",
      "amplitude": 15,
      "cycles": 1,
      "phase_speed": 0,
      "animated": true,
      "loop": true,
      "transitions": [
        {"formula": "fractal", "duration": 3.0, "easing": "smoothstep"},
        {"formula": "straight", "duration": 3.0, "easing": "smoothstep"}
      ]
    },
    {
      "formula": "lightning",
      "amplitude": 12,
      "cycles": 4,
      "phase_speed": 0.10
    }
  ]
}
```

This creates a rotating divider where sections 0 and 2 undergo independent
transformations while sections 1 and 3 remain static with their phase animations.

## Composing formulas with custom expressions

While transitions work with preset formulas (straight, sine, lightning, fractal),
you can create more complex behaviors using custom expressions with time-dependent
parameters. The `t` variable represents animation time in seconds.

### Time-varying amplitude

```json
{
  "formula": "sin(2*pi*cycles*u+speed*t)*(1-cos(2*pi*t/5))/2",
  "amplitude": 20,
  "cycles": 3,
  "phase_speed": 0.2
}
```

This sine wave's amplitude smoothly oscillates between 0 and the configured
amplitude over a 5-second period, creating a breathing effect.

### Morphing between wave types

```json
{
  "formula": "(1-f)*sin(2*pi*cycles*u+speed*t)+f*tri(cycles*u+speed*t)",
  "amplitude": 15,
  "cycles": 2,
  "phase_speed": 0.15
}
```

Note: This example would need a `f` variable that changes over time, which isn't
directly supported in expressions. For such transformations, use the transition
system instead, which handles morphing between different formulas automatically.

### Complex harmonic composition

```json
{
  "formula": "sin(2*pi*cycles*u+speed*t)+0.5*sin(4*pi*cycles*u+2*speed*t)+0.25*sin(8*pi*cycles*u+3*speed*t)",
  "amplitude": 18,
  "cycles": 2,
  "phase_speed": 0.1
}
```

This creates a rich wave by summing three harmonics with different frequencies
and phase speeds, approximating a Fourier series with time-dependent phase shifts.

## Limitations and future work

**Parametric curves (spirals)**: The current system represents the divider as a
single-valued graph offset from its tangent. True spirals (Archimedean, logarithmic)
require parametric curves with explicit (x, y) coordinates and robust region
classification to handle self-intersections. This is planned for a future update.

**Easing functions**: Multiple easing functions are now implemented: `linear`, `smoothstep`,
`quad_in`, `quad_out`, `quad_in_out`, `cubic_in`, `cubic_out`, and `cubic_in_out`.
These provide varied transition behaviors from linear to cubic curves.

**Custom expression transitions**: Transitions now support custom expressions using the
`expr:` prefix. For example: `{"formula": "expr:sin(2*pi*u)+0.5*sin(4*pi*u)"}`. This allows
transitioning between any valid mathematical expression.

## Preset formulas

The `formula` string may be `straight`, `sine`, `lightning`, `fractal`, `fourier`,
`chirp`, `trochoid`, or `harmonic_fractal`. They correspond to these mathematically-defined
unit displacements:

| Name | Description | Visual effect |
| --- | --- | --- |
| `straight` | `0` | Flat line, no displacement |
| `sine` | `sin(2*pi*(cycles*u + speed*t))` | Regular sine wave |
| `lightning` | `2*fract(cycles*u + speed*t)-1` | Regular saw teeth (zigzag) |
| `fractal` | Four weighted sine harmonics (freq 1, 2, 4, 8) | Self-similar detail, organic complexity |
| `fourier` | Harmonic superposition with different phase speeds | Rich wave, beating patterns |
| `chirp` | Frequency varies linearly with position | Sine wave with increasing/decreasing frequency |
| `trochoid` | Cycloid-like curve (rolling circle) | Creates loops and cyclical motion |
| `harmonic_fractal` | Six harmonics with slower decay | Finer detail, more controlled irregularity |

### Formula details

**Fourier**: Superposition of sine waves with harmonic frequencies (1x, 2x, 3x, 4x) and
independent phase speeds. Creates complex interference patterns and beating effects.

**Chirp**: A sine wave whose frequency varies linearly across the section. The local
frequency at position u is f(u) = 1 + 2u, creating a wave that compresses or expands
across the segment. Useful for creating accelerating or decelerating oscillations.

**Trochoid**: Simulates a point on a rolling circle (cycloid family). Creates loops when
the point is outside the rolling circle radius. The formula uses the y-component of
the trochoid equation, giving cyclical motion with rounded peaks.

**Harmonic Fractal**: Similar to the basic fractal but with six harmonics (1x through 6x)
and slower amplitude decay (0.6x per harmonic). This preserves more high-frequency detail
while maintaining self-similarity, creating more intricate wave patterns.

### Example: New formula showcase

```json
{
  "section_count": 4,
  "sections": [
    {"formula": "fourier", "amplitude": 12, "cycles": 2, "phase_speed": 0.15},
    {"formula": "chirp", "amplitude": 14, "cycles": 3, "phase_speed": 0.10},
    {"formula": "trochoid", "amplitude": 10, "cycles": 2, "phase_speed": 0.12},
    {"formula": "harmonic_fractal", "amplitude": 12, "cycles": 2, "phase_speed": 0.08}
  ]
}
```

This configuration demonstrates all four new formula types in different sections,
showcasing their distinct visual characteristics.

### Example: Custom expression in transition

```json
{
  "formula": "straight",
  "amplitude": 15,
  "cycles": 1,
  "phase_speed": 0,
  "animated": true,
  "loop": true,
  "transitions": [
    {"formula": "expr:sin(2*pi*u)+0.3*sin(6*pi*u)", "duration": 3.0, "easing": "cubic_in_out"},
    {"formula": "expr:0.5*sin(4*pi*u)+0.25*sin(12*pi*u)", "duration": 3.0, "easing": "cubic_in_out"}
  ]
}
```

This transitions between two custom expressions, demonstrating how to create
unique waveforms that don't match any preset formula. The `expr:` prefix indicates
a custom mathematical expression instead of a preset name.

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
normal displacement. For spirals, use the transition system or wait for the
parametric curve implementation (planned for a future update).
