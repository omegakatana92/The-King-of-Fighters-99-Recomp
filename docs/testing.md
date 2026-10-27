# Testing and measured results

All runs use the real `sp-s2.sp1` MVS BIOS and the decrypted set produced by
`kof99_decrypt` from the supplied `kof99.zip` + `neogeo.zip`.  Frame numbers
are 59.1856 Hz frames from power-on.

## Hosts

* Windows 11 x64, MSVC 19.51.36257 (Visual Studio 18 2026), CMake 4.3.3,
  SDL2 2.30.9, Release `/O1` for generated files.
* Ubuntu 22.04 under WSL2, gcc 11.4.0, CMake 4.4.4, libsdl2 2.0.20,
  Release `-O1` for generated files.

## ROM preparation

```
build-windows\Release\kof99_decrypt.exe --rom-path roms-src --out build-windows\roms
decrypt: P ROM -> 251-pd.bin (SP=0010F300 PC=00C00402)
decrypt: S ROM -> 251-s1.s1 (0x20000 bytes)
decrypt: C ROMs -> 251-c1..c8 (8 MB each)
decrypt: wrote a complete ROM set to build-windows\roms
```

Every input is size- and CRC32-checked before use; a wrong file aborts the
build.  `NEO-GEO` is present at `$100` of the fixed bank after decryption.

## Recompilation

```
[m68krecomp] 1046 extra entry points from build-windows/entries.txt
[m68krecomp] pointer scan: 18224 seeds
[m68krecomp] kof99: 1343495 instructions emitted across routines
[m68krecomp] 20738 routines, 39675 dispatch entries -> build-windows/generated
```

Static discovery alone produced 38,595 entries / 1,301,660 instructions; the
profile pass added 1,046 runtime-only entries.  The generated C is about
197 MB (32 files).

## Verification (`--verify`)

Windows:

```
KOF 99.exe --headless --verify --rom-path build-windows\roms --frames 6000 ^
    --input tests\play.txt
[verify] 4340995 blocks checked, 0 mismatches
[exec] native blocks 4340995, native instructions 111311016,
       interpreted instructions 76 (100.00% native), 0 uncovered entry points
```

```
KOF 99.exe --headless --verify --rom-path build-windows\roms --frames 3000
[verify] 1618976 blocks checked, 0 mismatches
```

Linux:

```
./build-linux/kof99 --headless --verify --rom-path build-linux/roms \
    --frames 4000 --input tests/play.txt
[verify] 2642700 blocks checked, 0 mismatches
[exec] ... interpreted instructions 76 (100.00% native), 0 uncovered entry points
```

## Native coverage

Windows, 24,000-frame scripted session (`tests/play_long.txt`):

```
[exec] native blocks 17282547, native instructions 445210488,
       interpreted instructions 66217 (99.99% native), 61 uncovered entry points
```

Windows, 12,000-frame no-input attract session:

```
[exec] native blocks 6861842, native instructions 209101748,
       interpreted instructions 16735458 (92.59% native), 0 uncovered entry points
```

Linux, same 24,000-frame session:

```
[exec] native blocks 17282334, native instructions 445210418,
       interpreted instructions 66217 (99.99% native), 61 uncovered entry points
```

The 61 interpreted entry points vary between sessions (the uPD4990A calendar
seeds some AI paths).  The attract session's interpreted share is the
bank-window fallback: code fetched through a swapped 1 MB bank cannot be
dispatched by the flat table, so it runs in Musashi even though its addresses
are in the table (hence "0 uncovered").

## Audio

`audio-check` runs 24,000 frames and analyses the WAV with two independent
checks:

```
audio-check: build-windows/audio-check.wav
  window 10s..405s  median RMS 2918.6
  longest silent run 0.0s (limit 4.0s)
  result: PASS
[exec] ... interpreted instructions 174 (100.00% native), 4 uncovered

bgm-check: build-windows/audio-check.wav
  405s total, analysis from 10s
  median RMS 2884.5 (minimum 1500.0)
  longest near-silent run 0s at 0s (limit 2.0s)
  39/39 10s windows at or above RMS 800 (100%, need 90%)
  result: PASS
```

`check_audio.py` fails on music-sized silent gaps; `check_bgm.py` requires
sustained music-sized energy in every 10-second window after the BIOS boot.
Both pass through the title, HOW TO PLAY, character select, order select and
the match.

### M1 bank fix

The Z80 M1 bank mapper uses MAME `init_audio()`'s masked file offset directly:

```c
uint32_t reg_mask = size ? (((uint32_t)size - 1u) & 0x3FFFFu) : 0;
off = (s_bank[w] & reg_mask) + (a & win_mask[w]);
```

For KOF '99's 128 KB M1 this is `0x1FFFF`; the earlier bug added `0x10000`
to every bank, so the driver read every banked window 64 KB too high.  The
runtime also calls `audio_sync()` once per frame, so the Z80 sequencer and
YM2610 timers keep running when the 68000 is not polling a sound port.  This
is the same generic fix used by the Garou, KOF '96/'97/2000 and Metal Slug 3
projects; `audio.c` was diffed against those copies and the functional code
is identical.

