# Third-party notices

This repository contains original project code and vendored or adapted
third-party material.  Each part keeps its own licence.  Nothing here
relicenses upstream code under the project's PolyForm licence.

## neogeorecomp (vendored toolkit)

`neogeorecomp/` is the sp00nznet NeoGeoRecomp toolkit, used unmodified except
for the generic NEO-SMA cart support that is already present in this copy
(bank map, protection read and LFSR hooks in `ng_game_t`).

```
MIT License

Copyright (c) 2026 sp00nznet
```

The full text is in `neogeorecomp/LICENSE`, and its own notices are in
`neogeorecomp/NOTICE` and `neogeorecomp/LICENSES/`.

## Musashi

`neogeorecomp/ext/musashi/` - Motorola 68000 interpreter, fallback CPU and
the `--verify` correctness oracle.

```
Copyright (c) 1998-2001 Karl Stenerud
MIT License
```

Full text: `neogeorecomp/LICENSES/Musashi.txt`.

## superzazu/z80

`neogeorecomp/ext/z80/` - Zilog Z80 core used for the sound CPU.

```
Copyright (c) 2019 Nicolas Allemand
MIT License
```

Full text: `neogeorecomp/LICENSES/z80.txt`.

## ymfm

`neogeorecomp/ext/ymfm/` - Yamaha YM2610 synthesis (FM, SSG, ADPCM-A/B).

```
Copyright (c) 2021 Aaron Giles
BSD 3-Clause License
```

Full text: `neogeorecomp/LICENSES/ymfm.txt`.

## SDL2

The Windows package ships `SDL2.dll`; the AppImage bundles
`libSDL2-2.0.so.0`.  SDL2 is zlib licensed.

```
Copyright (c) 1997-2025 Sam Lantinga
zlib License
```

Full text: `packaging/windows/SDL2.txt`.

## MAME NEO-SMA / NEO-CMC42 material

`tools/decrypt.c` and `tools/tables.h` implement the KOF '99 program and
sprite decryption and the fixed-layer extraction.  They are adapted from
MAME's Neo Geo cartridge device sources, which are BSD-3-Clause:

```
prot_sma.cpp: copyright-holders: S. Smith, David Haywood, Fabio Priuli,
              Razoola, Mr.K
prot_cmc.cpp: copyright-holders: S. Smith, David Haywood, Fabio Priuli
license: BSD-3-Clause
```

The BSD-3-Clause terms are reproduced in full in
`LICENSES/MAME-BSD-3-Clause.txt`, together with the copyright holders named
above: redistribution of source must retain the copyright notice, this list
of conditions and the disclaimer; redistribution in binary form must
reproduce them in the documentation; neither the names of the copyright
holders nor contributors may be used to endorse or promote derived products
without permission; the software is provided "as is", without warranty.

These files are **not** covered by the project's PolyForm licence.  They are
used at build time to turn the user's own encrypted dump into a
runtime-ready set; no ROM data is included.

## Game ROMs and ROM-derived output

No SNK ROMs, Neo Geo BIOS images, decrypted ROM files, recompiled C or entry
lists are distributed in this repository.  They are generated locally from
the user's own legally obtained dump and written into the build directory,
which is excluded by `.gitignore`.

The King of Fighters '99 and Neo Geo are trademarks of SNK Corporation.
This project is not affiliated with or endorsed by SNK.
