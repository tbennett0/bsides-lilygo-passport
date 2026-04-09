#pragma once

#include <cstdint>

enum class ControlPolarity : uint8_t { ButtonMeansBlock = 0, ButtonMeansAllow = 1 };

struct ThreatItem {
  const char* text = nullptr;
  bool isMalicious = false;
  uint8_t severity = 1;
  uint32_t deadlineMs = 0;
  uint16_t durationMs = 0;
  bool active = false;
  bool timerRunning = false;
  const char* explanation = nullptr;
  bool latchedPress = false;
  ControlPolarity polarity = ControlPolarity::ButtonMeansBlock;
};

struct GameState {
  int score = 0;
  int lives = 3;
  int level = 1;
  int streakCorrect = 0;
  int threatsResolved = 0;
  uint16_t threatSeq = 0;
  ThreatItem threat;
  bool showSplash = true;
  bool gameOver = false;
  bool awaitingRestart = false;
  uint32_t feedbackUntilMs = 0;
  uint16_t feedbackColor565 = 0;
  const char* feedbackLine = nullptr;
};

namespace Game {

void init();
void update(float dtSeconds);

GameState& state();

}  // namespace Game