A KOF '99 trace (`NG_AUDIO_TRACE=1`) shows the four windows reading file
offsets `0x0F000 / 0x0E000 / 0x0C000 / 0x08000`, exactly MAME's
`init_audio()` values (`0x1E<<11`, `0x0E<<12`, `0x06<<13`, `0x02<<14`), not
the buggy `0x1F000 / 0x1E000 / 0x1C000 / 0x18000`.

### MAME reference comparison

A MAME 0.289 attract-mode capture (`kof99`, 90 s, `-wavwrite`) was compared
with the recompiled attract capture:

* RMS envelope correlation **0.727** at a 1 s offset.
* Chroma (12 pitch-class) similarity **0.935** at a 1 s offset; **0.99+**
  for the first 50 seconds of active music.
* Both have the same silence during the BIOS/intro transition and the same
  title/intro music segments.

The residual difference is emulator timing and MAME's 48 kHz resampling, not
different notes or a missing bank.  The mix has still not been subjectively
auditioned.


## Cross-platform determinism

`--interp` and recompiled runs at frames 600 and 1199 are byte-identical on
both hosts:

```
297eb1262f921af2158afbc8bde9cb06c7f920631243468e342c73c92604d8e7  frame 600
58af4dda33546b82292d48ffbc2fd0e1bfa7663feb7201fff099014638f42612  frame 1199
```

## Gameplay / graphics checks

Screenshots captured with `--screenshot`:

| Frame | Screen |
|---|---|
| 600 | KOF '99 title ("NEW COMERS / INSERT COIN") |
| 1200 | intro silhouette |
| 5000 | "NEOGEO GIGA POWER / PRO-GEAR SPEC / SNK" |
| 9000 | attract demo match |
| 3000 (with coin/start) | HOW TO PLAY, POWER GAUGE |
| 6500 (with coin/start) | character select (Benimaru) |
| 7500 (with coin/start) | ORDER SELECT |
| 10000 (with coin/start) | match: Shingo vs Chin, China Town stage |
| 15999 (with coin/start) | match continues, next team member |

The fixed layer renders names, HUD, timer, credits and menu labels; sprites,
backgrounds and palettes render correctly, which also confirms the CMC42
decryption and the S-ROM extraction.

## Windowed launch

```
KOF 99.exe --rom-path build-windows\roms --frames 300 --scale 2 --mute
exit code 0
```

The SDL2 window, renderer and audio device initialize and the run exits
cleanly after 300 frames.

## AppImage

Built on Ubuntu 22.04 with SDL2 2.0.20:

```
./packaging/appimage/build-appimage.sh /path/to/roms-src
wrote dist/steamdeck/KOF 99-SteamDeck-x86_64.AppImage
```

Tests performed:

* `--appimage-extract-and-run` headless, 1,200 frames, screenshot at 1199:
  SHA-256 `58af4dda...`, byte-identical to the Windows/headless frame.
* AppRun auto-discovery with `KOF99_ROM_DIR` set to a decrypted folder:
  300 frames, 100.00% native, 0 uncovered.
* First-run path with `KOF99_ROM_DIR` pointing at the original encrypted
  set and a clean `HOME`: the bundled `kof99_decrypt` produced
  `~/.local/share/kof99/roms` and the game then ran 300 frames at 100%
  native.
* Fullscreen under Xvfb (`1280x800x24`): the rendered image is 1066x800 at
  x=107, i.e. 4:3 pillarboxed, not stretched to 16:10.  F11 toggles it.
* Extracted contents checked: `AppRun` executable, desktop entry, 256x256
  icon, `libSDL2-2.0.so.0`, license texts; `ldd` resolves SDL2 from
  `usr/lib`; no ROM files are inside.

The AppImage was **not** launched on physical Steam Deck hardware, and Steam
Input/Gaming Mode remain untested.

## Toolkit tests

The upstream Musashi/z80 binary test corpora are not shipped in this
repository.  The numbers below were measured with a full upstream toolkit
checkout; to reproduce them, enable `NEOGEORECOMP_BUILD_TESTS` on a complete
neogeorecomp tree.

```
ctest --test-dir build-windows -C Release
100% tests passed, 0 tests failed out of 2

ng_conformance --corpus neogeorecomp/ext/musashi/test/mc68000
conformance: 59/60 68000 test programs pass recompiled
             (reference interpreter: 59/60)

ctest --test-dir build-gamepad -C Release -R gamepad
1/1 Test #2: gamepad ... Passed
```

The gamepad test attaches two SDL virtual controllers and checks P1/P2
directions, A/B/C/D, Start, Back-as-coin and hot-unplug through
`window_sdl.c`.  It passed on Windows; on Linux, Ubuntu 22.04's SDL2 2.0.20
is older than the 2.24 virtual-joystick API, so the test skips there.


