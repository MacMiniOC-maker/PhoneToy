#include "logic.h"

void pt_debounce_init(pt_debounce_t *d, bool level)
{
    d->state = level;
    d->count = 0;
}

bool pt_debounce_step(pt_debounce_t *d, bool raw)
{
    if (raw == d->state) { d->count = 0; return false; }
    if (++d->count < PT_DEBOUNCE_TICKS) return false;
    d->state = raw;
    d->count = 0;
    return true;
}

void pt_logic_init(pt_logic_t *l)
{
    l->state = PT_IDLE;
    l->slot = -1;
    l->ticks_in_state = 0;
}

bool pt_logic_step(pt_logic_t *l, bool offhook, int pressed)
{
    pt_state_t next = l->state;
    l->ticks_in_state++;

    switch (l->state) {
    case PT_IDLE:
        if (offhook) next = PT_OFFHOOK;
        else if (pressed >= 0) { l->slot = pressed; next = PT_RINGING; }
        break;
    case PT_RINGING:
        if (offhook) next = PT_OFFHOOK;
        else if (pressed >= 0 || l->ticks_in_state > PT_RING_TIMEOUT_TICKS) next = PT_IDLE;
        break;
    case PT_OFFHOOK:
        if (!offhook) next = PT_IDLE;
        break;
    }
    if (next == l->state) return false;

    if (next == PT_IDLE) l->slot = -1;
    l->state = next;
    l->ticks_in_state = 0;
    return true;
}
