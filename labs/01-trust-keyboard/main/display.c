/*
 * display.c — ST7735 LCD driver for the LILYGO T-Dongle-S3.
 *
 * Uses ESP-IDF's esp_lcd SPI panel-IO layer for SPI transport,
 * then sends ST7735-specific initialisation commands manually
 * (esp_lcd has no built-in ST7735 panel driver).
 *
 * Only compiled when CONFIG_ENABLE_LCD is set.
 */

#include "sdkconfig.h"
#if CONFIG_ENABLE_LCD

#include "display.h"
#include "board.h"
#include "font8x8.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "display";

/* ── SPI / panel-IO handle ────────────────────────────────────────── */
static esp_lcd_panel_io_handle_t s_io_handle;

/* ── Helpers ──────────────────────────────────────────────────────── */

static void lcd_cmd(uint8_t cmd, const uint8_t *data, size_t len)
{
    esp_lcd_panel_io_tx_param(s_io_handle, cmd, data, len);
}

static void lcd_send_pixels(const void *data, size_t len)
{
    esp_lcd_panel_io_tx_color(s_io_handle, 0x2C, data, len);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* Apply T-Dongle-S3 display offsets */
    x0 += BOARD_LCD_COL_OFFSET;
    x1 += BOARD_LCD_COL_OFFSET;
    y0 += BOARD_LCD_ROW_OFFSET;
    y1 += BOARD_LCD_ROW_OFFSET;

    uint8_t ca[] = { (uint8_t)(x0 >> 8), (uint8_t)x0,
                     (uint8_t)(x1 >> 8), (uint8_t)x1 };
    uint8_t ra[] = { (uint8_t)(y0 >> 8), (uint8_t)y0,
                     (uint8_t)(y1 >> 8), (uint8_t)y1 };
    lcd_cmd(0x2A, ca, 4);   /* CASET */
    lcd_cmd(0x2B, ra, 4);   /* RASET */
}

/* ── Hardware reset ───────────────────────────────────────────────── */

static void lcd_hw_reset(void)
{
    gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << BOARD_LCD_RST,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&rst_cfg);

    gpio_set_level(BOARD_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BOARD_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(BOARD_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

/* ── Backlight ────────────────────────────────────────────────────── */

static void lcd_backlight_on(void)
{
    gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << BOARD_LCD_BL,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bl_cfg);

#if CONFIG_LCD_BACKLIGHT_INVERTED
    gpio_set_level(BOARD_LCD_BL, 0);
#else
    gpio_set_level(BOARD_LCD_BL, 1);
#endif
}

/* ── ST7735 initialisation sequence ───────────────────────────────── */

static void lcd_init_st7735(void)
{
    /* Software reset */
    lcd_cmd(0x01, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(150));

    /* Sleep out */
    lcd_cmd(0x11, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));

    /* Frame rate control (normal mode) */
    lcd_cmd(0xB1, (uint8_t[]){0x01, 0x2C, 0x2D}, 3);
    /* Frame rate control (idle mode) */
    lcd_cmd(0xB2, (uint8_t[]){0x01, 0x2C, 0x2D}, 3);
    /* Frame rate control (partial mode) */
    lcd_cmd(0xB3, (uint8_t[]){0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}, 6);

    /* Column inversion */
    lcd_cmd(0xB4, (uint8_t[]){0x07}, 1);

    /* Power control */
    lcd_cmd(0xC0, (uint8_t[]){0xA2, 0x02, 0x84}, 3);
    lcd_cmd(0xC1, (uint8_t[]){0xC5}, 1);
    lcd_cmd(0xC2, (uint8_t[]){0x0A, 0x00}, 2);
    lcd_cmd(0xC3, (uint8_t[]){0x8A, 0x2A}, 2);
    lcd_cmd(0xC4, (uint8_t[]){0x8A, 0xEE}, 2);

    /* VCOM */
    lcd_cmd(0xC5, (uint8_t[]){0x0E}, 1);

    /* Display inversion ON (needed for T-Dongle-S3 IPS panel) */
    lcd_cmd(0x21, NULL, 0);

    /*
     * Memory Access Control (MADCTL) — 0xC8:
     *   - MY=1, MX=1: 180° rotation to correct for T-Dongle-S3 panel mounting
     *   - BGR colour order (bit 3)
     * Baseline test (0x08) showed text mirrored + at bottom, confirming
     * both axes need flipping.
     */
    lcd_cmd(0x36, (uint8_t[]){0xC8}, 1);

    /* Colour mode: 16-bit (RGB565) */
    lcd_cmd(0x3A, (uint8_t[]){0x05}, 1);

    /* Positive gamma correction */
    lcd_cmd(0xE0, (uint8_t[]){
        0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10
    }, 16);

    /* Negative gamma correction */
    lcd_cmd(0xE1, (uint8_t[]){
        0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10
    }, 16);

    /* Display ON */
    lcd_cmd(0x29, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
}

/* ── Public API ───────────────────────────────────────────────────── */

void display_init(void)
{
    /* 1. Hardware reset */
    lcd_hw_reset();

    /* 2. Configure SPI bus */
    spi_bus_config_t bus_cfg = {
        .sclk_io_num     = BOARD_LCD_SCLK,
        .mosi_io_num     = BOARD_LCD_MOSI,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = BOARD_LCD_WIDTH * BOARD_LCD_HEIGHT * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_LCD_SPI_HOST, &bus_cfg,
                                       SPI_DMA_CH_AUTO));

    /* 3. Attach LCD as SPI device via esp_lcd panel IO */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num       = BOARD_LCD_CS,
        .dc_gpio_num       = BOARD_LCD_DC,
        .spi_mode          = 0,
        .pclk_hz           = BOARD_LCD_SPI_FREQ_HZ,
        .trans_queue_depth  = 10,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)BOARD_LCD_SPI_HOST, &io_cfg, &s_io_handle));

    /* 4. Send ST7735 init commands */
    lcd_init_st7735();

    /* 5. Backlight */
    lcd_backlight_on();

    ESP_LOGI(TAG, "ST7735 LCD initialised (%dx%d)", BOARD_LCD_WIDTH,
             BOARD_LCD_HEIGHT);
}

