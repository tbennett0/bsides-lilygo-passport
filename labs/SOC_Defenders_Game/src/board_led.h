#pragma once

#include <stdint.h>

// LilyGO T-Dongle S3 onboard APA102/SK9822 (factory led.ino: DI=40, CI=39).
namespace BoardLed {

void init();
void setRgb(uint8_t r, uint8_t g, uint8_t b);
void tickHeartbeat(uint32_t nowMs);

}  // namespace BoardLed
