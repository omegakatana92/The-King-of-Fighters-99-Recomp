# Changelog

## 0.1.0 - 2026-10-27

First working native recompilation of The King of Fighters '99 (MAME
`kof99`, original encrypted MVS set).

* Added `tools/decrypt.c`: CRC-checked NEO-SMA program decryption and
  banking data, NEO-CMC42 sprite decryption (key `0x00`) and 128 KB
  fixed-layer extraction from the C data.  Inputs match MAME's `kof99` set.
* Added the verified `ng_game_t` (`src/game.c`) with the KOF '99 NEO-SMA
  bank table, `$9A37` protection read and LFSR addresses.
* Added the Windows player, Linux player, generator, ROM discovery/picker
  and first-run decryption helper.
* Boots the real MVS BIOS, runs title, intro and attract mode, accepts a
  coin/start, walks HOW TO PLAY, character/striker/order select and plays a
  match with all four buttons.
* Measured: 39,675 dispatch entries, 1,343,495 instructions, 0 `--verify`
  mismatches, 99.99% native over a 24,000-frame session and 100% over the
  6,000-frame verify session.
* Added the Windows package, Steam Deck AppImage packaging and the docs.
* Verified the M1 bank-mask fix and per-frame `audio_sync()` are present,
  added `check_bgm.py` (39/39 windows with BGM-sized energy) and compared a
  90 s capture against MAME 0.289 (chroma similarity 0.935, 0.99+ in the
  first 50 s).
* Adopted the generic sprite-buffer zero-fill fix in the vendored toolkit.

Known limitations: bank-window fallback to Musashi; physical Steam Deck,
Steam Input and physical gamepads untested; audio not subjectively audited.
