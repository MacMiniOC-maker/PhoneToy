#!/usr/bin/env python3
"""Electrical-rules check for the pin map, without any hardware.

Reads firmware/main/config.h and checks every pin against what the
ESP32-S3-DevKitC-1 N16R8 allows, then checks the README wiring table agrees.
    python3 tools/check_pins.py
"""
import os, re, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
cfg = open(os.path.join(ROOT, "firmware/main/config.h")).read()
readme = open(os.path.join(ROOT, "README.md")).read()

HEADER = {4,5,6,7,15,16,17,18,8,3,46,9,10,11,12,13,14, 43,44,1,2,42,41,40,39,38,37,36,35,0,45,48,47,21,20,19}
FORBIDDEN = {
    0: "strapping pin (boot mode)", 3: "strapping pin (JTAG select)", 45: "strapping pin (flash voltage)",
    46: "strapping pin (boot mode)", 19: "native USB D-", 20: "native USB D+",
    35: "octal PSRAM", 36: "octal PSRAM", 37: "octal PSRAM", 43: "UART console TX", 44: "UART console RX",
}
NOTES = {38: "also the on-board RGB LED on v1.1 boards (harmless)", 48: "on-board RGB LED on v1.0 boards"}

def one(name):
    m = re.search(rf"#define {name}\s+(\d+)", cfg)
    assert m, name
    return int(m.group(1))

def many(name):
    m = re.search(rf"#define {name}\s+\{{([^}}]+)\}}", cfg)
    assert m, name
    return [int(x) for x in m.group(1).split(",")]

pins = {}
def add(role, gpio):
    pins.setdefault(gpio, []).append(role)

for i, g in enumerate(many("PT_BUTTON_GPIOS")): add(f"button {i+1} switch", g)
for i, g in enumerate(many("PT_LED_GPIOS")): add(f"button {i+1} light", g)
add("hook switch", one("PT_HOOK_GPIO"))
add("I2S BCLK", one("PT_I2S_BCLK_GPIO")); add("I2S LRC/WS", one("PT_I2S_WS_GPIO"))
add("I2S data out (amps)", one("PT_I2S_DOUT_GPIO")); add("I2S data in (mic)", one("PT_I2S_DIN_GPIO"))
add("earpiece amp SD", one("PT_EARPIECE_AMP_SD_GPIO")); add("ringer amp SD", one("PT_RINGER_AMP_SD_GPIO"))

errors, warnings = [], []
table = readme[readme.index("## Wiring"):readme.index("Per button LED")]
for gpio, roles in sorted(pins.items()):
    if len(roles) > 1: errors.append(f"GPIO {gpio} is used twice: {', '.join(roles)}")
    if gpio not in HEADER: errors.append(f"GPIO {gpio} ({roles[0]}) is not on the DevKitC-1 headers")
    if gpio in FORBIDDEN: errors.append(f"GPIO {gpio} ({roles[0]}) is reserved: {FORBIDDEN[gpio]}")
    if gpio in NOTES: warnings.append(f"GPIO {gpio} ({roles[0]}): {NOTES[gpio]}")
    if not re.search(rf"\b{gpio}\b", table): errors.append(f"GPIO {gpio} ({roles[0]}) is missing from the README wiring table")
for g in re.findall(r"GPIO ([\d, /]+)", table):
    for n in re.findall(r"\d+", g):
        if int(n) not in pins: errors.append(f"README wiring table mentions GPIO {n}, which the firmware does not use")

print(f"{len(pins)} pins in use:")
for gpio, roles in sorted(pins.items()): print(f"  GPIO {gpio:>2}  {', '.join(roles)}")
for w in warnings: print("note:", w)
for e in errors: print("ERROR:", e)
print("pin map OK" if not errors else f"{len(errors)} problem(s)")
sys.exit(1 if errors else 0)
