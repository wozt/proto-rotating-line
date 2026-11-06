# Tentacle-shaped Fermat spirals — patch 0020

Run:

```sh
./build/proto_rotating_line --config config/prototype_fermat_tentacle_4x.json
```

This keeps the `fermat_spiral` formula name, but changes its *growth profile* so
it behaves more like a tentacle:

- the base grows outward earlier during the cycle
- the tip narrows more aggressively
- angular motion is concentrated near the tip, so the tip can make more turns
- the developed shape is less teardrop-like and more pointed / curled

Implementation notes:

- cycle remains 18 seconds via `phase_speed = 1/18`
- outer radius now grows approximately with `progress^0.58`
- total curl grows up to `7*pi*progress^1.18`
- along each arm:
  - radius follows `(1-u)^0.58` (broad base, sharp tip)
  - angle follows `u^2.35` (most turning happens near the tip)

The result should look more like a tentacle: a larger body near the base and a
much tighter, finer curl at the end.
