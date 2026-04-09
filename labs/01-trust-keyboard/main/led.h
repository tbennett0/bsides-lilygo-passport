#pragma once

/*
 * led.h — APA102 (DotStar) single-LED driver via GPIO bit-bang.
 */

#include <stdint.h>

void led_init(void);

/*
 * Set the LED colour.  brightness: 0-31.
 * Colour order is standard RGB; the driver handles APA102's BGR wire format.
 */
void led_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

/* Convenience: turn the LED off. */
void led_off(void);
