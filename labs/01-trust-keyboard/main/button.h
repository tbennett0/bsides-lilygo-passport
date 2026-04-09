#pragma once

/*
 * button.h — Debounced BOOT-button input (GPIO0, active-low).
 */

#include <stdbool.h>

/* Configure GPIO0 as input with pull-up. */
void button_init(void);

/*
 * Poll the button state.  Returns true exactly once per press
 * (falling-edge detect with 50 ms debounce).
 * Call from your main loop at ≤ 20 ms intervals.
 */
bool button_pressed(void);
