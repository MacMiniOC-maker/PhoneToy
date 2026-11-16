# Virtual testing

Checks you can run before any part is on the bench. One command runs all of them:

```sh
sh tools/virtual_test.sh
```

| Check | What it catches | Tool |
|---|---|---|
| Pin map rules | a pin used twice, a reserved pin (boot, USB, memory), README table out of step with the firmware | `tools/check_pins.py` |
| Firmware logic | wrong ring / answer / hang-up behaviour, switch bounce counted twice, impossible states | `firmware/test/test_logic.c`, compiled and run on your computer |
| Circuit simulation | LED driver currents, the clock signal on the handset cable | `sim/*.cir` with ngspice |

Results of the last run are in [`RESULTS.md`](RESULTS.md), and the safety audit is in
[`SAFETY.md`](SAFETY.md): load against rating for every part, loudness estimates, and what
happens with each common wiring mistake. The run fails if any part is over its rating.

## What the simulations say

1. **The transistor light driver is sound.** With any built-in resistor from 150 Ω to 1 kΩ the
   2N2222 is fully on, the GPIO pin supplies about 2.5 mA (well inside its limit) and the
   transistor stays cold. If the button's LED has **no** resistor, the model shows about 240 mA
   through it, which would destroy the LED. That is why the build plan says to add 220 Ω if
   the listing does not mention one.
2. **Add 100 Ω resistors to the mic's clock wires.** In the model, the bit clock overshoots the
   mic's voltage limits at the far end of an unterminated wire, even a short one. A 68–100 Ω
   resistor in series, at the breadboard end of the mic's `SCK` and `WS` wires, removes it.
   The model is a worst case (ideal cable, fast edges, no protection diodes), so the real
   overshoot will be smaller, but the resistors cost nothing from the kit.
3. **USB power is fine.** About 215 mA typical and 455 mA on Wi-Fi peaks.

What a simulation cannot tell you: whether a solder joint is good, which transistor leg is
which on your parts, how loud the earpiece is, or whether a call has echo. Those need the bench.

## Opening the circuits yourself

The `.cir` files are plain SPICE netlists.

- **ngspice:** `ngspice -b sim/led_driver.cir`
- **LTspice:** File → Open, choose the `.cir` file, then Run. Change a `.param` line to try other values.
  LTspice does not use the `.control … .endc` block; delete those lines if it complains.
- **KiCad:** its simulator is ngspice, so the same netlists and models apply. There is no KiCad
  schematic in this repo yet; the pin-rules script covers the same ground as KiCad's electrical
  rules check for this wiring.
- **Falstad:** best for building the light driver by hand and watching current flow: a 5 V source,
  an LED, a resistor, an NPN transistor, and a 1 kΩ resistor from a 3.3 V source to the base.
  Compare its currents with the first table in `RESULTS.md`.
