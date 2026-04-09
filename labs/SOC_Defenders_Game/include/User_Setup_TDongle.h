// LilyGo T-Dongle S3 — from Bodmer TFT_eSPI Setup209_LilyGo_T_Dongle_S3.h
#define USER_SETUP_ID 209

#define ST7735_DRIVER

#define TFT_WIDTH 80
#define TFT_HEIGHT 160

#define ST7735_GREENTAB160x80
#define TFT_RGB_ORDER TFT_BGR

#define TFT_MISO -1
#define TFT_MOSI 3
#define TFT_SCLK 5
#define TFT_CS 4
#define TFT_DC 2
#define TFT_RST 1

// Backlight is driven manually in display.cpp (LEDC; inverted duty like LilyGO factory).
#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000
