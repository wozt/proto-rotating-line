# Fermat spiral separator — patch 0019

Run:

```sh
./build/proto_rotating_line --config config/prototype_fermat_4x.json
```

One continuous TWO-ARM Fermat spiral replaces every motif on the right-hand
half-line. Four identical motifs are visible, while the opposite half uses
the existing spatial lightning profile.

MATHEMATICS:

```
  theta_max = 3*pi * p(time)
  p(time)   = (1 - cos(2*pi*time/18)) / 2
  a         = min(amplitude, 0.45*cell_length)/sqrt(3*pi)
  r(theta)  = a*sqrt(theta)              (0 <= theta <= theta_max)
  arm +/-   = centre +/- r(theta)*(cos(theta-theta_max), sin(theta-theta_max))
```

We rotate the *whole* Fermat curve by -theta_max so that its outer endpoints
always lie on the line at -r_max and +r_max, exactly horizontal. Two short
straight connectors rejoin the original separator at the cell edges. Thus the
curve is a single uninterrupted colour boundary, with no overlay or outline.

The size grows as sqrt(p), while the number of turns grows linearly with p.
On the return half-cycle both recede together. The positive and negative arms
are mathematically the *same* Fermat curve with r = +/- a*sqrt(theta).

TUNING:

- `amplitude`: maximum outside radius, capped at 45% of the motif width
- `cycles: 4`: four motifs over the visible half-line
- `phase_speed: 0.05555556`: 18-second expand/retract cycle
- `phase_speed: 0`: hold at fully developed form
- `rotation_speed`: rotates the entire separator (including the spirals)
- `show_divider: false`: pure cyan/violet border, without white strokes

Previous `wolfram_*` and `pinched_rope` modes remain available unchanged.
