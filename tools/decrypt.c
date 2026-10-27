/*
 * decrypt.c - The King of Fighters '99 (MAME set "kof99") ROM preparation.
 *
 * The supplied kof99.zip is SNK's original encrypted MVS release:
 *   - program: NEO-SMA protected (251-p1.p1, 251-p2.p2, ka.neo-sma)
 *   - graphics: NEO-CMC42 protected (251-c1..c8)
 *   - fixed layer: not a separate ROM on this board; it is stored at the end
 *     of the C data, as MAME's sfix_decrypt() extracts it
 *   - audio: 251-m1.m1 and 251-v1..v4 are plain
 *
 * This build-time tool reads those files and writes a runtime-ready set:
 *
 *   251-pd.bin    9 MB program image (1 MB fixed + 8 MB bank, decrypted)
 *   251-c1..c8    CMC42-decrypted sprite ROMs (8 MB each)
 *   251-s1.s1     128 KB fixed-layer ROM extracted from the C data
 *   plus the M1, V ROMs and the MVS BIOS files, copied unchanged.
 *
 * The decryption algorithms and tables are derived from MAME's Neo Geo
 * SMA/CMC device sources, which are BSD-3-Clause:
 *
 *   prot_sma.cpp  copyright-holders: S. Smith, David Haywood, Fabio Priuli,
 *                 Razoola, Mr.K
 *   prot_cmc.cpp  copyright-holders: S. Smith, David Haywood, Fabio Priuli
 *
 * The tables live in tables.h.  See THIRD_PARTY_NOTICES.md.  No ROM data is
 * written outside the output directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <direct.h>
#define KOF99_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define KOF99_MKDIR(p) mkdir((p), 0777)
#endif
#include "tables.h"

#define PROM_FIXED   0x100000u
#define PROM_BANK    0x800000u
#define PROM_SIZE    (PROM_FIXED + PROM_BANK)     /* 9 MB decrypted image */
#define CROM_SIZE    0x4000000u                   /* 64 MB interleaved */
#define SROM_SIZE    0x20000u                     /* 128 KB fixed layer */
#define SMA_SIZE     0x40000u                     /* 256 KB stored in the chip */

/* MAME's bitswap<N>(value, b0, b1, ...): output bit N-1-i is input bit b[i]. */
static uint32_t bitswap(uint32_t val, const uint8_t *bits, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++)
        r |= ((val >> bits[i]) & 1u) << (n - 1 - i);
    return r;
}

/* ---------------- CRC32 (for input validation) ---------------- */

static uint32_t s_crc_table[256];
static int s_crc_ready = 0;

static void crc_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        s_crc_table[i] = c;
    }
    s_crc_ready = 1;
}

