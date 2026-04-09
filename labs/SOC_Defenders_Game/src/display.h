#pragma once

struct GameState;

namespace Display {

void init();
void draw(GameState& g);

int measureTextWidth(const char* text);

constexpr int width() {
  return 160;
}
constexpr int height() {
  return 80;
}

}  // namespace Display
