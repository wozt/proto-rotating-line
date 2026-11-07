# Patch 0045 — Cropped first four Metroid triangles and corrected bipolar tests

The user still saw an unwanted little harmonic at the bottom of the early
Metroid valleys. Instead of trying to smooth it away, the first four teeth
(indices 0–3, from the centre outwards) are now **cropped**: only the rising
front and the falling front are kept. The left and right root segments are
forced to a flat zero baseline.

This makes the first visible triangles read as clean spike-like triangles:
- flat baseline on the left,
- linear rise,
- single clean apex,
- linear fall,
- flat baseline on the right.

Teeth 5–8 (indices 4–7) keep the existing electrical character.

The previous Patch 0038 bipolar test now checks **zero negative dip** on
cropped teeth 1–4 and keeps the strict ±1 requirement for electric teeth 5–8.
Both groups still must have a strong positive crest and maintain their shape.

No changes to Aria spirals, sound-driven button waves, audio duration sync,
or horizontal-A placement.
