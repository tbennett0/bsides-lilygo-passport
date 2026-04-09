#pragma once

/*
 * hid_keyboard.h — USB HID keyboard via TinyUSB.
 *
 * Provides high-level helpers:  type a string, send a hotkey combo,
 * press Enter.  All functions block until the report is sent.
 */

#include <stdbool.h>
#include <stdint.h>

/* Initialise TinyUSB and register the HID keyboard device. */
void hid_keyboard_init(void);

/* Returns true once the host has mounted the device. */
bool hid_keyboard_mounted(void);

/*
 * Type an ASCII string one character at a time.
 * Blocks for (KEY_PRESS_MS + KEY_RELEASE_MS) * strlen(str).
 * Assumes US-QWERTY layout on the host.
 */
void hid_keyboard_type_string(const char *str);

/*
 * Send a single key with modifiers, then release.
 * modifier: bitmask of KEYBOARD_MODIFIER_* from hid.h
 * keycode:  HID usage code (HID_KEY_*)
 */
void hid_keyboard_send_key(uint8_t modifier, uint8_t keycode);

/* Press and release the Enter key. */
void hid_keyboard_press_enter(void);
