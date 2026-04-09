#pragma once

/*
 * board.h — LILYGO T-Dongle-S3 pin definitions and hardware constants.
 *
 * Single source of truth for all GPIO assignments.  If you port this
 * firmware to a different board, only this file needs to change.
 */

#include "driver/gpio.h"
#include "hal/spi_types.h"

/* ── ST7735 LCD (SPI) ─────────────────────────────────────────────── */
#define BOARD_LCD_SPI_HOST   SPI2_HOST
#define BOARD_LCD_SCLK       GPIO_NUM_5
#define BOARD_LCD_MOSI       GPIO_NUM_3
#define BOARD_LCD_CS         GPIO_NUM_4
#define BOARD_LCD_DC         GPIO_NUM_2
#define BOARD_LCD_RST        GPIO_NUM_1
#define BOARD_LCD_BL         GPIO_NUM_38

#define BOARD_LCD_WIDTH      80
#define BOARD_LCD_HEIGHT     160
#define BOARD_LCD_COL_OFFSET 26
#define BOARD_LCD_ROW_OFFSET 1
#define BOARD_LCD_SPI_FREQ_HZ (20 * 1000 * 1000)   /* 20 MHz */

/* ── APA102 RGB LED ───────────────────────────────────────────────── */
#define BOARD_LED_DATA       GPIO_NUM_40
#define BOARD_LED_CLK        GPIO_NUM_39

/* ── BOOT button ──────────────────────────────────────────────────── */
#define BOARD_BUTTON_GPIO    GPIO_NUM_0