static uint32_t crc32_buf(const uint8_t *p, size_t n) {
    if (!s_crc_ready) crc_init();
    uint32_t c = 0xFFFFFFFFu;
    while (n--) c = s_crc_table[(c ^ *p++) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

/* ---------------- file helpers ---------------- */

typedef struct {
    const char *name;
    uint32_t    size;
    uint32_t    crc;
} rom_expect_t;

static uint8_t *read_file(const char *dir, const char *name, size_t *size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "decrypt: cannot open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(buf); return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

static int write_file(const char *dir, const char *name, const uint8_t *data, size_t size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "decrypt: cannot write %s\n", path);
        return -1;
    }
    if (fwrite(data, 1, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

/* Read a file, check its size and CRC32 against MAME's values. */
static uint8_t *read_checked(const char *dir, const rom_expect_t *e) {
    size_t n = 0;
    uint8_t *d = read_file(dir, e->name, &n);
    if (!d) return NULL;
    if (n != e->size) {
        fprintf(stderr, "decrypt: %s is %u bytes, expected %u\n",
                e->name, (unsigned)n, (unsigned)e->size);
        free(d);
        return NULL;
    }
    uint32_t crc = crc32_buf(d, n);
    if (crc != e->crc) {
        fprintf(stderr, "decrypt: %s has CRC32 %08X, expected %08X\n",
                e->name, (unsigned)crc, (unsigned)e->crc);
        free(d);
        return NULL;
    }
    return d;
}

/* MAME loads P ROMs with ROM_LOAD16_WORD_SWAP: the file bytes are swapped
 * into the 68000's big-endian word order. */
static void copy_swapped(uint8_t *dst, const uint8_t *src, size_t n) {
    for (size_t i = 0; i + 1 < n; i += 2) {
        dst[i] = src[i + 1];
        dst[i + 1] = src[i];
    }
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

/* ---------------- program ROM (NEO-SMA) ---------------- */

static const rom_expect_t k_prom_files[3] = {
    {"ka.neo-sma", 0x040000, 0x7766D09E},
    {"251-p1.p1",  0x400000, 0x006E4532},
    {"251-p2.p2",  0x400000, 0x90175F15},
};

static int decrypt_prom(const char *dir, const char *outdir) {
    uint8_t *sma = read_checked(dir, &k_prom_files[0]);
    uint8_t *p1  = read_checked(dir, &k_prom_files[1]);
    uint8_t *p2  = read_checked(dir, &k_prom_files[2]);
    if (!sma || !p1 || !p2) { free(sma); free(p1); free(p2); return -1; }

    uint8_t *region = (uint8_t *)calloc(1, PROM_SIZE);
    if (!region) { free(sma); free(p1); free(p2); return -1; }

    /* MAME's maincpu region layout: SMA data at $0C0000, P1 at $100000,
     * P2 at $500000.  Everything is stored big-endian after the swap. */
    copy_swapped(region + 0x0C0000, sma, SMA_SIZE);
    copy_swapped(region + 0x100000, p1, 0x400000);
    copy_swapped(region + 0x500000, p2, 0x400000);
    free(sma); free(p1); free(p2);

    /* MAME sma_prot_device::kof99_decrypt_68k(). */
    static const uint8_t data_bits[16] =
        {13, 7, 3, 0, 9, 4, 5, 6, 1, 12, 8, 14, 10, 11, 2, 15};
    static const uint8_t bank_bits[10] =
        {6, 2, 4, 9, 8, 3, 1, 7, 0, 5};
    static const uint8_t fixed_bits[19] =
        {18, 11, 6, 14, 17, 16, 5, 8, 10, 12, 0, 4, 3, 2, 7, 9, 15, 13, 1};

    /* Swap data lines on the whole 8 MB program image. */
    for (uint32_t i = 0; i < PROM_BANK / 2; i++) {
        uint8_t *p = region + 0x100000 + 2 * i;
        wr16(p, (uint16_t)bitswap(rd16(p), data_bits, 16));
    }

    /* Swap address lines for the first 6 MB of the banked part. */
    uint8_t *buffer = (uint8_t *)malloc(0x800);
    if (!buffer) { free(region); return -1; }
    for (uint32_t i = 0; i < 0x600000 / 2; i += 0x800 / 2) {
        uint8_t *base = region + 0x100000 + 2 * i;
        memcpy(buffer, base, 0x800);
        for (uint32_t j = 0; j < 0x800 / 2; j++) {
            const uint8_t *src = buffer + 2 * bitswap(j, bank_bits, 10);
            base[2 * j]     = src[0];
            base[2 * j + 1] = src[1];
        }
    }
    free(buffer);

    /* Relocate the fixed 768 KB of code from the banked image.  The 256 KB
     * SMA data at $0C0000..$0FFFFF stays where it was loaded. */
    for (uint32_t i = 0; i < 0x0C0000 / 2; i++) {
        const uint8_t *src = region + 0x700000 + 2 * bitswap(i, fixed_bits, 19);
        uint8_t *dst = region + 2 * i;
        dst[0] = src[0]; dst[1] = src[1];
    }

    /* Sanity: the fixed bank must now carry SNK's cartridge header. */
    if (memcmp(region + 0x100, "NEO-GEO", 7) != 0) {
        fprintf(stderr, "decrypt: error: no NEO-GEO signature at $100 after P ROM decryption\n");
        free(region);
        return -1;
    }
    uint32_t sp = ((uint32_t)region[0] << 24) | ((uint32_t)region[1] << 16) |
                  ((uint32_t)region[2] << 8) | region[3];
    uint32_t pc = ((uint32_t)region[4] << 24) | ((uint32_t)region[5] << 16) |
                  ((uint32_t)region[6] << 8) | region[7];
    fprintf(stderr, "decrypt: P ROM -> 251-pd.bin (SP=%08X PC=%08X)\n",
            (unsigned)sp, (unsigned)pc);

    int rc = write_file(outdir, "251-pd.bin", region, PROM_SIZE);
    free(region);
    return rc;
}

/* ---------------- graphics ROM (NEO-CMC42) ---------------- */

static void gfx_byte(uint8_t *r0, uint8_t *r1, uint8_t c0, uint8_t c1,
                     const uint8_t *t0hi, const uint8_t *t0lo, const uint8_t *t1,
                     uint32_t base, int invert) {
    uint8_t tmp  = t1[(base & 0xff) ^ kof99_address_0_7_xor[(base >> 8) & 0xff]];
    uint8_t xor0 = (uint8_t)((t0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    uint8_t xor1 = (uint8_t)((tmp & 0xfe) | (t0lo[(base >> 8) & 0xff] & 0x01));
    if (invert) { *r0 = (uint8_t)(c1 ^ xor0); *r1 = (uint8_t)(c0 ^ xor1); }
    else        { *r0 = (uint8_t)(c0 ^ xor0); *r1 = (uint8_t)(c1 ^ xor1); }
}

/* MAME cmc_prot_device::gfx_decrypt().  extra_xor is KOF99_GFX_KEY = 0x00. */
static void gfx_decrypt(uint8_t *rom, uint32_t rom_size, uint32_t extra_xor) {
    uint8_t *buf = (uint8_t *)malloc(rom_size);
    if (!buf) return;
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        gfx_byte(&buf[4*rpos], &buf[4*rpos+3], rom[4*rpos], rom[4*rpos+3],
                 kof99_type0_t03, kof99_type0_t12, kof99_type1_t03, rpos,
                 (int)((rpos >> 8) & 1));
        gfx_byte(&buf[4*rpos+1], &buf[4*rpos+2], rom[4*rpos+1], rom[4*rpos+2],
                 kof99_type0_t12, kof99_type0_t03, kof99_type1_t12, rpos,
                 (int)(((rpos >> 16) ^ kof99_address_16_23_xor2[(rpos >> 8) & 0xff]) & 1));
    }
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = rpos ^ extra_xor;
        baser ^= (uint32_t)kof99_address_8_15_xor1[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)kof99_address_8_15_xor2[baser & 0xff] << 8;
        baser ^= (uint32_t)kof99_address_16_23_xor1[baser & 0xff] << 16;
        baser ^= (uint32_t)kof99_address_16_23_xor2[(baser >> 8) & 0xff] << 16;
        baser ^= kof99_address_0_7_xor[(baser >> 8) & 0xff];
        baser &= (rom_size / 4) - 1;
        rom[4*rpos]     = buf[4*baser];
        rom[4*rpos + 1] = buf[4*baser + 1];
        rom[4*rpos + 2] = buf[4*baser + 2];
        rom[4*rpos + 3] = buf[4*baser + 3];
    }
    free(buf);
}

/* MAME cmc_prot_device::sfix_decrypt(): the S data is the last `fixed_size`
 * bytes of the C data, with its columns permuted. */
static void sfix_extract(const uint8_t *rom, uint32_t rom_size,
                         uint8_t *fixed, uint32_t fixed_size) {
    const uint8_t *src = rom + rom_size - fixed_size;
    for (uint32_t i = 0; i < fixed_size; i++)
        fixed[i] = src[(i & ~0x1fu) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
}

static const rom_expect_t k_crom_files[8] = {
    {"251-c1.c1", 0x800000, 0x0F9E93FE},
    {"251-c2.c2", 0x800000, 0xE71E2EA3},
    {"251-c3.c3", 0x800000, 0x238755D2},
    {"251-c4.c4", 0x800000, 0x438C8B22},
    {"251-c5.c5", 0x800000, 0x0B0ABD0A},
    {"251-c6.c6", 0x800000, 0x65BBF281},
    {"251-c7.c7", 0x800000, 0xFF65F62E},
    {"251-c8.c8", 0x800000, 0x8D921C68},
};

static int decrypt_crom(const char *dir, const char *outdir) {
    uint8_t *c[8] = {0};
    for (int i = 0; i < 8; i++) {
        c[i] = read_checked(dir, &k_crom_files[i]);
        if (!c[i]) {
            for (int j = 0; j < i; j++) free(c[j]);
            return -1;
        }
    }

    uint8_t *crom = (uint8_t *)malloc(CROM_SIZE);
    if (!crom) {
        for (int i = 0; i < 8; i++) free(c[i]);
        return -1;
    }
    for (int pair = 0; pair < 4; pair++) {
        uint32_t base = 0x800000u * 2 * (uint32_t)pair;
        const uint8_t *even = c[pair * 2];
        const uint8_t *odd  = c[pair * 2 + 1];
        for (uint32_t k = 0; k < 0x800000; k++) {
            crom[base + 2*k]     = even[k];
            crom[base + 2*k + 1] = odd[k];
        }
    }
    gfx_decrypt(crom, CROM_SIZE, 0x00);   /* KOF99_GFX_KEY */

    uint8_t *srom = (uint8_t *)malloc(SROM_SIZE);
    if (!srom) { free(crom); for (int i = 0; i < 8; i++) free(c[i]); return -1; }
    sfix_extract(crom, CROM_SIZE, srom, SROM_SIZE);
    if (write_file(outdir, "251-s1.s1", srom, SROM_SIZE)) {
        free(srom); free(crom); for (int i = 0; i < 8; i++) free(c[i]); return -1;
    }
    free(srom);
    fprintf(stderr, "decrypt: S ROM -> 251-s1.s1 (0x%X bytes)\n", SROM_SIZE);

    for (int pair = 0; pair < 4; pair++) {
        uint32_t base = 0x800000u * 2 * (uint32_t)pair;
        for (uint32_t k = 0; k < 0x800000; k++) {
            c[pair * 2][k]     = crom[base + 2*k];
            c[pair * 2 + 1][k] = crom[base + 2*k + 1];
        }
        if (write_file(outdir, k_crom_files[pair * 2].name, c[pair * 2], 0x800000)) {
            free(crom); for (int i = 0; i < 8; i++) free(c[i]); return -1;
        }
        if (write_file(outdir, k_crom_files[pair * 2 + 1].name, c[pair * 2 + 1], 0x800000)) {
            free(crom); for (int i = 0; i < 8; i++) free(c[i]); return -1;
        }
    }
    free(crom);
    for (int i = 0; i < 8; i++) free(c[i]);
    fprintf(stderr, "decrypt: C ROMs -> 251-c1..c8 (8 MB each)\n");
    return 0;
}

static const rom_expect_t k_copy_files[] = {
    {"251-m1.m1",    0x020000, 0x5E74539C},
    {"251-v1.v1",    0x400000, 0xEF2EECC8},
    {"251-v2.v2",    0x400000, 0x73E211CA},
    {"251-v3.v3",    0x400000, 0x821901DA},
    {"251-v4.v4",    0x200000, 0xB49E6178},
    {"sp-s2.sp1",    0x020000, 0x9036D879},
    {"sfix.sfix",    0x020000, 0xC2EA0CFD},
    {"sm1.sm1",      0x020000, 0x94416D67},
    {"000-lo.lo",    0x020000, 0x5A86CFF2},
};

int main(int argc, char **argv) {
    const char *romdir = "roms-src", *outdir = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path") && i + 1 < argc) romdir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr,
                "usage: kof99_decrypt [--rom-path DIR] [--out DIR]\n"
                "  DIR must contain the unzipped MAME kof99 and neogeo sets.\n"
                "  Output is a runtime-ready set; no ROM data is bundled.\n");
            return 0;
        } else {
            fprintf(stderr, "decrypt: unknown argument %s\n", argv[i]);
            return 2;
        }
    }
    KOF99_MKDIR(outdir);
    if (decrypt_prom(romdir, outdir)) return 1;
    if (decrypt_crom(romdir, outdir)) return 1;
    for (size_t i = 0; i < sizeof(k_copy_files) / sizeof(k_copy_files[0]); i++) {
        uint8_t *d = read_checked(romdir, &k_copy_files[i]);
        if (!d) return 1;
        int rc = write_file(outdir, k_copy_files[i].name, d, k_copy_files[i].size);
        free(d);
        if (rc) return 1;
    }
    fprintf(stderr, "decrypt: wrote a complete ROM set to %s\n", outdir);
    return 0;
}
