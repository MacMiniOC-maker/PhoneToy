// Pin map and tunables. Matches the "Suggested wiring" table in the build plan
// and the wiring table in README.md; keep all three in sync.
//
// Avoided on the ESP32-S3-DevKitC-1 N16R8: GPIO 0/3/45/46 (strapping),
// 19/20 (native USB), 35/36/37 (octal PSRAM), 43/44 (UART console), 48 (RGB LED).
// GPIO 38 drives the on-board RGB LED on v1.1 boards instead; LED 1 shares it harmlessly.
#pragma once

#define PT_NUM_BUTTONS 4

// Buttons: switch between GPIO and GND, internal pull-up (pressed = LOW).
#define PT_BUTTON_GPIOS {9, 10, 11, 12}

// Button LEDs. Each GPIO drives a transistor (low-side switch), HIGH = lit.
#define PT_LED_GPIOS {38, 39, 40, 41}
#define PT_LED_ACTIVE_HIGH 1

// Hook switch: lever microswitch, common tab -> GND, normally-open tab -> GPIO
// (internal pull-up). Handset down presses it = LOW, lifted = HIGH.
// Level read when the handset is LIFTED. Flip to 0 if the log shows it backwards.
#define PT_HOOK_GPIO 16
#define PT_HOOK_OFFHOOK_LEVEL 1

// One I2S bus shared by the mic (RX) and both amps (TX).
#define PT_I2S_BCLK_GPIO 5
#define PT_I2S_WS_GPIO   6
#define PT_I2S_DOUT_GPIO 7   // -> DIN of both MAX98357A
#define PT_I2S_DIN_GPIO  15  // <- SD of INMP441

// MAX98357A SD pins: HIGH = amp on, LOW = shutdown.
#define PT_EARPIECE_AMP_SD_GPIO 17
#define PT_RINGER_AMP_SD_GPIO   18
// Set to 1 if you only have ONE amp (wired to the earpiece): the ring tone then
// plays through the earpiece and PT_RINGER_AMP_SD_GPIO is unused.
#define PT_SINGLE_AMP 0

#define PT_SAMPLE_RATE 16000

// Output levels, 0..256 (256 = full scale). The earpiece sits against a
// toddler's ear: start low and only raise it after listening yourself.
#define PT_EARPIECE_GAIN 24
#define PT_RINGER_GAIN   128
// Mic gain as a left shift of the 16-bit sample (INMP441 is quiet; 2 = x4).
#define PT_MIC_SHIFT 2

// Ring cadence: the embedded burst, then silence, repeating.
#define PT_RING_PERIOD_MS 4000
