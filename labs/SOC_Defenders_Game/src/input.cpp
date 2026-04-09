#include "input.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 0
#endif

namespace Input {

static constexpr uint32_t kDebounceMs = 35;
static bool stableDown = false;
static bool rawLast = false;
static uint32_t lastChangeMs = 0;

void init() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  stableDown = (digitalRead(PIN_BUTTON) == LOW);
  rawLast = stableDown;
  lastChangeMs = millis();
}

void update() {
  const bool raw = (digitalRead(PIN_BUTTON) == LOW);
  const uint32_t now = millis();
  if (raw != rawLast) {
    rawLast = raw;
    lastChangeMs = now;
  } else if ((now - lastChangeMs) >= kDebounceMs) {
    stableDown = raw;
  }
}

bool isButtonDown() {
  return stableDown;
}

}  // namespace Input
