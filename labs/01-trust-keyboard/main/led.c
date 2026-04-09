/*
 * led.c — APA102 single-LED driver, GPIO bit-bang.
 *
 * We bit-bang because the APA102 data/clock pins (GPIO 40/39) would
 * conflict with the LCD's SPI bus, and we only update the LED on
 * state transitions so speed is irrelevant.
 *
 * APA102 wire format:
 *   Start frame:  32 bits of 0x00
 *   LED frame:    0xE0 | brightness(5 bits), Blue, Green, Red
 *   End frame:    32 bits of 0xFF
 */

#include "led.h"
#include "board.h"
#include "rom/ets_sys.h"      /* ets_delay_us */

/* ~100 kHz clock — plenty fast for a single LED. */
#define BIT_DELAY_US  1

static void send_byte(uint8_t b)
{
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(BOARD_LED_DATA, (b >> i) & 1);
        ets_delay_us(BIT_DELAY_US);
        gpio_set_level(BOARD_LED_CLK, 1);
        ets_delay_us(BIT_DELAY_US);
        gpio_set_level(BOARD_LED_CLK, 0);
    }
}

static void send_frame(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    send_byte(a);
    send_byte(b);
    send_byte(c);
    send_byte(d);
}

void led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOARD_LED_DATA) | (1ULL << BOARD_LED_CLK),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(BOARD_LED_DATA, 0);
    gpio_set_level(BOARD_LED_CLK, 0);

    led_off();
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (brightness > 31) brightness = 31;

    /* Start frame */
    send_frame(0x00, 0x00, 0x00, 0x00);
    /* LED frame: APA102 expects BGR order */
    send_frame(0xE0 | brightness, b, g, r);
    /* End frame */
    send_frame(0xFF, 0xFF, 0xFF, 0xFF);
}

void led_off(void)
{
    led_set_color(0, 0, 0, 0);
}
