# License status

**Status: PolyForm Noncommercial 1.0.0, confirmed by the repository owner on
2026-10-27.**

The original project code in this repository - the game host
(`src/main.c`, `src/rom_select.*`, `src/game.*`, `src/recomp_main.c`), the
packaging scripts, the build files and the documentation - is licensed under
the **PolyForm Noncommercial License 1.0.0** (see `LICENSE`).

Required Notice: Copyright (c) 2026 the KOF '99 Recompilation authors.

## What this licence does not cover

* `neogeorecomp/` is the upstream toolkit by sp00nznet, **MIT** licensed.
  Its `LICENSE`, `NOTICE` and `LICENSES/` directory are preserved.  Do not
  relicense those files under PolyForm.
* Musashi (**MIT**), superzazu/z80 (**MIT**) and ymfm (**BSD-3-Clause**) are
  vendored under `neogeorecomp/ext/`; their notices are preserved.
* `tools/decrypt.c` and `tools/tables.h` are adapted from MAME's NEO-SMA and
  NEO-CMC42 device sources and are **BSD-3-Clause**, not PolyForm.  The file
  headers and `THIRD_PARTY_NOTICES.md` record the copyright holders.
* SDL2 is **zlib** licensed.  The Windows DLL and the AppImage bundle it
  under that licence.
* The game ROMs, the Neo Geo BIOS, and anything generated from them
  (decrypted ROMs, recompiled C, entry lists, gameplay screenshots) are not
  part of this project's licensed source and are not distributed.  Users
  supply their own legally obtained dumps.
* `src/game.c` describes a commercial game's ROM layout.  It contains no SNK
  code or data, but the game it describes is SNK's copyrighted work.

PolyForm Noncommercial is not a free/open-source licence and does not permit
commercial redistribution.  Do not assume otherwise.  If you need commercial
terms, contact the project owner.

The combined Windows executable and AppImage contain the MIT/BSD-3-Clause
vendored components listed above.  Their notices are shipped in the
`LICENSES` folder of each package.

See `THIRD_PARTY_NOTICES.md` for the complete attribution list.
