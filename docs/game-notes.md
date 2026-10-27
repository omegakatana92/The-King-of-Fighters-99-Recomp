# Game notes: The King of Fighters '99

MAME set: `kof99` (MVS, original encrypted release).
Cartridge ID: NGM-2510.  Board: NEO-MVS PROGLBA (NEO-SMA) with NEO-MVS
CHAFIO (NEO-CMC 7042), per MAME's `neogeo.cpp`.

## What is in the supplied set

`kof99.zip` contains 17 files, all with MAME's CRCs:

| File | Size | CRC32 | Role |
|---|---|---|---|
| `ka.neo-sma` | 256 KB | `7766D09E` | data stored inside the NEO-SMA chip |
| `251-p1.p1` | 4 MB | `006E4532` | encrypted program, first half |
| `251-p2.p2` | 4 MB | `90175F15` | encrypted program, second half |
| `251-c1.c1` .. `251-c8.c8` | 8 MB each | see README | encrypted sprite planes |
| `251-m1.m1` | 128 KB | `5E74539C` | plain Z80 sound program |
| `251-v1.v1` .. `251-v3.v3` | 4 MB each | see README | plain ADPCM-A |
| `251-v4.v4` | 2 MB | `B49E6178` | plain ADPCM-A |

The BIOS files used are `sp-s2.sp1` (MVS Asia/Europe), `sfix.sfix`,
`sm1.sm1` and `000-lo.lo`.

There is **no S ROM** and no `251-s1.s1` in the set.  On this board the
128 KB fixed-layer data lives at the end of the C data; MAME extracts it in
`cmc_prot_device::sfix_decrypt()`.  The decryption tool does the same and
writes `251-s1.s1`, so the runtime sees a normal 128 KB S ROM.

## Why a decryption stage is needed

neogeorecomp v0.2.0 explicitly lists "protected and encrypted sets (CMC,
PVC, SMA)" as unsupported: the recompiler must see the real 68000 code.  The
KOF '99 cartridge uses two protection schemes:

* **NEO-SMA program encryption and banking.**  The 8 MB program is
  data-line swapped and address-scrambled; the fixed 768 KB of code is
  relocated out of the banked image.  The chip also contains 256 KB of
  program data (`ka.neo-sma`) mapped at `$0C0000..$0FFFFF`.
* **NEO-CMC42 graphics encryption.**  The eight 8 MB C ROMs are interleaved
  into a 64 MB sprite region, decrypted with the KOF '99 tables and key
  `0x00`, then split back into the eight files the runtime loads.

`tools/decrypt.c` performs both, CRC-checks the inputs first, verifies the
`NEO-GEO` signature at `$100`, and writes a runtime-ready set into the build
directory.  The algorithms and the 9 x 256-byte CMC tables come from MAME's
BSD-3-Clause `prot_sma.cpp` / `prot_cmc.cpp` (see `THIRD_PARTY_NOTICES.md`).
Nothing title-specific was added to the runtime.

The decrypted program image is 9 MB: 1 MB fixed (`$000000..$0FFFFF`) plus
8 MB banked (loaded at `$100000` and visible through the `$200000` window).

## NEO-SMA banking and protection

From MAME `sma_prot_device::kof99_bank_base()` and the NEO-SMA wiki page:

* bank-select write: `$2FFFF1` (word port `$2FFFF0`)
* protection read: `$2FE446`/`$2FE447`, returns `$9A37`
* random latch: `$2FFFF8` and `$2FFFFA`, a 16-bit LFSR seeded `$2345`
  (taps 2, 3, 5, 6, 7, 11, 12, 15), one step per read
* selector unscramble: `bitswap<6>(sel, 5,12,10,8,6,14)`

The 1 MB window at `$200000` is taken from the 8 MB banked image at:

```
000000 100000 200000 300000 3CC000 4CC000 3F2000 4F2000
407800 507800 40D000 50D000 417800 517800 420800 520800
424800 524800 429000 529000 42E800 52E800 431800 531800
54D000 551000 567000 592800 588800 581800 599800 594800
598000
```

The remaining table entries are unused by this cart.  The runtime's generic
`ng_bank_map_t` supplies the offsets and bit order; `src/game.c` is the only
file that knows them.

## Boot sequence observed

With the real `sp-s2.sp1` BIOS:

1. BIOS memory/checksum test, SNK eye-catcher.
2. KOF '99 title ("NEW COMERS / INSERT COIN") from about frame 600.
3. Intro: "NEOGEO GIGA POWER / PRO-GEAR SPEC / SNK" from about frame 5000.
4. Attract demo match from about frame 9000; returns to the title.
5. With a credit (frame ~1600) and Start (frame ~2000): HOW TO PLAY pages,
   character select (frame ~6500), striker select, order select
   (frame ~7500), then the match (frame ~10000).

Frame numbers are 59.1856 Hz MVS frames from power-on.
