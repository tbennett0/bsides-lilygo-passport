/*
 * main.c — Firmware Reverse Engineering lab.
 *
 * This firmware contains an XOR-encrypted flag.  The participant's
 * goal is to dump the firmware image with esptool, locate the key
 * and ciphertext in an RE tool (Ghidra, Binary Ninja, etc.), and
 * decrypt the flag using CyberChef or a script.
 *
 * The device displays a taunting message on the LCD but never
 * reveals the flag at runtime.
 */

#include "board.h"
#include "button.h"
#include "display.h"
#include "led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const char *TAG = "main";

/*
 * XOR key and encrypted flag.
 *
 * These are intentionally stored as byte arrays so they appear as
 * recognisable constant data in the firmware binary — easy to spot
 * in a hex editor or disassembler once you know what to look for.
 */
static const uint8_t xor_key[]  = {0x76, 0x6f, 0x6f, 0x64, 0x6f, 0x6f};
static const uint8_t enc_flag[] = {0x1e, 0x0e, 0x0c, 0x0f, 0x30, 0x1b,
                                   0x1e, 0x0a, 0x30, 0x14, 0x03, 0x0e,
                                   0x18, 0x0a, 0x1b};

/*
 * Decrypt the flag into a stack buffer.  This runs once at boot to
 * verify the data is intact (logged at debug level only), but the
 * result is never displayed or stored persistently.
 */
static void verify_flag(void)
{
    size_t key_len  = sizeof(xor_key);
    size_t flag_len = sizeof(enc_flag);
    uint8_t buf[sizeof(enc_flag) + 1];

    memcpy(buf, enc_flag, flag_len);
    for (size_t i = 0; i < flag_len; i++) {
        buf[i] ^= xor_key[i % key_len];
    }
    buf[flag_len] = '\0';

    /* Debug-level only — not visible at default log level */
    ESP_LOGD(TAG, "Flag check: %s", (char *)buf);
}

/* ── Taunt messages shown on the display ─────────────────────────── */

static const char *taunts[] = {
    "Nice try",
    "Read the",
    "firmware",
    "Got esptool?",
    "Dump me",
    "XOR is life",
    "RE me",
};
#define NUM_TAUNTS (sizeof(taunts) / sizeof(taunts[0]))

void app_main(void)
{
    /* ── Initialise hardware ────────────────────────────────────── */
    led_init();
    led_set_color(255, 0, 255, 8);   /* purple — mysterious */

    button_init();
    display_init();

    display_show_status("SECRETS", "inside...");
    ESP_LOGI(TAG, "Firmware RE lab — LILYGO T-Dongle-S3");

    verify_flag();

    /* ── Main loop: cycle taunts on button press ────────────────── */
    int taunt_idx = 0;

    while (1) {
        if (button_pressed()) {
            display_show_status(taunts[taunt_idx], "Press BOOT");
            led_set_color(
                (taunt_idx & 1) ? 0 : 255,
                (taunt_idx & 2) ? 255 : 0,
                (taunt_idx & 4) ? 255 : 128,
                8);

            ESP_LOGI(TAG, "Taunt: %s", taunts[taunt_idx]);
            taunt_idx = (taunt_idx + 1) % NUM_TAUNTS;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
