/*
 * game.c - The King of Fighters '99 (MAME set "kof99"), the verified ROM
 * layout shared by the player and the recompiler.
 *
 * The supplied set is SNK's original encrypted MVS release.  tools/decrypt.c
 * turns it into the files named here at build time, in the build directory:
 *
 *   251-pd.bin      9 MB program: 1 MB fixed bank + 8 MB banked image
 *   251-c1..c8      8 MB each, NEO-CMC42 decrypted
 *   251-s1.s1       128 KB fixed layer extracted from the end of the C data
 *
 * The decrypted program keeps the original NEO-SMA layout.  The cartridge
 * chooses a 1 MB window from a scrambled 6-bit selector through the NEO-SMA
 * mapper at $2FFFF0; the selector bits and the offset table come from MAME's
 * sma_prot_device::kof99_bank_base().  The same chip returns $9A37 at
 * $2FE446 and its 16-bit LFSR at $2FFFF8 and $2FFFFA.
 *
 * The 256 KB of program data stored inside the SMA chip itself is kept at
 * $0C0000..$0FFFFF of the fixed bank (MAME loads ka.neo-sma there).
 */
#include "game.h"

/* kof99_bank_base() offsets, relative to the start of the 8 MB banked
 * image.  MAME stores them as 0x100000 + offset in the full 9 MB cartridge
 * region.  Only the first 33 entries are used by this cart. */
static const uint32_t k_kof99_bank_offsets[64] = {
    0x000000, 0x100000, 0x200000, 0x300000,  /* 00 */
    0x3CC000, 0x4CC000, 0x3F2000, 0x4F2000,  /* 04 */
    0x407800, 0x507800, 0x40D000, 0x50D000,  /* 08 */
    0x417800, 0x517800, 0x420800, 0x520800,  /* 12 */
    0x424800, 0x524800, 0x429000, 0x529000,  /* 16 */
    0x42E800, 0x52E800, 0x431800, 0x531800,  /* 20 */
    0x54D000, 0x551000, 0x567000, 0x592800,  /* 24 */
    0x588800, 0x581800, 0x599800, 0x594800,  /* 28 */
    0x598000,                                 /* 32; the rest are unused */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const ng_bank_map_t k_kof99_bank_map = {
    .offsets = k_kof99_bank_offsets,
    .count = 64,
    .bits = {5, 12, 10, 8, 6, 14},   /* bitswap<6>(sel, 5,12,10,8,6,14) */
};

const ng_game_t k_kof99 = {
    .name = "kof99",
    .title = "The King of Fighters '99 - Millennium Battle",
    .prom_size = 0x900000,          /* 1 MB fixed + 8 MB banked */
    .prom = {
        {"251-pd.bin", 0x000000, 0x000000, 0x900000, 0},  /* decrypted image */
    },
    .crom_size = 0x4000000,         /* 64 MB after pairing */
    .crom_pairs = {
        {"251-c1.c1", "251-c2.c2"},
        {"251-c3.c3", "251-c4.c4"},
        {"251-c5.c5", "251-c6.c6"},
        {"251-c7.c7", "251-c8.c8"},
    },
    .srom = "251-s1.s1",            /* 128 KB, extracted from the C data */
    .m1 = "251-m1.m1",              /* plain Z80 program on this board */
    .vrom = {"251-v1.v1", "251-v2.v2", "251-v3.v3", "251-v4.v4"},
    .fix_bank_type = NG_FIX_BANK_NONE,
    .bank_map = &k_kof99_bank_map,
    .bank_write_addr = 0x2FFFF0,    /* NEO-SMA bank select (wiki: $2FFFF1) */
    .prot_read_addr = 0x2FE446,     /* NEO-SMA protection read */
    .prot_value = 0x9A37,
    .rng_read_addr = {0x2FFFF8, 0x2FFFFA},  /* NEO-SMA LFSR */
    .rng_seed = 0x2345,
};
