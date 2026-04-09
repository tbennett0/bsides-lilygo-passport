/*
 * main.c — "Trust Me, I'm a Keyboard" lab entry point.
 *
 * State machine:
 *   INIT  → READY  (when USB host mounts the device)
 *   READY → TYPING (on BOOT button press)
 *   TYPING→ DONE   (after payload completes)
 *   DONE  → READY  (on button press — allows repeating the demo)
 *
 * Payload sequence (TYPING state):
 *   1. Ctrl+Alt+T  → open a terminal on Lubuntu / LXQt
 *   2. Wait for terminal to appear
 *   3. Type the configured command
 *   4. Press Enter
 */

#include "app_state.h"
#include "board.h"
#include "button.h"
#include "display.h"
#include "hid_keyboard.h"
#include "led.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "class/hid/hid.h"   /* HID_KEY_T, KEYBOARD_MODIFIER_* */

static const char *TAG = "main";

/* ── LED colours for each state ───────────────────────────────────── */

static void set_state_led(app_state_t state)
{
    switch (state) {
    case APP_STATE_INIT:   led_set_color(  0,   0, 255, 8); break; /* blue   */
    case APP_STATE_READY:  led_set_color(  0, 255,   0, 8); break; /* green  */
    case APP_STATE_TYPING: led_set_color(255,   0,   0, 8); break; /* red    */
    case APP_STATE_DONE:   led_set_color(255,   0, 255, 8); break; /* purple */
    }
}

/* ── Payload execution ────────────────────────────────────────────── */

static void execute_payload(void)
{
#if CONFIG_OPEN_TERMINAL
    ESP_LOGI(TAG, "Sending Ctrl+Alt+T to open terminal...");
    display_show_status("OPENING", "terminal...");

    /* Ctrl + Alt + T */
    hid_keyboard_send_key(
        KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTALT,
        HID_KEY_T);

    vTaskDelay(pdMS_TO_TICKS(CONFIG_TERMINAL_OPEN_DELAY_MS));
#endif

    ESP_LOGI(TAG, "Typing payload: %s", CONFIG_PAYLOAD_COMMAND);
    display_show_status("TYPING", "...");

    hid_keyboard_type_string(CONFIG_PAYLOAD_COMMAND);
    hid_keyboard_press_enter();
}

/* ── Main ─────────────────────────────────────────────────────────── */

void app_main(void)
{
    /* ── Initialise hardware ────────────────────────────────────── */
    led_init();
    set_state_led(APP_STATE_INIT);

    button_init();
    display_init();

    display_show_status("Trust Me", "I'm a Keyboard");
    ESP_LOGI(TAG, "Trust Me, I'm a Keyboard — LILYGO T-Dongle-S3");
    ESP_LOGI(TAG, "Payload: %s", CONFIG_PAYLOAD_COMMAND);

    hid_keyboard_init();

    /* ── State machine loop (10 ms tick) ────────────────────────── */
    app_state_t state = APP_STATE_INIT;

    while (1) {
        switch (state) {

        case APP_STATE_INIT:
            if (hid_keyboard_mounted()) {
                state = APP_STATE_READY;
                set_state_led(state);
                display_show_status("READY", "Press BOOT");
                ESP_LOGI(TAG, "USB mounted — READY");
            }
            break;

        case APP_STATE_READY:
            if (button_pressed()) {
                state = APP_STATE_TYPING;
                set_state_led(state);
                ESP_LOGI(TAG, "Button pressed — executing payload");

                execute_payload();

                state = APP_STATE_DONE;
                set_state_led(state);
                display_show_status("DONE", CONFIG_PAYLOAD_COMMAND);
                ESP_LOGI(TAG, "Payload complete — DONE");
            }
            break;

        case APP_STATE_DONE:
            if (button_pressed()) {
                state = APP_STATE_READY;
                set_state_led(state);
                display_show_status("READY", "Press BOOT");
                ESP_LOGI(TAG, "Reset — READY");
            }
            break;

        case APP_STATE_TYPING:
            /* Should not reach here; typing is blocking. */
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
