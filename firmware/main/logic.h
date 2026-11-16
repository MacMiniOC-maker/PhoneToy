// Pure logic with no hardware calls, so it can be compiled and tested on a PC
// (see firmware/test). The ESP32 code feeds it inputs and acts on its state.
#pragma once
#include <stdbool.h>

#define PT_TICK_MS 10
#define PT_DEBOUNCE_TICKS 4                       // 40 ms of agreement
#define PT_RING_TIMEOUT_TICKS (30000 / PT_TICK_MS)

// --- input debounce ---
typedef struct { bool state; int count; } pt_debounce_t;
void pt_debounce_init(pt_debounce_t *d, bool level);
// Feed one raw sample per tick. Returns true when the debounced level changed.
bool pt_debounce_step(pt_debounce_t *d, bool raw);

// --- hardware-check state machine ---
typedef enum { PT_IDLE, PT_RINGING, PT_OFFHOOK } pt_state_t;
typedef struct {
    pt_state_t state;
    int slot;            // button that started the ring, -1 if none
    int ticks_in_state;
} pt_logic_t;

void pt_logic_init(pt_logic_t *l);
// Advance one tick. `pressed` is the button pressed this tick, or -1.
// Returns true when the state changed.
bool pt_logic_step(pt_logic_t *l, bool offhook, int pressed);
