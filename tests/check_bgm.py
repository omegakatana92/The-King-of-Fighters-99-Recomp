#!/usr/bin/env python3
"""KOF '99 BGM continuity check.

The M1-bank bug fixed in neogeorecomp did not silence the game: one-shot
ADPCM effects kept firing while the banked music stream stopped.  A whole-file
RMS check or a peak check therefore passes on a broken build.  This checker
looks for sustained, music-sized energy in every 10-second window after the
BIOS boot, and for silent runs longer than 2 seconds.

Usage:
    check_bgm.py WAV [--from SEC] [--min-median-rms N] [--window SEC]
                     [--min-window-rms N] [--min-good-fraction F]
                     [--max-silence SEC]

Defaults: --from 10 --min-median-rms 1500 --window 10 --min-window-rms 800
          --min-good-fraction 0.9 --max-silence 2

Exit code 0 = pass, 1 = a continuity check failed, 2 = bad input.
"""
import argparse
import math
import sys
import wave


def load_mono(path):
    with wave.open(path, "rb") as w:
        n, rate, ch, sw = w.getnframes(), w.getframerate(), w.getnchannels(), w.getsampwidth()
        if sw != 2:
            raise ValueError("expected 16-bit PCM, got %d-byte samples" % sw)
        data = w.readframes(n)
    samples = memoryview(data).cast("h")
    return samples, n, rate, ch


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wav")
    ap.add_argument("--from", dest="start", type=float, default=10.0)
    ap.add_argument("--min-median-rms", type=float, default=1500.0)
    ap.add_argument("--window", type=float, default=10.0)
    ap.add_argument("--min-window-rms", type=float, default=800.0)
    ap.add_argument("--min-good-fraction", type=float, default=0.9)
    ap.add_argument("--max-silence", type=float, default=2.0)
    a = ap.parse_args()

    try:
        s, n, rate, ch = load_mono(a.wav)
    except Exception as e:  # noqa: BLE001 - report and exit 2
        print("bgm-check: %s: %s" % (a.wav, e))
        return 2
    if n == 0:
        print("bgm-check: %s is empty" % a.wav)
        return 1

    secs = n // rate
    start = max(0, int(a.start))
    if secs <= start + 1:
        print("bgm-check: only %ds of audio, need more than %ds" % (secs, start))
        return 1

    per = rate * ch
    rms = []
    peak = []
    for t in range(secs):
        chunk = s[t * per:(t + 1) * per]
        if ch == 2:
            total = 0
            pk = 0
            count = 0
            for i in range(0, len(chunk) - 1, 2):
                v = (int(chunk[i]) + int(chunk[i + 1])) // 2
                total += v * v
                av = v if v >= 0 else -v
                if av > pk:
                    pk = av
                count += 1
            rms.append(math.sqrt(total / count) if count else 0.0)
            peak.append(pk)
        else:
            total = 0
            pk = 0
            for v in chunk:
                v = int(v)
                total += v * v
                av = v if v >= 0 else -v
                if av > pk:
                    pk = av
            rms.append(math.sqrt(total / len(chunk)) if len(chunk) else 0.0)
            peak.append(pk)

    seg = rms[start:]
    median = sorted(seg)[len(seg) // 2] if seg else 0.0

    run = best = 0
    best_at = 0
    at = start
    for i in range(start, secs):
        if peak[i] < 32:
            if run == 0:
                at = i
            run += 1
            if run > best:
                best, best_at = run, at
        else:
            run = 0

    win = max(1, int(a.window))
    means = []
    for i in range(start, secs - win + 1, win):
        means.append((i, sum(rms[i:i + win]) / win))
    good = [m for m in means if m[1] >= a.min_window_rms]
    frac = (len(good) / len(means)) if means else 0.0

    print("bgm-check: %s" % a.wav)
    print("  %ds total, analysis from %ds" % (secs, start))
    print("  median RMS %.1f (minimum %.1f)" % (median, a.min_median_rms))
    print("  longest near-silent run %.0fs at %ds (limit %.1fs)"
          % (best, best_at, a.max_silence))
    print("  %d/%d %ds windows at or above RMS %.0f (%.0f%%, need %.0f%%)"
          % (len(good), len(means), win, a.min_window_rms,
             100.0 * frac, 100.0 * a.min_good_fraction))
    if means:
        print("  window means: " + " ".join("%d:%.0f" % m for m in means))

    ok = True
    if median < a.min_median_rms:
        ok = False
    if best > a.max_silence:
        ok = False
    if frac < a.min_good_fraction:
        ok = False
    print("  result:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
