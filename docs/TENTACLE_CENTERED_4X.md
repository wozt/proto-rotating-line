# Centered tentacle spiral (patch 0010)

Use:

```bash
./build/proto_rotating_line --config config/prototype_tentacle_centered_4x.json
```

## Goals

- Four motifs per half-line.
- Lightning morphs much faster than the spiral.
- Spiral is no longer a Wind-Waker wave.
- Each spiral is compact, centred on the separator axis, and retracts like a
  tentacle curling inward until it straightens.

## Timing

- Lightning morph duration: **4.5 s**
- Spiral morph duration: **9.0 s**
- Hold between states: **0.2 s**

## Shape

The spiral cell now has three phases:

1. short flat lead-in on the separator axis
2. one centred inward curl with radius shrinking as `(1-v)^2.65`
3. flat tail back on the separator axis

This keeps the motif thin, centred, and visibly retracting when morphing back
into a straight segment.
