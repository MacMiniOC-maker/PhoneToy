// USB handset probe. Answers one question without opening the handset:
// can the ESP32-S3 use the USB-C handset as its microphone and earpiece?
//
// Flash this, keep the computer on the board's UART port for power and the
// log, then plug the handset into the board's other USB-C port (marked USB).
// Nothing is played and nothing is recorded: it only reads the handset's
// description of itself.
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "usb/usb_host.h"

static const char *TAG = "probe";
static volatile uint8_t s_new_addr;   // address of a newly attached device, 0 if none
static volatile bool s_gone;

static void client_cb(const usb_host_client_event_msg_t *msg, void *arg)
{
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) s_new_addr = msg->new_dev.address;
    else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) s_gone = true;
}

static void host_task(void *arg)
{
    for (;;) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
    }
}

typedef struct {
    bool present;
    int channels, bits;
    int rates[8], n_rates;
    bool continuous;
    int ep_addr, ep_size;
} stream_t;

static bool has_rate(const stream_t *s, int hz)
{
    if (s->continuous) return s->n_rates >= 2 && hz >= s->rates[0] && hz <= s->rates[1];
    for (int i = 0; i < s->n_rates; i++) if (s->rates[i] == hz) return true;
    return false;
}

static void print_stream(const char *name, const stream_t *s)
{
    if (!s->present) { printf("  %-10s NOT FOUND\n", name); return; }
    printf("  %-10s %d channel(s), %d-bit, rates:", name, s->channels, s->bits);
    for (int i = 0; i < s->n_rates; i++) printf(" %d", s->rates[i]);
    printf("%s Hz, endpoint 0x%02x, %d bytes per packet\n", s->continuous ? " (range)" : "", s->ep_addr, s->ep_size);
}

static void describe(usb_device_handle_t dev)
{
    const usb_device_desc_t *dd;
    const usb_config_desc_t *cfg;
    usb_device_info_t info;
    if (usb_host_get_device_descriptor(dev, &dd) != ESP_OK ||
        usb_host_get_active_config_descriptor(dev, &cfg) != ESP_OK ||
        usb_host_device_info(dev, &info) != ESP_OK) {
        printf("RESULT: the device connected but would not describe itself. Unplug, replug, try again.\n");
        return;
    }

    stream_t spk = {0}, mic = {0}, cur = {0};
    int uac_bcd = 0, n_audio = 0, n_hid = 0, in_audio_stream = 0;
    const uint8_t *p = (const uint8_t *)cfg;
    const uint8_t *end = p + cfg->wTotalLength;
    while (p + 2 <= end && p[0] >= 2 && p + p[0] <= end) {
        uint8_t len = p[0], type = p[1];
        if (type == 0x04 && len >= 9) {                 // interface
            in_audio_stream = (p[5] == 0x01 && p[6] == 0x02);
            if (p[5] == 0x01) n_audio++;
            if (p[5] == 0x03) n_hid++;
            if (in_audio_stream && p[4] > 0) memset(&cur, 0, sizeof cur);
        } else if (type == 0x24 && len >= 5 && p[2] == 0x01 && !in_audio_stream && uac_bcd == 0) {
            uac_bcd = p[3] | (p[4] << 8);               // AudioControl header: class version
        } else if (type == 0x24 && in_audio_stream && p[2] == 0x02 && len >= 8) {   // format type
            cur.channels = p[4];
            cur.bits = p[6];
            int n = p[7];
            cur.continuous = (n == 0);
            if (n == 0) n = 2;
            cur.n_rates = 0;
            for (int i = 0; i < n && i < 8 && 8 + i * 3 + 2 < len; i++)
                cur.rates[cur.n_rates++] = p[8 + i * 3] | (p[9 + i * 3] << 8) | (p[10 + i * 3] << 16);
        } else if (type == 0x05 && in_audio_stream && len >= 7 && (p[3] & 0x03) == 0x01) {   // isochronous endpoint
            cur.present = true;
            cur.ep_addr = p[2];
            cur.ep_size = (p[4] | (p[5] << 8)) & 0x7ff;
            stream_t *dst = (p[2] & 0x80) ? &mic : &spk;
            if (!dst->present) *dst = cur;
        }
        p += len;
    }

    printf("\n================ USB HANDSET PROBE ================\n");
    printf("Device %04x:%04x, %s speed, asks for up to %d mA\n", dd->idVendor, dd->idProduct,
           info.speed == USB_SPEED_HIGH ? "high" : info.speed == USB_SPEED_FULL ? "full" : "low", cfg->bMaxPower * 2);
    printf("Audio interfaces: %d, button (HID) interfaces: %d, USB audio class version %x.%02x\n",
           n_audio, n_hid, uac_bcd >> 8, uac_bcd & 0xff);
    print_stream("Earpiece:", &spk);
    print_stream("Microphone:", &mic);

    bool uac1 = (uac_bcd >> 8) == 1;
    printf("\nRESULT: ");
    if (!n_audio) printf("FAIL. This is not a USB audio device.\n");
    else if (!spk.present || !mic.present) printf("FAIL. It needs both an earpiece and a microphone stream; one is missing.\n");
    else if (!uac1) printf("FAIL for now. It uses USB audio class %d; the ESP32 driver handles class 1 only.\n", uac_bcd >> 8);
    else if (info.speed == USB_SPEED_HIGH) printf("FAIL. It is a high-speed device; this ESP32 port is full-speed only.\n");
    else {
        printf("PASS. The handset can be used whole, as a USB microphone and earpiece.\n");
        printf("        Sample rate to use: %s.\n",
               has_rate(&spk, 16000) && has_rate(&mic, 16000) ? "16 kHz on both (ideal for calls)" :
               has_rate(&spk, 48000) && has_rate(&mic, 48000) ? "48 kHz on both (the firmware will convert)" :
               "earpiece and microphone differ (the firmware will convert)");
        if (n_hid) printf("        Note: its volume buttons report to the ESP32, so the firmware can ignore or limit them.\n");
    }
    printf("Send this whole block to Claude Code before returning any parts.\n");
    printf("===================================================\n\n");
}

void app_main(void)
{
    const usb_host_config_t host_cfg = { .intr_flags = ESP_INTR_FLAG_LEVEL1 };
    ESP_ERROR_CHECK(usb_host_install(&host_cfg));
    xTaskCreate(host_task, "usb_host", 4096, NULL, 5, NULL);

    usb_host_client_handle_t client;
    const usb_host_client_config_t client_cfg = {
        .max_num_event_msg = 5,
        .async = { .client_event_callback = client_cb },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_cfg, &client));

    ESP_LOGI(TAG, "Plug the handset into the board's USB port (the one that is NOT UART).");
    usb_device_handle_t dev = NULL;
    int waited_ms = 0;
    for (;;) {
        usb_host_client_handle_events(client, pdMS_TO_TICKS(500));
        waited_ms += 500;
        if (s_new_addr && !dev) {
            if (usb_host_device_open(client, s_new_addr, &dev) == ESP_OK) describe(dev);
            s_new_addr = 0;
        }
        if (s_gone && dev) {
            usb_host_device_close(client, dev);
            dev = NULL;
            s_gone = false;
            waited_ms = 0;
            ESP_LOGI(TAG, "Handset unplugged. Plug it in again to repeat.");
        }
        if (!dev && waited_ms == 15000) {
            printf("\nRESULT so far: nothing detected after 15 s.\n"
                   "The most likely cause is that this port does not supply 5 V to the handset.\n"
                   "Check for a solder pad labelled USB-OTG on the back of the board, and tell Claude Code what you see.\n\n");
        }
    }
}
