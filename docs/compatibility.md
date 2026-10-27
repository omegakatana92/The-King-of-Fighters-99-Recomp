# Compatibility report

Game: The King of Fighters '99 - Millennium Battle (MAME `kof99`, MVS).
Toolkit: neogeorecomp v0.2.0 (vendored copy with generic NEO-SMA support).

## Hardware model

| Feature | Status | Evidence |
|---|---|---|
| 68000 CPU | Native recompiled code; Musashi fallback | `--verify` 0 mismatches, 4.34M blocks Windows / 2.64M Linux |
| Program banking | NEO-SMA 63-entry scrambled table at `$2FFFF0` | Boot, menus and match run from multiple banks |
| Program protection | `$9A37` at `$2FE446`; LFSR at `$2FFFF8`/`$2FFFFA` | No protection hang; demo/match reach gameplay |
| BIOS | Real `sp-s2.sp1`, no HLE | Eye-catcher, coin/credit, BIOS calls |
| Sprite graphics | CMC42-decrypted 64 MB C data | Characters, backgrounds and effects render |
| Fixed layer | 128 KB S extracted from C data | HUD, names, menus, credits render |
| Palette | Both palette banks | Intro and stages show correct colours |
| Raster | Scanline palette/timer effects | Intro and stage backgrounds match interpreter frames |
| Z80 sound CPU | superzazu/z80, real M1 driver | Audio capture non-silent for 405 s; BGM check 39/39 windows; MAME chroma match 0.935 |
| YM2610 | ymfm FM/SSG/ADPCM-A | Audio capture; 405 s continuity and MAME reference comparison pass |
| SDL2 video | Windowed and fullscreen 4:3 pillarbox | Windowed run exits 0; fullscreen Xvfb capture is 1066x800 in 1280x800 |
| SDL2 audio | Queued audio device / WAV | `--wav` capture checked |
| SDL2 input | Keyboard + GameController mapping | Keyboard path exercised by input scripts; SDL virtual-gamepad test passes; no physical pad available |
| Memory card | Not implemented by toolkit | Out of scope |
| AES system | Not tested (MVS BIOS used) | Out of scope |
| PAL timing | Not tested | MVS 59.1856 Hz only |

## Platform matrix

| Platform | Build | Launched | Notes |
|---|---|---|---|
| Windows 11 x64 | MSVC 19.51 Release | Yes, headless + windowed | SDL2 2.30.9 |
| Ubuntu 22.04 (WSL2) | gcc 11.4 Release | Yes, headless | SDL2 2.0.20, AppImage base |
| SteamOS / Deck LCD/OLED | AppImage built | **Not tested on hardware** | WSL2 only; Steam Input/Gaming Mode unverified |
| macOS | Not built | No | Toolkit supports it; not a deliverable |

## Measured coverage

| Session | Native |
|---|---|
| 6,000-frame coin/start/menus/match, `--verify` | 100.00% (76 interpreted instructions) |
| 24,000-frame play_long | 99.99% (66,217 interpreted, 61 uncovered) |
| 24,000-frame audio-check | 100.00% (114 interpreted, 0 uncovered) |
| 12,000-frame no-input attract | 92.59% (bank-window fallback, 0 uncovered) |

The interpreter fallback is allowed by the brief for incomplete native
coverage and is used here only for banked code and RTC-seeded paths.

## Untested / known gaps

* Physical Steam Deck (LCD and OLED), Steam Input, Gaming Mode.
* Physical controllers (SDL mapping is the toolkit's fixed layout).
* Subjective audio quality; only levels and continuity were measured.
* Long sessions beyond one match; continues and endings.
* Test/service menus and DIP switch combinations.
* Memory cards.
* Commercial use (PolyForm Noncommercial; not permitted by default).
