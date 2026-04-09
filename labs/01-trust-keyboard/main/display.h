#pragma once

/*
 * display.h — ST7735 LCD driver for the T-Dongle-S3.
 *
 * All functions are no-ops when CONFIG_ENABLE_LCD is not set,
 * so callers do not need #if guards.
 */

#include "sdkconfig.h"
#include <stdint.h>

/* RGB565 colour helpers  (5-6-5 bit packing, big-endian on wire) */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

#if CONFIG_ENABLE_LCD

void display_init(void);
void display_clear(uint16_t color);

/*
 * Draw a null-terminated string at pixel position (x, y) using the
 * built-in 8×8 font.  Text wraps at the right edge of the screen.
 */
void display_draw_text(int x, int y, const char *text,
                       uint16_t fg, uint16_t bg);

/*
 * Convenience: clear screen and show a two-line status message
 * (large status on top, smaller detail below).
 */
void display_show_status(const char *line1, const char *line2);

#else /* LCD disabled — provide inline stubs */

static inline void display_init(void) {}
static inline void display_clear(uint16_t c) { (void)c; }
static inline void display_draw_text(int x, int y, const char *t,
                                     uint16_t fg, uint16_t bg)
{ (void)x; (void)y; (void)t; (void)fg; (void)bg; }
static inline void display_show_status(const char *a, const char *b)
{ (void)a; (void)b; }

#endif
