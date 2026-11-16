// PC-side tests for the firmware's logic. No ESP32 needed:
//   gcc -Wall -Ifirmware/main firmware/main/logic.c firmware/test/test_logic.c -o /tmp/test_logic && /tmp/test_logic
#include <stdio.h>
#include <stdlib.h>
#include "logic.h"

static int failures, checks;
#define CHECK(cond, what) do { checks++; if (!(cond)) { failures++; printf("  FAIL: %s (line %d)\n", what, __LINE__); } } while (0)

static void run_ticks(pt_logic_t *l, bool offhook, int n) { while (n--) pt_logic_step(l, offhook, -1); }

static void test_ring_answer_hangup(void)
{
    puts("press -> ring -> lift -> loopback -> hang up");
    pt_logic_t l; pt_logic_init(&l);
    CHECK(l.state == PT_IDLE && l.slot == -1, "starts idle");
    CHECK(pt_logic_step(&l, false, 1), "press changes state");
    CHECK(l.state == PT_RINGING && l.slot == 1, "button 2 rings on slot 1");
    run_ticks(&l, false, 200);
    CHECK(l.state == PT_RINGING, "keeps ringing for 2 s");
    pt_logic_step(&l, true, -1);
    CHECK(l.state == PT_OFFHOOK && l.slot == 1, "lifting answers and keeps the slot");
    run_ticks(&l, true, 500);
    CHECK(l.state == PT_OFFHOOK, "stays off-hook while lifted");
    pt_logic_step(&l, false, -1);
    CHECK(l.state == PT_IDLE && l.slot == -1, "hanging up returns to idle and clears the slot");
}

static void test_ring_stops(void)
{
    puts("ring stops on a second press, and after 30 s");
    pt_logic_t l; pt_logic_init(&l);
    pt_logic_step(&l, false, 0);
    pt_logic_step(&l, false, 3);
    CHECK(l.state == PT_IDLE, "any button stops the ring");
    pt_logic_step(&l, false, 2);
    run_ticks(&l, false, PT_RING_TIMEOUT_TICKS - 1);
    CHECK(l.state == PT_RINGING, "still ringing just before 30 s");
    run_ticks(&l, false, 3);
    CHECK(l.state == PT_IDLE && l.slot == -1, "gives up after 30 s");
}

static void test_lift_without_ring(void)
{
    puts("lift with nothing ringing; buttons ignored while lifted");
    pt_logic_t l; pt_logic_init(&l);
    pt_logic_step(&l, true, -1);
    CHECK(l.state == PT_OFFHOOK && l.slot == -1, "lift from idle has no slot (all lights on)");
    pt_logic_step(&l, true, 2);
    CHECK(l.state == PT_OFFHOOK && l.slot == -1, "a press while lifted does nothing");
    pt_logic_step(&l, true, 0);       // press and lift in the same tick from idle
    pt_logic_init(&l);
    pt_logic_step(&l, true, 0);
    CHECK(l.state == PT_OFFHOOK, "lift wins over a press in the same tick");
}

static void test_debounce(void)
{
    puts("switch bounce");
    pt_debounce_t d; pt_debounce_init(&d, true);   // idle HIGH (pull-up)
    int changes = 0;
    // a bouncy press: 12 ticks of chatter, then solid LOW
    bool chatter[] = {0,1,0,0,1,0,1,1,0,0,0,1};
    for (unsigned i = 0; i < sizeof chatter; i++) changes += pt_debounce_step(&d, chatter[i]);
    for (int i = 0; i < 20; i++) changes += pt_debounce_step(&d, false);
    CHECK(changes == 1 && d.state == false, "a bouncy press counts exactly once");
    changes = 0;
    for (int i = 0; i < 3; i++) changes += pt_debounce_step(&d, true);   // 30 ms glitch
    for (int i = 0; i < 20; i++) changes += pt_debounce_step(&d, false);
    CHECK(changes == 0, "a 30 ms glitch is ignored");
    for (int i = 0; i < 4; i++) changes += pt_debounce_step(&d, true);
    CHECK(changes == 1 && d.state == true, "a release registers after 40 ms");
}

static void test_random(void)
{
    puts("200,000 random ticks (a toddler mashing everything)");
    pt_logic_t l; pt_logic_init(&l);
    srand(1234);
    bool offhook = false, ok = true;
    for (int i = 0; i < 200000 && ok; i++) {
        if (rand() % 50 == 0) offhook = !offhook;
        int pressed = rand() % 8 == 0 ? rand() % 4 : -1;
        pt_logic_step(&l, offhook, pressed);
        if (l.state == PT_RINGING && offhook) ok = false;                   // never ring into a lifted handset
        if (l.state == PT_OFFHOOK && !offhook) ok = false;                  // never loop audio with the handset down
        if (l.state == PT_RINGING && (l.slot < 0 || l.slot > 3)) ok = false; // a ring always belongs to a button
        if (l.state == PT_IDLE && l.slot != -1) ok = false;
        if (l.state == PT_RINGING && l.ticks_in_state > PT_RING_TIMEOUT_TICKS) ok = false;
    }
    CHECK(ok, "no impossible state reached");
}

int main(void)
{
    test_ring_answer_hangup();
    test_ring_stops();
    test_lift_without_ring();
    test_debounce();
    test_random();
    printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
