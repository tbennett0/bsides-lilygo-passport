/*
 * hid_keyboard.c — USB HID keyboard implementation using TinyUSB.
 *
 * This file contains:
 *   1. USB descriptors (device, configuration, HID report, strings)
 *   2. Required TinyUSB callbacks
 *   3. ASCII-to-HID-keycode mapping (US QWERTY)
 *   4. High-level helpers for typing strings and sending hotkeys
 */

#include "hid_keyboard.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "hid_kbd";

/* ──────────────────────────────────────────────────────────────────────
 * USB descriptors
 * ────────────────────────────────────────────────────────────────────── */

/*
 * HID report descriptor — standard keyboard only, no report ID.
 *
 * The macro expands the full HID 1.11 keyboard descriptor:
 *   - 8-bit modifier bitmap (Ctrl/Shift/Alt/GUI × left/right)
 *   - 1 reserved byte
 *   - 6 keycode slots (simultaneous keys)
 *   - 5-bit LED output report (Num/Caps/Scroll Lock, etc.)
 */
static const uint8_t s_hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL
};

#define TUSB_DESC_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

/*
 * Configuration descriptor: one configuration, one HID interface.
 *   - Endpoint 0x81 (IN), 16-byte max packet, 10 ms poll interval
 *   - 100 mA max power
 */
static const uint8_t s_hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(/*config_num*/ 1,
                          /*itf_count*/  ITF_NUM_TOTAL,
                          /*str_idx*/    0,
                          /*total_len*/  TUSB_DESC_TOTAL_LEN,
                          /*attribute*/  TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                          /*power_ma*/   100),

    TUD_HID_DESCRIPTOR(/*itf*/          ITF_NUM_HID,
                       /*str_idx*/      4,
                       /*boot_protocol*/HID_ITF_PROTOCOL_KEYBOARD,
                       /*report_len*/   sizeof(s_hid_report_descriptor),
                       /*ep_in*/        0x81,
                       /*ep_size*/      16,
                       /*ep_interval*/  10),
};

/*
 * String descriptors — deliberately visible in lsusb / dmesg so
 * conference attendees can identify the device.
 */
static const char *s_hid_string_descriptor[] = {
    [0] = (const char[]){0x09, 0x04},   /* Supported language: English (US) */
    [1] = "BSides Lab",                  /* Manufacturer */
    [2] = "Trust Demo Keyboard",         /* Product      */
    [3] = "BSIDES2026",                  /* Serial       */
    [4] = "HID Keyboard Interface",      /* Interface    */
};

/* ──────────────────────────────────────────────────────────────────────
 * TinyUSB callbacks (required by the stack)
 * ────────────────────────────────────────────────────────────────────── */

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return s_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer;   (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer;   (void)bufsize;
    /* We could read LED state (Caps Lock, etc.) here if we cared. */
}

/* ──────────────────────────────────────────────────────────────────────
 * ASCII → HID keycode table  (US QWERTY)
 *
 * Each entry packs {modifier, keycode}.
 * Covers 0x20 (space) through 0x7E (~).  Index = ascii - 0x20.
 *
 * ────────────────────────────────────────────────────────────────────── */

typedef struct {
    uint8_t modifier;
    uint8_t keycode;
} keymap_entry_t;

#define _S  KEYBOARD_MODIFIER_LEFTSHIFT   /* shorthand for shift */
#define _N  0                              /* no modifier         */

