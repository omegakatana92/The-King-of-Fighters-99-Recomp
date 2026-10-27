The King of Fighters '99 - native static recompilation (Windows x64)
====================================================================

This folder contains the recompiled player, the ROM decrypt helper and the
runtime dependencies.  It does NOT contain game ROMs or the Neo Geo BIOS.

Quick start
-----------
1. Prepare your own legally obtained MAME dumps:

   - kof99.zip   (the "kof99" set: 251-p1.p1, 251-p2.p2, ka.neo-sma,
                  251-c1..c8, 251-m1.m1, 251-v1..v4)
   - neogeo.zip  (the system ROM set: sp-s2.sp1, sfix.sfix, sm1.sm1,
                  000-lo.lo)

   Run the helper from this folder:

       powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1

   It asks for the two zips, extracts them into roms-src\, runs
   kof99_decrypt.exe, and writes the runtime-ready set into roms\.

   To do it by hand:

       mkdir roms-src
       tar -xf kof99.zip -C roms-src
       tar -xf neogeo.zip -C roms-src
       .\kof99_decrypt.exe --rom-path roms-src --out roms

2. Run:

       "KOF 99.exe" --rom-path roms

   The BIOS is the real sp-s2.sp1 MVS system ROM.  There is no BIOS HLE.

   If you run without --rom-path and no roms folder is found, the player
   opens a folder picker.  Select either the decrypted folder (contains
   251-pd.bin) or the original encrypted set (contains 251-p1.p1).  In the
   encrypted case the bundled kof99_decrypt.exe prepares a decrypted copy
   under %LOCALAPPDATA%\KOF99\roms and later runs reuse it.  --headless never
   opens the picker.

Controls
--------
  Arrow keys              move
  Z X C V                 Neo Geo A B C D
  5                       insert coin
  1                       player 1 start
  2                       player 2 start
  3                       select
  9                       service credit
  F2                      test switch
  F11                     toggle fullscreen
  Esc                     quit

  SDL2 gamepads: D-pad or left stick; A B X Y = A B C D; Start = start;
  Back = coin; left shoulder = select.

Options
-------
  --rom-path DIR          game ROM files (default: roms)
  --bios-path DIR         system ROM files (default: the ROM path)
  --bios FILE             system ROM (default: sp-s2.sp1)
  --headless              no window; run as fast as possible
  --frames N              stop after N frames
  --screenshot N:FILE     save frame N as PNG (repeatable)
  --input FILE            scripted input
  --wav FILE              write audio to a WAV file
  --interp                run everything in the interpreter
  --verify                replay native blocks in Musashi and compare
  --dump-misses FILE      write interpreted entry points
  --scale N               window scale (default 3)
  --fullscreen            fullscreen, 4:3 pillarboxed
  --select-rom            force the ROM folder picker
  --mute                  no sound from the window

Status and limitations
----------------------
  * Windows x64 Release build, MSVC 19.51 (VS 18 2026), SDL2 2.30.9.
  * Boot, title, intro, attract mode, coin/start, menus, character select,
    order select and a match were tested headless; the windowed launch was
    tested for startup and rendering.
  * --verify: see the project README for the measured block count and
    mismatch count.
  * Audio: the M1 bank mapper uses the masked file offset (128 KB mask
    0x1FFFF) and the Z80 is advanced once per frame.  A 405 s capture has no
    2 s silent run and BGM-sized energy in 39/39 ten-second windows; MAME
    0.289 chroma similarity is 0.935.  The mix was not subjectively
    auditioned.
  * Physical Steam Deck hardware and physical gamepads were not tested in
    this environment.  Memory cards are not implemented.
  * The mix was not subjectively auditioned; signal continuity and level are
    measured.

Licensing
---------
  Original project code is PolyForm Noncommercial 1.0.0; see
  LICENSES\LICENSE-STATUS.txt and LICENSES\LICENSE.txt.  The bundled
  neogeorecomp toolkit remains MIT; Musashi/z80 are MIT; ymfm, the
  NEO-SMA/CMC decryption material and the MAME-derived tables are
  BSD-3-Clause; SDL2 is zlib.  See LICENSES\THIRD_PARTY_NOTICES.txt.

  The King of Fighters '99 and Neo Geo are trademarks of SNK Corporation.
  This project is not affiliated with or endorsed by SNK.  You are
  responsible for using ROM images you are legally allowed to use.
