# Toolkit changes relative to neogeorecomp v0.2.0

The vendored `neogeorecomp/` tree is the same generic toolkit copy used by
the neighbouring Garou project, not a title-specific fork.  The changes that
matter for KOF '99 are generic hardware support; `src/game.c` is the only
KOF '99-specific file.

## NEO-SMA carts (`include/neogeorecomp/neogeo.h`, `src/bus.c`)

`ng_game_t` gained optional fields:

```c
typedef struct {
    const uint32_t *offsets;   /* bank offsets inside the banked area */
    uint32_t        count;
    uint8_t         bits[6];   /* selector bits, most significant first */
} ng_bank_map_t;

const ng_bank_map_t *bank_map;   /* NULL = standard 1 MB banking */
uint32_t bank_write_addr;        /* bank-select port; 0 = $2FFFF0 */
uint32_t prot_read_addr;         /* protection read address; 0 = none */
uint16_t prot_value;
uint32_t rng_read_addr[2];
uint16_t rng_seed;
```

`bus.c` implements:

* a scrambled selector -> offset lookup for the 1 MB `$200000` window;
* a byte/word read path for addresses inside the bank window;
* the SMA protection register (`$9A37` for KOF '99);
* the 16-bit LFSR with one step per read, seeded at reset.

Carts that leave `bank_map` NULL keep the original linear `(selector & 7) <<
20` behaviour, so existing games are unaffected.

## Audio (`src/audio.c`)

The M1 bank mapper uses the masked file offset directly
(`(size - 1) & 0x3FFFF`) instead of adding a 64 KB base, and the Z80 is
advanced once per frame even when the 68000 does not poll the sound latches.
Both fixes are generic; KOF '99's M1 driver relies on them for continuous
music.

## Video (`src/video.c`)

The decoded sprite buffer is rounded up to the power-of-two tile mask and
zero-filled, matching MAME's `neosprite` device.  This matters for C ROM
sizes that are not a power of two (KOF '97 is 40 MB).  KOF '99 is 64 MB, so
the fix is defensive here.

## Window (`src/window_sdl.c`)

`--fullscreen` and F11 use `SDL_WINDOW_FULLSCREEN_DESKTOP`, fit a 4:3 image
into the panel and pillarbox rather than stretching to 16:10, and pace to the
MVS 59.1856 Hz refresh.

## What was not changed

* No ROM addresses, patches or branches specific to KOF '99 were added to
  the runtime.
* The recompiler, Musashi, z80 and ymfm are unmodified.
* The upstream MIT licence, NOTICE and LICENSES are preserved.
