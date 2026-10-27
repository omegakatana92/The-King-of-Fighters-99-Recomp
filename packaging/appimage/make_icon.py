#!/usr/bin/env python3
"""Draw an original 256x256 icon for the KOF '99 recompilation.

No SNK artwork is used: a dark field, a red diagonal band, and block letters
drawn from a tiny bitmap font.
"""
import os, struct, zlib

W = H = 256
px = bytearray(W * H * 3)

def put(x, y, r, g, b):
    if 0 <= x < W and 0 <= y < H:
        i = (y * W + x) * 3
        px[i] = r; px[i+1] = g; px[i+2] = b

# background: near-black with a warm vertical gradient
for y in range(H):
    for x in range(W):
        put(x, y, 10 + y // 26, 8 + y // 40, 9 + y // 64)

# red diagonal band
for y in range(H):
    for x in range(W):
        if 40 <= (x + y) % 256 <= 74:
            put(x, y, 172, 20, 26)

# black drop panel behind the letters
for y in range(86, 178):
    for x in range(10, 246):
        put(x, y, 8, 8, 12)

# 5x7 block font, scaled
FONT = {
    'K': ["10001","10010","10100","11000","10100","10010","10001"],
    'O': ["01110","10001","10001","10001","10001","10001","01110"],
    'F': ["11111","10000","10000","11110","10000","10000","10000"],
    '9': ["01110","10001","10001","01111","00001","00010","01100"],
}
text = "KOF99"
scale = 6
tw = (len(text) * 5 + (len(text) - 1)) * scale
tx = (W - tw) // 2
ty = (H - 7 * scale) // 2
for ci, ch in enumerate(text):
    for gy, row in enumerate(FONT[ch]):
        for gx, bit in enumerate(row):
            if bit == '1':
                for yy in range(scale):
                    for xx in range(scale):
                        put(tx + (ci * 6 + gx) * scale + xx,
                            ty + gy * scale + yy, 246, 246, 246)

# small yellow accent bar under the letters
for y in range(190, 198):
    for x in range(64, 192):
        put(x, y, 238, 190, 40)

raw = bytearray()
for y in range(H):
    raw.append(0)
    raw += px[y*W*3:(y+1)*W*3]

def chunk(t, d):
    return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

out = b'\x89PNG\r\n\x1a\n'
out += chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
out += chunk(b'IDAT', zlib.compress(bytes(raw), 9))
out += chunk(b'IEND', b'')
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "kof99.png")
open(path, 'wb').write(out)
print(path)
