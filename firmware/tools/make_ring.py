#!/usr/bin/env python3
"""Generate the ring tone embedded in the firmware.

Output: main/ring.pcm -- raw signed 16-bit little-endian mono, 16 kHz.
A synthesized two-bell "brrring" (no recording of anything or anyone).
Re-run after changing parameters:  python3 tools/make_ring.py
"""
import math, os, struct

RATE = 16000
SECONDS = 1.6          # one ring burst; firmware adds the silence between bursts
F1, F2 = 1100.0, 1400.0  # two bell partials
WARBLE_HZ = 20.0       # clapper rate

out = bytearray()
n = int(RATE * SECONDS)
for i in range(n):
    t = i / RATE
    # clapper alternates between the two bells, with a soft-edged gate
    gate = 0.5 * (1 + math.sin(2 * math.pi * WARBLE_HZ * t))
    s = gate * math.sin(2 * math.pi * F1 * t) + (1 - gate) * math.sin(2 * math.pi * F2 * t)
    s += 0.25 * math.sin(2 * math.pi * 2 * F1 * t) * gate  # a little brightness
    # 10 ms fade in/out so the burst doesn't click
    edge = min(1.0, t / 0.01, (SECONDS - t) / 0.01)
    out += struct.pack("<h", int(max(-1, min(1, s * 0.6 * edge)) * 32767))

path = os.path.join(os.path.dirname(__file__), "..", "main", "ring.pcm")
with open(path, "wb") as f:
    f.write(out)
print(f"wrote {len(out)} bytes to {os.path.normpath(path)}")
