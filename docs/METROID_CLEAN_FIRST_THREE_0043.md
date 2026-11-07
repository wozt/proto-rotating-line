# Patch 0043 — Three clean Metroid triangles

The first three Metroid motifs (from the centre outwards, indices 0, 1, 2)
are now exact symmetrical piecewise-linear bipolar triangles. Their peaks are
+1 and -1, and every join is zero so differently timed cells stay connected.
There are no added electrical sine waves, sigmoid-based asymmetry or smooth
connector shoulders in these three motifs. The negative troughs have only
intentional straight-line corners; no extra local wiggles.

From the fourth motif onwards (indices 3–7), the **existing signal is
unchanged**, including its growing controlled electrical harmonics.

The patch does **not** alter audio-driven X/Y wave timing, sound generation,
ambient animation, spiral geometry or horizontal A configuration.

Validation: `cmake -S . -B build && cmake --build build -j8 &&
ctest --test-dir build --output-on-failure`.
