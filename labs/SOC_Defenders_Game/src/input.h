#pragma once

#include <Arduino.h>

namespace Input {

void init();
void update();

// Stable LOW = pressed (BOOT to GND when pressed)
bool isButtonDown();

}  // namespace Input
