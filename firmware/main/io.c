#include "io.h"
#include "config.h"
#include "logic.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define N PT_NUM_BUTTONS
#define LED_DUTY_MAX 255
#define FLASH_HALF_PERIOD_TICKS 20  // 200 ms on / 200 ms off

static const int s_btn_gpio[N] = PT_BUTTON_GPIOS;
static const int s_led_gpio[N] = PT_LED_GPIOS;

typedef struct { int gpio; pt_debounce_t db; } debounced_t;

static debounced_t s_btn[N], s_hook;
static bool s_pressed[N];
static led_mode_t s_led_mode[N];
static unsigned s_ticks;

static void input_init(debounced_t *d, int gpio)
{
    gpio_config_t c = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&c);
    d->gpio = gpio;
    pt_debounce_init(&d->db, gpio_get_level(gpio));
}

// Returns true when the debounced level changed.
static bool input_tick(debounced_t *d)
{
    return pt_debounce_step(&d->db, gpio_get_level(d->gpio));
}

static void led_write(int i, bool on)
{
    uint32_t duty = (on == (bool)PT_LED_ACTIVE_HIGH) ? LED_DUTY_MAX + 1 : 0;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, i, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, i);
}

void io_init(void)
{
    for (int i = 0; i < N; i++) input_init(&s_btn[i], s_btn_gpio[i]);
    input_init(&s_hook, PT_HOOK_GPIO);

    // LEDC (PWM) rather than plain GPIO so later milestones can dim/breathe.
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
    for (int i = 0; i < N; i++) {
        ledc_channel_config_t ch = {
            .gpio_num = s_led_gpio[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = i,
            .timer_sel = LEDC_TIMER_0,
            .duty = PT_LED_ACTIVE_HIGH ? 0 : LED_DUTY_MAX + 1,
        };
        ledc_channel_config(&ch);
    }
}

void io_tick(void)
{
    s_ticks++;
    for (int i = 0; i < N; i++) {
        if (input_tick(&s_btn[i]) && !s_btn[i].db.state) s_pressed[i] = true;
    }
    input_tick(&s_hook);

    if (s_ticks % FLASH_HALF_PERIOD_TICKS == 0) {
        bool on = (s_ticks / FLASH_HALF_PERIOD_TICKS) & 1;
        for (int i = 0; i < N; i++) {
            if (s_led_mode[i] == LED_FLASH) led_write(i, on);
        }
    }
}

bool io_button_down(int i) { return !s_btn[i].db.state; }

bool io_button_pressed(int i)
{
    bool p = s_pressed[i];
    s_pressed[i] = false;
    return p;
}

bool io_offhook(void) { return s_hook.db.state == (bool)PT_HOOK_OFFHOOK_LEVEL; }

void io_led_set(int i, led_mode_t mode)
{
    s_led_mode[i] = mode;
    led_write(i, mode != LED_OFF);
}

void io_led_all(led_mode_t mode)
{
    for (int i = 0; i < N; i++) io_led_set(i, mode);
}
