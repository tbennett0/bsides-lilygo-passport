#include "events.h"

#include <Arduino.h>

#include "content.h"

namespace Events {

static uint16_t lastIdx = 0;

void init() {
  lastIdx = UINT16_MAX;
}

uint16_t pickContentIndex() {
  if (kContentCount <= 1) {
    return 0;
  }
  uint16_t idx;
  do {
    idx = static_cast<uint16_t>(random(kContentCount));
  } while (idx == lastIdx);
  lastIdx = idx;
  return idx;
}

}  // namespace Events
