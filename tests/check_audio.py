#!/usr/bin/env python3
"""Fail if a captured session has music-sized silent gaps.

The Garou bug this guards against did not silence the game: one-shot ADPCM
effects kept firing while the banked music stream stopped. A whole-file RMS
check therefore passes on the broken build. This checker looks for *runs* of
near-silent one-second windows after the boot sequence, which is what a
stopped BGM sounds like.

Usage:
    check-audio.py WAV [--from SEC] [--to SEC] [--max-silence SEC]
                       [--silence-peak N] [--quiet-rms N] [--min-median-rms N]

Defaults: --from 8 --to end --max-silence 1.5 --silence-peak 32
          --quiet-rms 0 (disabled) --min-median-rms 0 (disabled)

Exit code 0 = pass, 1 = a gap or level check failed, 2 = bad input.
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
    ap.add_argument("--from", dest="start", type=float, default=8.0)
    ap.add_argument("--to", dest="end", type=float, default=None)
    ap.add_argument("--max-silence", type=float, default=1.5)
    ap.add_argument("--silence-peak", type=int, default=32)
    ap.add_argument("--quiet-rms", type=float, default=0.0)
    ap.add_argument("--min-median-rms", type=float, default=0.0)
    a = ap.parse_args()

    try:
        s, n, rate, ch = load_mono(a.wav)
    except Exception as e:  # noqa: BLE001 - report and exit 2
        print("audio-check: %s: %s" % (a.wav, e))
        return 2
    if n == 0:
        print("audio-check: %s is empty" % a.wav)
        return 1
    secs = n // rate
    per = rate * ch
    end = secs if a.end is None else min(secs, int(a.end))
    start = max(0, int(a.start))

    rms_per_sec = []
    silent = []
    for t in range(start, end):
        chunk = s[t * per:(t + 1) * per]
        peak = max(abs(x) for x in chunk) if len(chunk) else 0
        rms = math.sqrt(sum(x * x for x in chunk) / len(chunk)) if len(chunk) else 0.0
        rms_per_sec.append(rms)
        silent.append(peak < a.silence_peak)

    runs = []
    run_start = None
    for i, is_silent in enumerate(silent):
        t = start + i
        if is_silent and run_start is None:
            run_start = t
        if not is_silent and run_start is not None:
            runs.append((run_start, t - 1))
            run_start = None
    if run_start is not None:
        runs.append((run_start, end - 1))
    runs = [r for r in runs if (r[1] - r[0] + 1) >= a.max_silence]

    quiet = [start + i for i, v in enumerate(rms_per_sec) if v < a.quiet_rms]
    median = sorted(rms_per_sec)[len(rms_per_sec) // 2] if rms_per_sec else 0.0

    print("audio-check: %s" % a.wav)
    print("  window %ds..%ds  median RMS %.1f" % (start, end, median))
    print("  longest silent run %.1fs (limit %.1fs)%s"
          % (max((r[1] - r[0] + 1) for r in runs) if runs else 0.0, a.max_silence,
             "" if not runs else ": " + ", ".join("%d-%ds" % r for r in runs)))
    if quiet:
        print("  %d quiet seconds below RMS %.1f (first %d)"
              % (len(quiet), a.quiet_rms, quiet[0]))

    ok = True
    if runs:
        ok = False
    if quiet:
        ok = False
    if median < a.min_median_rms:
        print("  median RMS below required %.1f" % a.min_median_rms)
        ok = False
    print("  result:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
