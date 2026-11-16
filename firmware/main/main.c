// Milestone 1: hardware check. No Wi-Fi, no network, nothing stored.
//
//   boot                 -> each button LED lights in turn (1,2,3,4)
//   press a button       -> that LED flashes and the ringer rings (like an incoming call)
//   press any button     -> stops the ringing
//   lift the handset     -> ringing stops, LED goes solid, mic is looped to the earpiece
//   lift with no ringing -> all four LEDs solid + loopback
//   hang up              -> back to idle
//
// Everything is also logged on the serial console (115200 baud).
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio.h"
#include "config.h"
#include "io.h"

#define TICK_MS 10
#define RING_TIMEOUT_TICKS (30000 / TICK_MS)

static const char *TAG = "phonetoy";

typedef enum { ST_IDLE, ST_RINGING, ST_OFFHOOK } state_t;

void app_main(void)
{
    io_init();
    audio_init();

    ESP_LOGI(TAG, "Milestone 1 hardware check");
    for (int i = 0; i < PT_NUM_BUTTONS; i++) {
        io_led_set(i, LED_SOLID);
        vTaskDelay(pdMS_TO_TICKS(400));
        io_led_set(i, LED_OFF);
    }
    for (int i = 0; i < PT_NUM_BUTTONS; i++) io_button_pressed(i);  // drop boot-time presses

    state_t state = ST_IDLE;
    int slot = -1;  // button that started the ring, -1 if none
    int ticks_in_state = 0;
    bool offhook = io_offhook();
    ESP_LOGI(TAG, "ready. handset is %s", offhook ? "LIFTED (off-hook)" : "down (on-hook)");

    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(TICK_MS));
        io_tick();
        ticks_in_state++;

        int pressed = -1;
        for (int i = 0; i < PT_NUM_BUTTONS; i++) {
            if (io_button_pressed(i)) {
                ESP_LOGI(TAG, "button %d pressed", i + 1);
                pressed = i;
            }
        }
        if (io_offhook() != offhook) {
            offhook = !offhook;
            ESP_LOGI(TAG, "handset %s", offhook ? "LIFTED (off-hook)" : "down (on-hook)");
        }

        state_t next = state;
        switch (state) {
        case ST_IDLE:
            if (offhook) next = ST_OFFHOOK;
            else if (pressed >= 0) { slot = pressed; next = ST_RINGING; }
            break;
        case ST_RINGING:
            if (offhook) next = ST_OFFHOOK;
            else if (pressed >= 0 || ticks_in_state > RING_TIMEOUT_TICKS) next = ST_IDLE;
            break;
        case ST_OFFHOOK:
            if (!offhook) next = ST_IDLE;
            else if (ticks_in_state % (1000 / TICK_MS) == 0) {
                // Speak into the mic: this should jump from a few hundred into the thousands.
                ESP_LOGI(TAG, "mic peak %5d / 32767", audio_take_mic_peak());
            }
            break;
        }
        if (next == state) continue;

        io_led_all(LED_OFF);
        switch (next) {
        case ST_IDLE:
            slot = -1;
            audio_set_mode(AUDIO_IDLE);
            ESP_LOGI(TAG, "idle");
            break;
        case ST_RINGING:
            io_led_set(slot, LED_FLASH);
            audio_set_mode(AUDIO_RING);
            ESP_LOGI(TAG, "ringing on button %d (lift handset to answer, any button to stop)", slot + 1);
            break;
        case ST_OFFHOOK:
            if (slot >= 0) io_led_set(slot, LED_SOLID);
            else io_led_all(LED_SOLID);
            audio_set_mode(AUDIO_LOOPBACK);
            ESP_LOGI(TAG, "loopback: mic -> earpiece");
            break;
        }
        state = next;
        ticks_in_state = 0;
    }
}
