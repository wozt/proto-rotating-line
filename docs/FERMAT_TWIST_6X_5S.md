# Six outward-growing Fermat spirals — patch 0025

Run:

```sh
./build/proto_rotating_line --config config/prototype_fermat_twist_6x_5s.json
```

The `fermat_twist` formula now interpolates the size and winding
**independently** over the number of visible cells:

| Index from centre | Radius percentage | Winding percentage | Parity |
|---|---:|---:|---|
| 0 | 50% | 25% | A |
| 1 | 60% | 40% | B |
| 2 | 70% | 55% | A |
| 3 | 80% | 70% | B |
| 4 | 90% | 85% | A |
| 5 | 100% | 100% | B |

All six share the same 5-second expansion/retraction clock:
2.5 s to full development and 2.5 s to a flat separator. The direction
also flips once per full 5-second cycle, *in addition* to alternating by
index parity. The pronounced mid-phase acceleration and generous rounded
connector shoulders from patches 0022 and 0024 are unchanged.

The size percentage is applied **after** calculating the radius that fits
one cell. This is essential: applying it before the radius clamp would make
multiple outer spirals virtually identical at six motifs per half-line.

The opposite half-line intentionally keeps four `spatial_lightning` motifs.

For all `fermat_twist` profiles, including older four-cell JSON presets,
size now progresses from 50% near the centre to 100% at the exterior.
For a four-cell profile the intermediate steps are about 66.7% and 83.3%.
The older `fermat_spiral` mode remains untouched.
