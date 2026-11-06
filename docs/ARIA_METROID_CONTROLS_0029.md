# Patch 0029 — input / audio / launch

Run the visual profile from patch 0028:

```sh
./build/proto_rotating_line --config config/prototype_aria_metroid_8x.json
```

## Input (SDL3)

- Keyboard **X** = Aria (spirals), **Y** = Metroid (electricity).
- Nintendo layout gamepad: **X** (top/north) = Aria, **Y** (left/west) = Metroid.
  Other controller brands use SDL physical NORTH/WEST, so labels can differ.
- Tap = one rapid visual pulse + matching synthetic sound (0.22 s).
- Hold = a new pulse every 0.5 s while pressed; at 1.0 s the command is
  launched (if configured), while pulses continue every 0.5 s.
- Both games are optional local hooks; no ROMs or copyrighted audio assets
  are bundled. Without a command, holding prints an explanatory message.

## Launch commands

Set the executable command lines in the environment before starting the
selector. They are executed only on a 1-second hold of the respective input.
Example using emulator ROM paths local to your machine:

```sh
export PROTO_ARIA_LAUNCH='mgba "/absolute/path/to/aria.gba"'
export PROTO_METROID_LAUNCH='mgba "/absolute/path/to/metroid-zero-mission.gba"'
./build/proto_rotating_line --config config/prototype_aria_metroid_8x.json
```

Use real paths to **your own** game files. These are examples, not defaults.
Alternatively point to your existing native reconstructed executable.

## Audio

SDL3 synthesizes two original placeholders at 48 kHz mono, 0.22 seconds:
Aria has a two-beat low thud; Metroid has a short filtered crackle.
Source audio is not embedded or committed. Audio failure is nonfatal.
