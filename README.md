# The King of Fighters '99 — native static recompilation

**The King of Fighters '99 - Millennium Battle (SNK, 1999 / MAME set `kof99`)
running as a native PC program: its 68000 program is translated ahead of time
into C and executed against a model of the MVS board by
[neogeorecomp](https://github.com/sp00nznet/neogeorecomp).**

Two deliverables are produced:

| Platform | Artifact |
|---|---|
| Windows x64 | `dist/windows/KOF-99-Windows-x64/KOF 99.exe` |
| Steam Deck / Linux x86_64 | `dist/steamdeck/KOF 99-SteamDeck-x86_64.AppImage` |

The game's own code runs natively. The real MVS system ROM boots the board,
checks its checksum and RAM, shows the SNK eye-catcher, handles coins and
credits, and calls the cartridge exactly as on hardware. There is no BIOS HLE
and no emulator frontend. The Musashi interpreter remains as the correctness
oracle and as the fallback for banked code the flat dispatch cannot identify
(measured below).

> **No game data here.** You need your own legally obtained dumps of the MAME
> `kof99` set and the `neogeo` system ROMs. The decrypted ROMs and the
> recompiled C are generated from your dump on your machine into `build-*/`;
> they are never committed.

## Status

**v0.1.0, alpha - boots, runs attract mode and plays a match on recompiled
code.**

| Area | State |
|---|---|
| Boot | Real MVS BIOS (`sp-s2.sp1`) -> KOF '99 title -> intro (NEOGEO GIGA POWER / SNK) -> attract demo match |
| ROM set | Exact MAME `kof99` MVS set, all 20 input files CRC-verified by `kof99_decrypt` (see below) |
| Input / credits | Coin, Start, directions and A/B/C/D exercised through the BIOS, HOW TO PLAY, character select, striker select, order select and a match |
| Gameplay | Scripted 24,000-frame session reaches a match, moves and uses all four buttons; HUD, health bars and character switching render |
| Graphics | Sprites, backgrounds, palettes, fixed layer, raster effects; Windows and Linux produce byte-identical frames for the same inputs |
| Audio | Real Z80 M1 driver + YM2610 (ymfm). 405 s capture: median 1 s RMS 2884.5, no 2 s silent run, **39/39** 10 s windows with BGM-sized energy; MAME 0.289 reference chroma similarity **0.935** (0.99+ in the first 50 s). Not subjectively auditioned |
| Native coverage | **99.99%** of the 24,000-frame play session (66,217 interpreted instructions, 61 uncovered); **100.00%** on the 6,000-frame verify and 24,000-frame audio-check sessions; **92.59%** of a 12,000-frame no-input attract run (bank-window fallback) |
| `--verify` | Windows 4,340,995 blocks over 6,000 gameplay frames, **0 mismatches**; Linux 2,642,700 blocks over 4,000 frames, **0 mismatches** |
| Recompiler | 1,343,495 instructions, 20,738 routines, **39,675 dispatch entries** (1,046 of them from the profile pass) |
| Toolkit tests | 68000 conformance 59/60 (reference 59/60); SDL virtual-gamepad test passes (P1/P2, D-pad/stick, A/B/C/D, Start, Back-as-coin, unplug) |
| Windows | Release exe built with MSVC 19.51 (VS 18 2026) x64 + SDL2 2.30.9; headless and windowed launches tested |
| Steam Deck | AppImage built and launched headless and fullscreen under WSL2 Ubuntu 22.04; first-run decryption tested; **not tested on physical Deck hardware** |
| License | PolyForm Noncommercial 1.0.0 for original code (confirmed 2026-10-27); upstream MIT/BSD-3-Clause notices preserved |

### The supplied set is the original encrypted MVS release

The `kof99.zip` in `game/` is the exact MAME `kof99` set. CRC-32 of every
input was checked against `ROM_START( kof99 )` in MAME's Neo Geo driver:

```
ka.neo-sma  256 KB  7766D09E   NEO-SMA protection program/data
251-p1.p1     4 MB  006E4532   encrypted program, first half
251-p2.p2     4 MB  90175F15   encrypted program, second half
251-c1.c1     8 MB  0F9E93FE   encrypted sprite plane 0/1
251-c2.c2     8 MB  E71E2EA3   encrypted sprite plane 2/3
251-c3.c3     8 MB  238755D2   encrypted sprite plane 0/1
251-c4.c4     8 MB  438C8B22   encrypted sprite plane 2/3
251-c5.c5     8 MB  0B0ABD0A   encrypted sprite plane 0/1
251-c6.c6     8 MB  65BBF281   encrypted sprite plane 2/3
251-c7.c7     8 MB  FF65F62E   encrypted sprite plane 0/1
251-c8.c8     8 MB  8D921C68   encrypted sprite plane 2/3
251-m1.m1   128 KB  5E74539C   plain Z80 sound program
251-v1.v1     4 MB  EF2EECC8   plain ADPCM-A samples
251-v2.v2     4 MB  73E211CA   plain ADPCM-A samples
251-v3.v3     4 MB  821901DA   plain ADPCM-A samples
251-v4.v4     2 MB  B49E6178   plain ADPCM-A samples
```

This cartridge is **not** a plain set:

* The program is **NEO-SMA encrypted** and bank-switched. The chip also
  supplies a 16-bit LFSR (seed `$2345`) and answers `$9A37` on its
  protection read.
* The graphics are **NEO-CMC42 encrypted** (KOF '99 key `$00`).
* There is **no S ROM**: the 128 KB fixed-layer data is stored at the end of
  the C data and is extracted by the same permutation MAME uses. (The
  protected board has no `251-s1.s1` in its set.)
* `neogeorecomp` v0.2.0 lists SMA/CMC sets as unsupported. This project adds
  a separate, build-time decryption stage (`tools/decrypt.c`), so the toolkit
  itself sees an ordinary 9 MB program image, plain C ROMs and a 128 KB S
  ROM. The decryption is not a title-specific hack in the runtime.

## Requirements

* **Windows**: Visual Studio 2022/2026 with the C++ workload, CMake 3.21+,
  SDL2 2.x (built against SDL2 2.30.9).
* **Linux**: gcc/g++, CMake 3.21+, make, `libsdl2-dev`.
* Your own legally obtained dumps of the MAME `kof99` and `neogeo` sets.

## Quick start

### 1. Prepare your ROM files

Unzip both sets into one directory, for example `roms-src/`:

```
mkdir roms-src
unzip kof99.zip -d roms-src
unzip neogeo.zip -d roms-src
```

The build runs `kof99_decrypt` and writes the runtime-ready set into
`build-windows/roms` (or `build-linux/roms`). To prepare it by hand:

```
build-windows\Release\kof99_decrypt.exe --rom-path roms-src --out roms
```

### 2. Build

Windows (from the project root):

```
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 ^
      -DKOF99_ROM_DIR=C:/path/to/roms-src ^
      -DSDL2_DIR=C:/path/to/SDL2-2.30.9/cmake
cmake --build build-windows --config Release --parallel
```

If no SDL2 path is supplied, the build is headless-only.

Linux:

```
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DKOF99_ROM_DIR=$PWD/roms-src
cmake --build build-linux --parallel "$(nproc)"
```

To build the portable AppImage in one step:

```
./packaging/appimage/build-appimage.sh /path/to/roms-src
```

That writes `dist/steamdeck/KOF 99-SteamDeck-x86_64.AppImage` (SDL2 is
bundled). Put your `roms/` directory next to the AppImage, or pass
`KOF99_ROM_DIR` / `--rom-path`.

### 3. Run

```
build-windows/Release/"KOF 99.exe" --rom-path build-windows/roms
# or
build-linux/kof99 --rom-path build-linux/roms
```

Without `--rom-path`, the player looks for `roms` in the working directory,
`roms` beside the executable, `KOF99_ROM_DIR`, then
`%LOCALAPPDATA%\KOF99\roms` (`~/.local/share/kof99/roms` on Linux). If the
folder holds the original encrypted set, the bundled decrypt tool prepares a
cached copy. If nothing is found, a native folder picker is shown.

Useful flags: `--headless`, `--frames N`, `--input FILE`,
`--screenshot N:FILE`, `--wav FILE`, `--interp`, `--verify`,
`--dump-misses FILE`, `--scale N`, `--fullscreen`, `--mute`. Run `--help`
for the runtime list.

## Controls

| Input | Action |
|---|---|
| Arrows / left stick | move |
| Z X C V / A B X Y (Xbox layout) | Neo Geo A B C D |
| 5 / Back | insert coin |
| 1 / Start | start (P1) |
| 2 | player 2 start |
| 3 / left shoulder | select |
| 9 | service credit |
| F2 | test switch |
| F11 | toggle fullscreen |
| Esc | quit |

The pad mapping is the toolkit's fixed SDL2 GameController layout; a pad SDL
does not recognise can be described with `SDL_GAMECONTROLLERCONFIG`.

On an MVS, insert a coin before Start does anything. The BIOS ignores
coin/start until the cartridge title screen is up; the test scripts insert
the first credit at frame 1600.

## Testing

The upstream Musashi/z80 binary test corpora are not shipped in this
repository, to keep it free of ROM-like binary test data.  The 59/60
conformance result below was measured with a full upstream toolkit checkout;
to reproduce it, point `NEOGEORECOMP_BUILD_TESTS=ON` at a complete
neogeorecomp tree.

```
# boot, title, intro and attract mode
"KOF 99.exe" --headless --rom-path build-windows\roms --frames 12000 ^
    --screenshot 600:shots/title.png --screenshot 5000:shots/intro.png ^
    --screenshot 9000:shots/attract.png

# coin -> start -> HOW TO PLAY -> character/striker/order select -> match
"KOF 99.exe" --headless --rom-path build-windows\roms --frames 16000 ^
    --input tests/play.txt --screenshot 6500:shots/select.png ^
    --screenshot 10000:shots/match.png

# replay every native block in Musashi and compare
"KOF 99.exe" --headless --verify --rom-path build-windows\roms --frames 6000 ^
    --input tests/play.txt

# collect runtime-only entry points, rebuild, and re-measure
cmake --build build-windows --config Release --target profile
cmake --build build-windows --config Release --target kof99

# audio continuity regression
cmake --build build-windows --config Release --target audio-check
```

Measured on the Windows Release build:

```
[tests/play_long.txt, 24,000 frames]
[exec] native blocks 17,282,547, native instructions 445,210,488,
       interpreted instructions 66,217 (99.99% native), 61 uncovered

[no input, 12,000 frames]
[exec] native blocks 6,861,842, native instructions 209,101,748,
       interpreted instructions 16,735,458 (92.59% native), 0 uncovered

[--verify, tests/play.txt, 6,000 frames]
[verify] 4,340,995 blocks checked, 0 mismatches
[exec] ... interpreted instructions 76 (100.00% native), 0 uncovered

[--verify, no input, 3,000 frames]
[verify] 1,618,976 blocks checked, 0 mismatches
```

Linux Release build:

```
[--verify, tests/play.txt, 4,000 frames]
[verify] 2,642,700 blocks checked, 0 mismatches
[exec] ... interpreted instructions 76 (100.00% native), 0 uncovered

[play_long.txt, 24,000 frames]
[exec] ... interpreted instructions 66,217 (99.99% native), 61 uncovered
```

The 61 interpreted entry points in the long run are RTC-seeded paths that
differ between sessions; they run correctly in Musashi.  The no-input
attract session spends time in code fetched through swapped bank windows
(the flat dispatcher cannot tell which 1 MB bank is mapped), so that session
is 92.59% native even though no address is "uncovered".

`--interp` and recompiled screenshots at frames 600 and 1199 are
**byte-identical** (SHA-256 `297eb126...` and `58af4dda...`) on Windows and
Linux.

## Compatibility report

| Item | Result |
|---|---|
| 68000 execution | Native for the fixed bank and bank 0; verified block-for-block against Musashi |
| BIOS initialization | Real `sp-s2.sp1`; memory test, eyecatcher, coin/credit path exercised |
| Cartridge banking | NEO-SMA 63-entry LFSR-scrambled bank table at `$2FFFF0`; `$9A37` read at `$2FE446`; LFSR at `$2FFFF8` / `$2FFFFA` |
| Sprite graphics | CMC42-decrypted, rendered in attract and match |
| Fixed layer | Extracted 128 KB S ROM renders HUD, names, menus and credits |
| Palettes / raster | Palette banks and raster effects visible in intro and stages |
| Z80 + YM2610 | Real M1 driver, NMI/reply latches, bank selects; audio captured and checked for gaps |
| SDL2 input/audio | Keyboard and window startup tested; gamepad mapping is the toolkit's; no physical pad available |
| AES / memory card | Not implemented by the toolkit |
| Physical Steam Deck | Not tested; AppImage launched under WSL2 |

## Known limitations

* **Steam Deck hardware not tested.** The AppImage was built and run headless
  under WSL2 Ubuntu 22.04; physical Deck LCD/OLED, Steam Input and Gaming
  Mode still need validation. If the system lacks FUSE 2, use
  `APPIMAGE_EXTRACT_AND_RUN=1`.
* **Physical gamepads not tested here.** The SDL2 GameController path is the
  toolkit's existing mapping.
* **Audio not auditioned.** Signal presence, level and continuity are
  measured; subjective mix quality is not verified.
* **One credit / one match session tested.** The scripted session reaches a
  match and exercises movement and all four buttons; later stages, continues
  and endings are not exhaustively tested.
* **No memory card support.** The toolkit does not implement it.
* **Bank-window fallback.** Code reached through a swapped bank window runs
  in Musashi; measured shares are above.
* **No commercial use.** Original code is PolyForm Noncommercial 1.0.0; do
  not assume commercial redistribution is permitted.
* **No ROMs or BIOS included.** Supply your own legally obtained dumps.

## Project layout

```
CMakeLists.txt              build: decrypt -> recompile -> player
src/game.c, game.h          verified ng_game_t: ROM layout and SMA map
src/main.c                  player (ng_main)
src/rom_select.c/.h         ROM folder discovery, picker and first-run decrypt
src/recomp_main.c           generator (ng_recomp_main)
tools/decrypt.c             NEO-SMA + NEO-CMC42 decryption, CRC-checked
tools/tables.h              MAME-derived CMC42 tables (BSD-3-Clause)
tests/coin_start.txt        credit/start script
tests/play.txt              16,000-frame menu and match script
tests/play_long.txt         24,000-frame profile script
tests/attract.txt           no-input attract profile script
tests/check_audio.py        audio continuity regression (`audio-check`)
tests/check_bgm.py          BGM continuity: every 10 s window must carry music
packaging/appimage/         AppImage build script, desktop file, icon, README
packaging/windows/          Windows package README, ROM helper, SDL2 notice
scripts/package-windows.ps1 Windows dist packaging
neogeorecomp/               toolkit copy (MIT), with generic NEO-SMA support
```

## Documentation

* `docs/game-notes.md` - ROM set, encryption, banking and the boot sequence.
* `docs/testing.md` - exact commands and measured results.
* `docs/compatibility.md` - hardware and platform compatibility report.
* `docs/runtime-changes.md` - generic toolkit changes relative to v0.2.0.
* `THIRD_PARTY_NOTICES.md` - complete attribution.

## License

Original code in `src/`, `packaging/`, `scripts/` and the documentation is
licensed under **PolyForm Noncommercial 1.0.0** (see `LICENSE` and
`LICENSE-STATUS.md`), confirmed by the repository owner on 2026-10-27.

`tools/decrypt.c` and `tools/tables.h` contain MAME-derived BSD-3-Clause
material and are licensed under BSD-3-Clause, not PolyForm. `neogeorecomp/`
remains MIT (sp00nznet); Musashi and superzazu/z80 are MIT; ymfm is
BSD-3-Clause; SDL2 is zlib. Full attribution is in
`THIRD_PARTY_NOTICES.md` and the preserved `neogeorecomp/LICENSE`, `NOTICE`
and `LICENSES/`.

No SNK ROMs, BIOS images, decrypted ROM files, generated recompiled C or
ROM-derived entry lists are distributed. The King of Fighters '99 and Neo Geo
are trademarks of SNK Corporation. This project is not affiliated with or
endorsed by SNK. Supply your own legally obtained dump.
