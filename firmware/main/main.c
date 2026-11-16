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
#include "logic.h"

static const char *TAG = "phonetoy";

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

    pt_logic_t logic;
    pt_logic_init(&logic);
    bool offhook = io_offhook();
    ESP_LOGI(TAG, "ready. handset is %s", offhook ? "LIFTED (off-hook)" : "down (on-hook)");

    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(PT_TICK_MS));
        io_tick();

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

        if (!pt_logic_step(&logic, offhook, pressed)) {
            if (logic.state == PT_OFFHOOK && logic.ticks_in_state % (1000 / PT_TICK_MS) == 0) {
                // Speak into the mic: this should jump from a few hundred into the thousands.
                ESP_LOGI(TAG, "mic peak %5d / 32767", audio_take_mic_peak());
            }
            continue;
        }

        io_led_all(LED_OFF);
        switch (logic.state) {
        case PT_IDLE:
            audio_set_mode(AUDIO_IDLE);
            ESP_LOGI(TAG, "idle");
            break;
        case PT_RINGING:
            io_led_set(logic.slot, LED_FLASH);
            audio_set_mode(AUDIO_RING);
            ESP_LOGI(TAG, "ringing on button %d (lift handset to answer, any button to stop)", logic.slot + 1);
            break;
        case PT_OFFHOOK:
            if (logic.slot >= 0) io_led_set(logic.slot, LED_SOLID);
            else io_led_all(LED_SOLID);
            audio_set_mode(AUDIO_LOOPBACK);
            ESP_LOGI(TAG, "loopback: mic -> earpiece");
            break;
        }
    }
}
