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

| Part | Notes |
|---|---|
| ESP32-S3-DevKitC-1 **N16R8** | 16 MB flash, 8 MB octal PSRAM |
| Retro handset | earpiece speaker reused; the INMP441 goes in the mouthpiece |
| Hook switch | any microswitch the resting handset presses |
| MAX98357A I2S amp × **2** | one for the earpiece, one for the ringer speaker (see note below) |
| Small speaker, 4–8 Ω | ringer |
| INMP441 I2S mic | |
| LED arcade button × 4 | **5 V LED version** recommended |
| NPN transistor × 4 (2N2222, BC337, …) + 1 kΩ resistor × 4 | LED drivers |

**Two amps or one?** A MAX98357A drives one speaker. This design uses two amps
on the same I2S wires and switches between them with their `SD` pins. If you
only have one, wire it to the earpiece, set `PT_SINGLE_AMP 1` in
`firmware/main/config.h`, and the ring tone plays (quietly) from the handset.

## Check the button LEDs first

Arcade button LEDs come as 5 V or 12 V parts with the resistor built in, and
either is more than a 3.3 V GPIO pin can drive, so every LED goes through a
transistor.

1. Look for a marking on the LED holder (`5V` / `12V`), or check the listing.
2. Not sure? Touch the LED terminals to the DevKit's `5V` and `GND` pins
   (try both polarities). Bright = 5 V part. Dim = 12 V part.
3. A 12 V LED is too dim on USB power. Swap the LED holder for a 5 V one
   (they pull out of the button), or replace its resistor with ~150 Ω.

## Wiring

All pins are defined in [`firmware/main/config.h`](firmware/main/config.h).

| Signal | ESP32-S3 pin | Connects to |
|---|---|---|
| I2S bit clock | GPIO 4 | `BCLK` on both amps, `SCK` on mic |
| I2S word select | GPIO 5 | `LRC` on both amps, `WS` on mic |
| I2S data out | GPIO 6 | `DIN` on both amps |
| I2S data in | GPIO 7 | `SD` on mic |
| Earpiece amp enable | GPIO 15 | `SD` on the earpiece MAX98357A |
| Ringer amp enable | GPIO 16 | `SD` on the ringer MAX98357A |
| Hook switch | GPIO 8 | switch → `GND` |
| Button 1 / 2 / 3 / 4 | GPIO 9 / 10 / 11 / 12 | button switch → `GND` |
| LED 1 / 2 / 3 / 4 | GPIO 13 / 14 / 17 / 18 | 1 kΩ → transistor base |
| 5 V | `5V` pin | `VIN` on both amps, `+` of each button LED |
| 3.3 V | `3V3` pin | `VDD` on mic |
| Ground | `GND` | amps, mic (`GND` **and** `L/R`), transistor emitters, all switches |

Per button LED (low-side switch):

```
5V ──► LED + ... LED − ──► collector
GPIO ──[1 kΩ]──► base        (NPN: 2N2222 / BC337)
GND ◄── emitter
```

Other notes:

- Amp outputs (`+`/`−`) go straight to the earpiece / ringer speaker. Never
  connect either output wire to ground.
- Leave the amps' `GAIN` pins unconnected.
- Buttons and the hook switch use the ESP32's internal pull-ups; no resistors.
- **Handset cord:** the mic needs 5 wires (3V3, GND, SCK, WS, SD) and the
  earpiece 2, so 7 in total. A classic coiled handset cord has only 4. Use an
  8-conductor cable (a flat Ethernet patch cable works) and keep it under ~1 m;
  I2S clocks do not like long cables. If the mic is noisy or silent only when
  it is on the long cable, add a 100 Ω resistor in series with GPIO 4 at the
  ESP32 end.
- **Volume:** `PT_EARPIECE_GAIN` starts low on purpose. Listen yourself before
  raising it; it sits against a child's ear.

## Milestone 1: hardware check

### Build and flash

Needs [ESP-IDF v5.3](https://docs.espressif.com/projects/esp-idf/en/v5.3.2/esp32s3/get-started/index.html).
Plug into the DevKit's **UART** USB port.

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
| `mic peak` stays at 0 | mic `L/R` must go to GND; check `SD` → GPIO 7 and 3V3 power |
| Mic too quiet / distorted | raise / lower `PT_MIC_SHIFT` |
| Earpiece squeals | lower `PT_EARPIECE_GAIN`; pad the mic so it is not facing the earpiece inside the handset |
| Ring comes out of the wrong speaker | swap the two `SD` wires (GPIO 15 / 16) |

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
