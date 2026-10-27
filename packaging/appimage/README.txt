The King of Fighters '99 - Steam Deck / Linux x86_64
====================================================

This AppImage contains the native x86_64 recompilation of The King of
Fighters '99 (Neo Geo MVS), the SDL2 window/audio layer and the bundled ROM
decrypt tool.  The game's 68000 program is translated ahead of time to C and
runs against a model of the MVS board; it is not an emulator wrapper.  The
real sp-s2.sp1 MVS BIOS boots the cartridge.

It does not contain game ROMs or the Neo Geo BIOS.  Supply your own legally
obtained dump.

Quick start
-----------
1. Put your MAME "kof99" set and "neogeo" set in a folder, unzipped:

     mkdir -p roms
     unzip kof99.zip -d roms
     unzip neogeo.zip -d roms

   Put that folder next to the AppImage (a "roms" directory works), or set
   KOF99_ROM_DIR in the launch options.  If the folder contains SNK's
   original encrypted set (251-p1.p1), the launcher decrypts it once into
   ~/.local/share/kof99/roms and starts from there.  To prepare it yourself:

     ./KOF\ 99-SteamDeck-x86_64.AppImage --appimage-extract
     ./squashfs-root/usr/bin/kof99_decrypt --rom-path roms --out roms

2. Make the AppImage executable and run it:

     chmod +x "KOF 99-SteamDeck-x86_64.AppImage"
     ./"KOF 99-SteamDeck-x86_64.AppImage"

   It starts in fullscreen with a 4:3 pillarboxed image, not stretched to
   the 16:10 panel.  F11 toggles fullscreen; Esc quits.

If no ROM folder is found, the player opens a folder picker (zenity, kdialog
or yad).  --rom-path bypasses discovery; --headless never opens a dialog.

Steam Deck
----------
Copy the AppImage and a roms folder to the Deck, mark the AppImage executable
(chmod +x), and add it as a non-Steam game.  Set KOF99_ROM_DIR in the game's
launch options if the ROMs live elsewhere.  The AppImage is built on Ubuntu
22.04 (glibc 2.35), which SteamOS 3 satisfies.  If FUSE 2 is unavailable,
run it with APPIMAGE_EXTRACT_AND_RUN=1.

Controls
--------
  D-pad / left stick        move
  A B X Y (Xbox layout)     Neo Geo A B C D
  Back                      insert coin
  Start                     player 1 start
  Left shoulder             select
  Arrows, Z X C V           keyboard movement and A B C D
  5 / 1 / 2 / 3             coin / P1 start / P2 start / select
  9                         service credit
  F2                        test switch
  F11                       toggle fullscreen
  Esc                       quit

The pad mapping is the toolkit's fixed SDL2 GameController layout.  A pad
SDL does not recognise can be described with SDL_GAMECONTROLLERCONFIG.  Steam
Input's standard gamepad emulation works without extra configuration.

Notes
-----
* The MVS BIOS is the real sp-s2.sp1 system ROM; there is no BIOS HLE.
* The game's 68000 code is statically recompiled.  Code executed through a
  swapped bank window, where the flat dispatcher cannot identify the bank,
  falls back to the bundled Musashi interpreter; the measured share is in
  the project README.
* Audio is produced by the real Z80 driver and the YM2610 model (ymfm).
  The M1 bank mapper uses the masked file offset (KOF '99's 128 KB M1 mask
  is 0x1FFFF), so no bank is read 64 KB too high, and the Z80 is advanced
  once per frame.  A 405 s capture passes both the gap check and the
  per-10-second BGM check (39/39 windows); a 90 s comparison with MAME 0.289
  gives 0.935 chroma similarity (0.99+ in the first 50 s).
* Coverage, verification and test details are in the project README and
  docs/testing.md.

Licensing
---------
Original project code is PolyForm Noncommercial 1.0.0 (see LICENSES).  The
vendored neogeorecomp toolkit is MIT; Musashi and z80 are MIT; ymfm, the
NEO-SMA/CMC decryption material and the MAME-derived tables are BSD-3-Clause;
SDL2 is zlib.  Full attribution is in LICENSES/THIRD_PARTY_NOTICES.txt.

No ROMs or BIOS are included.  This AppImage contains code generated from
SNK's ROM data; use it only with ROMs you are legally allowed to use.  The
King of Fighters '99 and Neo Geo are trademarks of SNK Corporation.  This
project is not affiliated with or endorsed by SNK.
