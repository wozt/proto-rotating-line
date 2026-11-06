# Wolfram ribbon tentacles (patch 0016)

```bash
./build/proto_rotating_line --config config/prototype_wolfram_ribbon_4x.json
```

The *wolfram_ribbon* mode fills a **single continuous tapered ribbon** rather
than displaying a pair of almost coincident spiral contours. Its centreline
is unchanged from Wolfram Alpha:

```
x(t) = (1-t)^2 cos(6*pi*t)
y(t) = (1-t)^2 sin(6*pi*t), 0 <= t <= t_max
```

`t_max = .25*(1-cos(2*pi*phase_speed*time))`: with phase_speed=1/18 the
spiral extends to t=.5 in nine seconds, then retracts to t=0 over nine more.
A static inspection is possible with phase_speed=0.

The ribbon is a union of tapered, connected round capsules along the sampled
centreline. This is a filled, closed shape, not two overlapping open strokes.
No separate inside/outside parity is inferred from overlapping curls. The
ribbon is cyan even when it crosses into the violet half; an approximately
one-logical-pixel white edge follows its visible contour. Each curled tip
narrows to zero radius. The ordinary violet/cyan separator remains intact.

- `ribbon_width` is the FULL width in logical pixels at the base (default 7),
  with 0.5..12 accepted in the JSON. Try `8.0` for a thicker tentacle or `5.0`
  for a thinner one. Its tip always tapers to zero.
- `amplitude=11.5` controls the spiral's overall diameter, independently of width.
- This preset uses `ribbon_width=6.5` so the inner coil remains readable.
- `cycles=4` places four motifs along the visible half-line.
- `gap_pixels=.35` leaves a small join between cells.
- `rotation_speed=10` keeps the earlier rotation.

The previous `wolfram_halfturn` and `wolfram_tentacle` modes are unchanged.