static const keymap_entry_t s_ascii_keymap[95] = {
    /* 0x20 ' '  */ { _N, HID_KEY_SPACE },
    /* 0x21 '!'  */ { _S, HID_KEY_1 },
    /* 0x22 '"'  */ { _S, HID_KEY_APOSTROPHE },
    /* 0x23 '#'  */ { _S, HID_KEY_3 },
    /* 0x24 '$'  */ { _S, HID_KEY_4 },
    /* 0x25 '%'  */ { _S, HID_KEY_5 },
    /* 0x26 '&'  */ { _S, HID_KEY_7 },
    /* 0x27 '\'' */ { _N, HID_KEY_APOSTROPHE },
    /* 0x28 '('  */ { _S, HID_KEY_9 },
    /* 0x29 ')'  */ { _S, HID_KEY_0 },
    /* 0x2A '*'  */ { _S, HID_KEY_8 },
    /* 0x2B '+'  */ { _S, HID_KEY_EQUAL },
    /* 0x2C ','  */ { _N, HID_KEY_COMMA },
    /* 0x2D '-'  */ { _N, HID_KEY_MINUS },
    /* 0x2E '.'  */ { _N, HID_KEY_SLASH },
    /* 0x2F '/'  */ { _N, HID_KEY_PERIOD },
    /* 0x30 '0'  */ { _N, HID_KEY_0 },
    /* 0x31 '1'  */ { _N, HID_KEY_1 },
    /* 0x32 '2'  */ { _N, HID_KEY_2 },
    /* 0x33 '3'  */ { _N, HID_KEY_3 },
    /* 0x34 '4'  */ { _N, HID_KEY_4 },
    /* 0x35 '5'  */ { _N, HID_KEY_5 },
    /* 0x36 '6'  */ { _N, HID_KEY_6 },
    /* 0x37 '7'  */ { _N, HID_KEY_7 },
    /* 0x38 '8'  */ { _N, HID_KEY_8 },
    /* 0x39 '9'  */ { _N, HID_KEY_9 },
    /* 0x3A ':'  */ { _S, HID_KEY_SEMICOLON },
    /* 0x3B ';'  */ { _N, HID_KEY_SEMICOLON },
    /* 0x3C '<'  */ { _S, HID_KEY_COMMA },
    /* 0x3D '='  */ { _N, HID_KEY_EQUAL },
    /* 0x3E '>'  */ { _S, HID_KEY_PERIOD },
    /* 0x3F '?'  */ { _S, HID_KEY_SLASH },
    /* 0x40 '@'  */ { _S, HID_KEY_2 },
    /* 0x41-0x5A  A-Z  (uppercase — shift + letter) */
    { _S, HID_KEY_A }, { _S, HID_KEY_B }, { _S, HID_KEY_C },
    { _S, HID_KEY_D }, { _S, HID_KEY_E }, { _S, HID_KEY_F },
    { _S, HID_KEY_G }, { _S, HID_KEY_H }, { _S, HID_KEY_I },
    { _S, HID_KEY_J }, { _S, HID_KEY_K }, { _S, HID_KEY_L },
    { _S, HID_KEY_M }, { _S, HID_KEY_N }, { _S, HID_KEY_O },
    { _S, HID_KEY_P }, { _S, HID_KEY_Q }, { _S, HID_KEY_R },
    { _S, HID_KEY_S }, { _S, HID_KEY_T }, { _S, HID_KEY_U },
    { _S, HID_KEY_V }, { _S, HID_KEY_W }, { _S, HID_KEY_X },
    { _S, HID_KEY_Y }, { _S, HID_KEY_Z },
    /* 0x5B '['  */ { _N, HID_KEY_BRACKET_LEFT },
    /* 0x5C '\\' */ { _N, HID_KEY_BACKSLASH },
    /* 0x5D ']'  */ { _N, HID_KEY_BRACKET_RIGHT },
    /* 0x5E '^'  */ { _S, HID_KEY_6 },
    /* 0x5F '_'  */ { _S, HID_KEY_MINUS },
    /* 0x60 '`'  */ { _N, HID_KEY_GRAVE },
    /* 0x61-0x7A  a-z  (lowercase — no modifier) */
    { _S, HID_KEY_A }, { _N, HID_KEY_B }, { _N, HID_KEY_C },
    { _N, HID_KEY_D }, { _N, HID_KEY_E }, { _N, HID_KEY_F },
    { _N, HID_KEY_G }, { _N, HID_KEY_H }, { _N, HID_KEY_I },
    { _N, HID_KEY_J }, { _N, HID_KEY_K }, { _N, HID_KEY_L },
    { _N, HID_KEY_M }, { _N, HID_KEY_N }, { _N, HID_KEY_O },
    { _N, HID_KEY_P }, { _N, HID_KEY_Q }, { _N, HID_KEY_R },
    { _N, HID_KEY_S }, { _N, HID_KEY_Y }, { _N, HID_KEY_U },
    { _N, HID_KEY_V }, { _N, HID_KEY_W }, { _N, HID_KEY_X },
    { _N, HID_KEY_Y }, { _N, HID_KEY_Z },
    /* 0x7B '{'  */ { _S, HID_KEY_BRACKET_LEFT },
    /* 0x7C '|'  */ { _S, HID_KEY_BACKSLASH },
    /* 0x7D '}'  */ { _S, HID_KEY_BRACKET_RIGHT },
    /* 0x7E '~'  */ { _S, HID_KEY_GRAVE },
};

#undef _S
#undef _N

/* ──────────────────────────────────────────────────────────────────────
 * Internal helpers
 * ────────────────────────────────────────────────────────────────────── */

/* Wait for the HID endpoint to be ready, with a timeout. */
static bool wait_hid_ready(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    while (!tud_hid_ready()) {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++elapsed >= timeout_ms) return false;
    }
    return true;
}

static void send_report(uint8_t modifier, uint8_t keycode)
{
    if (!wait_hid_ready(100)) return;
    uint8_t keycodes[6] = { keycode, 0, 0, 0, 0, 0 };
    tud_hid_keyboard_report(0, modifier, keycodes);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_KEY_PRESS_MS));
}

static void release_all(void)
{
    if (!wait_hid_ready(100)) return;
    tud_hid_keyboard_report(0, 0, NULL);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_KEY_RELEASE_MS));
}

/* ──────────────────────────────────────────────────────────────────────
 * Public API
 * ────────────────────────────────────────────────────────────────────── */

void hid_keyboard_init(void)
{
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,  /* use TinyUSB defaults */
        .string_descriptor        = s_hid_string_descriptor,
        .string_descriptor_count  = sizeof(s_hid_string_descriptor) /
                                    sizeof(s_hid_string_descriptor[0]),
        .external_phy             = false,
        .configuration_descriptor = s_hid_configuration_descriptor,
    };

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TinyUSB install failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "TinyUSB HID keyboard initialised");
    }
}

bool hid_keyboard_mounted(void)
{
    return tud_mounted();
}

void hid_keyboard_type_string(const char *str)
{
    if (!str) return;

    vTaskDelay(pdMS_TO_TICKS(CONFIG_TYPING_INITIAL_DELAY_MS));

    for (const char *p = str; *p; p++) {
        uint8_t ch = (uint8_t)*p;
        if (ch < 0x20 || ch > 0x7E) {
            /* Skip non-printable characters. */
            continue;
        }
        const keymap_entry_t *entry = &s_ascii_keymap[ch - 0x20];
        send_report(entry->modifier, entry->keycode);
        release_all();
    }
}

void hid_keyboard_send_key(uint8_t modifier, uint8_t keycode)
{
    send_report(modifier, keycode);
    release_all();
}

void hid_keyboard_press_enter(void)
{
    send_report(0, HID_KEY_ENTER);
    release_all();
}
