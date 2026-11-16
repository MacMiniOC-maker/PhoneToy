# PhoneToy

A toddler "family phone". Four light-up photo buttons, one per family member. A
button glows when that person is available; pressing it places a live call
through a retro handset. No screen, no battery, cardboard box, USB power.

**No voice is ever recorded.** The firmware only passes audio through RAM, and
Twilio call recording is never enabled (see [Privacy](#privacy)).

## Status

| Milestone | What | State |
|---|---|---|
| 1 | Hardware check: buttons, LEDs, hook switch, ring tone, mic→earpiece loopback | **ready to test** |
| 2 | Schedule: Neon tables, schedule endpoint, Wi-Fi captive portal, LEDs on schedule | not started |
| 3 | Outbound calls over a Twilio SIP domain | not started (hardware assessment first) |
| 4 | Inbound calls | not started |

## Parts

These are the parts on the shopping list.

| Part | Role |
|---|---|
| ESP32-S3-DevKitC-1 **N16R8** (2-pack) | brain; the second board is a spare |
| Adafruit MAX98357A I2S amp (#3006) × 2 | one for the earpiece, one for the ringer |
| 28 mm 8 Ω mini speaker | earpiece, glued inside the handset |
| 3 W 4 Ω speaker | ringer |
| INMP441 I2S mic | handset mouthpiece |
| Old corded phone handset | shell only; its own electronics come out |
| Lever microswitch | hook switch under the handset cradle |
| Adafruit 30 mm LED arcade button × 4 | photo buttons |
| 2N2222 NPN transistor × 4 + 1 kΩ resistor × 4 | LED drivers |

## Button LEDs

The Adafruit 30 mm buttons have an LED with a built-in resistor meant for 5 V,
which is more than a 3.3 V GPIO pin should drive directly. So each LED runs
from the `5V` pin through a 2N2222. Quick check before wiring all four: touch
one LED's two tabs to `5V` and `GND` (swap them if it stays dark). It should
be clearly bright. If it gets hot or is blindingly bright, that button has no
built-in resistor: add 220 Ω in series.

## Wiring

All pins are defined in [`firmware/main/config.h`](firmware/main/config.h).

| Part | Part pin | ESP32-S3 pin |
|---|---|---|
| Both amplifiers | `Vin` | `5V` |
| Both amplifiers | `GND` | `GND` |
| Both amplifiers | `BCLK` | GPIO 5 |
| Both amplifiers | `LRC` | GPIO 6 |
| Both amplifiers | `DIN` | GPIO 7 |
| Earpiece amplifier | `SD` (on/off) | GPIO 17 |
| Ringer amplifier | `SD` (on/off) | GPIO 18 |
| Earpiece amplifier | `+` / `−` | 28 mm speaker, with 220 Ω in series on one wire |
| Ringer amplifier | `+` / `−` | 3 W speaker |
| Microphone | `VDD` | `3V3` (not 5V) |
| Microphone | `GND` and `L/R` | `GND` |
| Microphone | `SCK` | GPIO 5 (shared) |
| Microphone | `WS` | GPIO 6 (shared) |
| Microphone | `SD` (data) | GPIO 15 |
| Button switches 1 to 4 | one tab each | GPIO 9, 10, 11, 12 |
| Button switches 1 to 4 | other tab | `GND` |
| Button lights 1 to 4 | plus tab | `5V` |
| Button lights 1 to 4 | minus tab | transistor collector |
| Transistors 1 to 4 (2N2222) | base, through 4.7 kΩ (1 kΩ also works) | GPIO 38, 39, 40, 41 |
| Transistors 1 to 4 | emitter | `GND` |
| Hook switch | common tab | `GND` |
| Hook switch | normally-open tab | GPIO 16 |

This is the same pin map as the "Suggested wiring" table in the build plan.

Per button LED (low-side switch):

```
5V ──► LED + ... LED − ──► collector
GPIO ─[4.7 kΩ]─► base        (NPN: 2N2222 / BC337)
GND ◄── emitter
```

Other notes:

- Amp outputs (`+`/`−`) go straight to the earpiece / ringer speaker. Never
  connect either output wire to ground.
- Leave the amps' `GAIN` pins unconnected.
- Buttons and the hook switch use the ESP32's internal pull-ups; no resistors.
- On some DevKitC boards the on-board RGB LED is also on GPIO 38, so it may
  show a stray colour when button light 1 changes. That is harmless.
- **Handset cord:** the mic needs 5 wires (3V3, GND, SCK, WS, SD) and the
  earpiece 2, so 7 in total. The handset's own cord likely has too few. Use an
  8-conductor cable (a flat Ethernet patch cable works) and keep it to about
  12 inches.
- **Mic clock wires:** put a 100 Ω resistor in series with the mic's `SCK` and
  `WS` wires, at the breadboard end. Simulation shows the clock overshooting the
  mic's voltage limits without them (see [`sim/`](sim/README.md)).
- For the bench test the mic and 28 mm speaker can sit on the breadboard; they
  do not need to be in the handset yet.
- **Volume:** see [Safety](#safety). Listen yourself before he does.

## Safety

[`sim/SAFETY.md`](sim/SAFETY.md) compares every part's worst-case load with its rating
and models the common wiring mistakes. Nothing in normal use runs near a limit or gets
warm. Four things came out of it:

1. **Earpiece volume: fit a 220 Ω resistor in series with the earpiece speaker.** It caps
   the level in hardware, so no software mistake can make it loud. Until you fit one, the
   firmware holds the earpiece very quiet and refuses to build with a higher setting.
   After fitting it, set `PT_EARPIECE_SERIES_OHMS 220` and raise `PT_EARPIECE_GAIN`.
2. **Ringer volume** now starts at roughly 75 dB at half a metre, and the firmware refuses
   a setting above 128.
3. **Use 4.7 kΩ base resistors instead of 1 kΩ.** The lights are just as bright, and if a
   light wire ever shorts to 5 V the transistor survives instead of overheating.
4. **Measure, do not trust the model.** Check both speakers with a phone sound-meter app
   before he uses it. Aim for 75 dB or less at the earpiece.

Also: only ever power it from a certified 5 V USB charger, and unplug USB before moving
any wire. A short across the power rails can pull more current than the jumper wires are
rated for.

## Virtual testing

Before the parts arrive, `sh tools/virtual_test.sh` checks the pin map against the
board's rules, runs the firmware's logic on your computer, and simulates the light
driver and the handset cable. Details and results are in [`sim/`](sim/README.md).

## Milestone 1: hardware check

### Build and flash

Plug into the DevKit's **UART** USB port.

**PlatformIO (VS Code):** open the `firmware/` folder, then use *Upload and
Monitor* from the PlatformIO toolbar. The first build downloads ESP-IDF and
takes several minutes.

**Or plain [ESP-IDF v5.3](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32s3/get-started/index.html):**

```sh
cd firmware
idf.py build
idf.py -p <PORT> flash monitor      # Ctrl-] exits the monitor
```

### Test

| Do this | Expect |
|---|---|
| Power on | LEDs light one at a time in order 1→4. Log: `ready. handset is down (on-hook)` |
| Press button 2 (handset down) | LED 2 flashes, ringer rings (1.6 s on, 2.4 s off). Log: `button 2 pressed` |
| Press any button | Ringing stops, LED off |
| Press a button, then lift the handset | Ringing stops, that LED goes solid, you hear yourself in the earpiece |
| Lift the handset with nothing ringing | All four LEDs solid, you hear yourself |
| Speak while lifted | Log `mic peak` jumps from a few hundred into the thousands |
| Hang up | LEDs off, earpiece silent. Log: `idle` |

Repeat the press test for each button to confirm button N lights LED N.

### If something is off

| Symptom | Fix |
|---|---|
| Log says `LIFTED` while the handset is down | set `PT_HOOK_OFFHOOK_LEVEL 0` in `config.h` (or use the switch's other contact) |
| LEDs on when they should be off | set `PT_LED_ACTIVE_HIGH 0`, or check the transistor's pin order |
| `mic peak` stays at 0 | mic `L/R` must go to GND; check `SD` → GPIO 15 and 3V3 power; reflow the mic's header pins |
| Mic too quiet / distorted | raise / lower `PT_MIC_SHIFT` |
| Earpiece squeals | lower `PT_EARPIECE_GAIN`; pad the mic so it is not facing the earpiece inside the handset |
| Ring comes out of the wrong speaker | swap the two `SD` wires (GPIO 17 / 18) |
| No sound from an amp | check the solder joints on its header pins first |

The ring tone is synthesized by `firmware/tools/make_ring.py` and embedded in
flash as `firmware/main/ring.pcm`.

## Neon setup

Arrives with milestone 2, together with the SQL migrations it refers to.

## Twilio setup

Arrives with milestone 3. Nothing in milestones 1–2 needs a Twilio account.

## Privacy

- The firmware never writes audio to flash or sends it anywhere except the live call.
- Twilio recording is off unless explicitly requested per call; this project
  never sets `record` on any TwiML verb or API call, and the setup steps will
  include checking that account-level recording features are disabled.
- Only allowlisted numbers can call the toy or be called by it.
