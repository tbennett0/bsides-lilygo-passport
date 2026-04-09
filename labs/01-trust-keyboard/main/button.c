/*
 * button.c — Debounced BOOT-button input for the T-Dongle-S3.
 *
 * GPIO0 is active-low with an internal pull-up.  We poll rather than
 * use an ISR because the main loop already ticks at 10 ms and
 * polling is simpler to reason about.
 */

#include "button.h"
#include "board.h"
#include "esp_timer.h"

#define DEBOUNCE_US (50 * 1000)   /* 50 ms */

static int      s_last_stable;    /* 1 = released, 0 = pressed */
static int64_t  s_last_change_us;

void button_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BOARD_BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    s_last_stable    = gpio_get_level(BOARD_BUTTON_GPIO);
    s_last_change_us = esp_timer_get_time();
}

bool button_pressed(void)
{
    int raw = gpio_get_level(BOARD_BUTTON_GPIO);
    int64_t now = esp_timer_get_time();

    if (raw != s_last_stable) {
        if ((now - s_last_change_us) >= DEBOUNCE_US) {
            s_last_stable = raw;
            s_last_change_us = now;
            /* Return true on the falling edge (button down). */
            if (raw == 0) {
                return true;
            }
        }
    } else {
        s_last_change_us = now;
    }
    return false;
}
