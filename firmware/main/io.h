// Buttons, hook switch and button LEDs.
#pragma once
#include <stdbool.h>

typedef enum { LED_OFF, LED_SOLID, LED_FLASH } led_mode_t;

void io_init(void);
// Call every 10 ms: debounces inputs and animates flashing LEDs.
void io_tick(void);

bool io_button_down(int i);
// True once per press (cleared by reading).
bool io_button_pressed(int i);
bool io_offhook(void);

void io_led_set(int i, led_mode_t mode);
void io_led_all(led_mode_t mode);
