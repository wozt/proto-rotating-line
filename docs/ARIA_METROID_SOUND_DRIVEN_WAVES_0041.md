# Patch 0041 — single source of truth for sound and button wave timing

An X (Metroid) or Y (Aria) press queues the original generated PCM audio.
`proto_pulse_audio_emit()` now returns the **actual emitted PCM frame count / sample
rate** in seconds. The visual animation uses this returned length, not a second
hardcoded duration. It still works if no audio device is available.

For a clip duration `T`:

- Start delay between adjacent elements: `T / 22`.
- Each element deploys for `15*T/44`, then retracts for `15*T/44`.
- Each element's full active span: `15*T/22`.
- Eight-element total span: `15*T/22 + 7*T/22 = T`.
- Entry/exit interpolation is also scaled to that element's duration.
- The ambient animation continues on its independent five-second cycle,
  with the existing 0.4-second interval. No geometry is superposed.
- Existing keyboard/gamepad assignments, long-hold launch and A layout stay put.

At the current sample length of 10560 frames at 48000 Hz, `T = 0.220 s`:
0.150 s per element, 0.010 s delay, 0.220 s for the complete wave.
If generated/replacement audio logic changes the PCM length, use the returned
sample count to update the timing automatically. The app currently synthesizes
its sounds; an audio-file picker is not implemented by this patch.

The patch also repairs two pre-existing `test_curve` fixtures inherited from
0035 and 0038. They set their wave ages without setting clip durations; since
PCM synchronization, zero-duration events are intentionally inactive.
They now explicitly use the 220ms reference clip and still verify that both
sides move and that each button affects only its own side. No visibility
assertions are removed or weakened.

Queued button waves store the duration of each corresponding sound event,
so clips of different lengths can be serialized correctly on one button.

Run: `./build/proto_rotating_line --config config/prototype_aria_metroid_horizontal_a.json`
