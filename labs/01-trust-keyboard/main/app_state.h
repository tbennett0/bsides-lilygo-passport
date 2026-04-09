#pragma once

/*
 * app_state.h — Application state machine.
 *
 *   INIT ──(USB mounted)──► READY ──(button)──► TYPING ──► DONE
 *                             ▲                               │
 *                             └──────────(button)─────────────┘
 */

typedef enum {
    APP_STATE_INIT,     /* Subsystems booting, waiting for USB mount   */
    APP_STATE_READY,    /* USB mounted, idle, waiting for button press */
    APP_STATE_TYPING,   /* Actively sending HID key reports            */
    APP_STATE_DONE,     /* Payload delivered, awaiting reset           */
} app_state_t;

/* Return the human-readable label for a state (e.g. "READY"). */
const char *app_state_name(app_state_t state);
