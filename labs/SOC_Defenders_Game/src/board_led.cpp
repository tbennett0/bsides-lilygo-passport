#include "board_led.h"

#include <FastLED.h>

namespace {

constexpr uint8_t kDataPin = 40;
constexpr uint8_t kClockPin = 39;
constexpr int kNumLeds = 1;

CRGB leds[kNumLeds];

}  // namespace

namespace BoardLed {

void init() {
  FastLED.addLeds<APA102, kDataPin, kClockPin, BGR>(leds, kNumLeds);
  FastLED.setBrightness(96);
  leds[0] = CRGB::Orange;
  FastLED.show();
}

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  leds[0] = CRGB(r, g, b);
  FastLED.show();
}

void tickHeartbeat(uint32_t nowMs) {
  static uint32_t last = 0;
  static uint8_t phase = 0;
  if (nowMs - last < 180) {
    return;
  }
  last = nowMs;
  phase = static_cast<uint8_t>((phase + 1) % 6);
  switch (phase) {
    case 0:
      leds[0] = CRGB(40, 0, 0);
      break;
    case 1:
      leds[0] = CRGB(0, 40, 0);
      break;
    case 2:
      leds[0] = CRGB(0, 0, 40);
      break;
    default:
      leds[0] = CRGB(15, 15, 15);
      break;
  }
  FastLED.show();
}

}  // namespace BoardLed