void display_clear(uint16_t color)
{
    lcd_set_window(0, 0, BOARD_LCD_WIDTH - 1, BOARD_LCD_HEIGHT - 1);

    /* Send entire framebuffer in one RAMWR to avoid RAMWRC compatibility
     * issues — some ST7735 variants ignore 0x3C. 25,600 bytes is fine
     * for ESP32-S3 heap. */
    size_t total = BOARD_LCD_WIDTH * BOARD_LCD_HEIGHT;
    uint16_t *buf = heap_caps_malloc(total * sizeof(uint16_t),
                                     MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!buf) return;

    uint16_t c = __builtin_bswap16(color);
    for (size_t i = 0; i < total; i++) {
        buf[i] = c;
    }
    esp_lcd_panel_io_tx_color(s_io_handle, 0x2C, buf, total * sizeof(uint16_t));
    free(buf);
}

void display_draw_text(int x, int y, const char *text,
                       uint16_t fg, uint16_t bg)
{
    if (!text) return;

    uint16_t fg_be = __builtin_bswap16(fg);
    uint16_t bg_be = __builtin_bswap16(bg);

    for (const char *p = text; *p; p++) {
        uint8_t ch = (uint8_t)*p;
        if (ch < 0x20 || ch > 0x7E) ch = '?';

        const uint8_t *glyph = font8x8_basic[ch - 0x20];

        /* Clip: skip characters that fall off-screen */
        if (x + 8 > BOARD_LCD_WIDTH) {
            /* Wrap to next line */
            x = 0;
            y += 8;
        }
        if (y + 8 > BOARD_LCD_HEIGHT) break;

        lcd_set_window(x, y, x + 7, y + 7);

        uint16_t buf[64];   /* 8×8 = 64 pixels */
        for (int row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                buf[row * 8 + (7 - col)] = (bits & (0x80 >> col)) ? fg_be : bg_be;
            }
        }
        lcd_send_pixels(buf, sizeof(buf));

        x += 8;
    }
}

void display_show_status(const char *line1, const char *line2)
{
    display_clear(COLOR_BLACK);

    /* Line 1: centered-ish, slightly down from top, green text */
    int x1 = 0;
    if (line1) {
        int len = strlen(line1);
        x1 = (BOARD_LCD_WIDTH - len * 8) / 2;
        if (x1 < 0) x1 = 0;
        display_draw_text(x1, 30, line1, COLOR_GREEN, COLOR_BLACK);
    }

    /* Line 2: smaller detail text below */
    if (line2 && line2[0]) {
        int len2 = strlen(line2);
        int x2 = (BOARD_LCD_WIDTH - len2 * 8) / 2;
        if (x2 < 0) x2 = 0;
        display_draw_text(x2, 50, line2, COLOR_WHITE, COLOR_BLACK);
    }
}

#endif /* CONFIG_ENABLE_LCD */
