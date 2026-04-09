#include "game.h"

#include <Arduino.h>

#include "content.h"
#include "events.h"
#include "input.h"

namespace {

GameState g;
static bool s_prevBoot = false;

bool effectiveIntentBlock(bool latched, ControlPolarity p) {
  if (p == ControlPolarity::ButtonMeansBlock) {
    return latched;
  }
  return !latched;
}

void showFeedback(const char* msg, uint16_t color565, uint32_t ms) {
  g.feedbackLine = msg;
  g.feedbackColor565 = color565;
  g.feedbackUntilMs = millis() + ms;
}

void spawnNewThreat() {
  const uint16_t idx = Events::pickContentIndex();
  const ContentEntry& e = kContent[idx];

  int d = 6000 - g.level * 150;
  if (d < 3000) {
    d = 3000;
  }
  const uint16_t durationMs = static_cast<uint16_t>(d);

  g.threat.text = e.text;
  g.threat.isMalicious = e.isMalicious;
  g.threat.explanation = e.explanation;
  g.threat.severity = 1;
  g.threat.durationMs = durationMs;
  g.threat.deadlineMs = 0;
  g.threat.active = true;
  g.threat.timerRunning = false;
  g.threat.latchedPress = false;
  g.threat.polarity = (random(2) == 0) ? ControlPolarity::ButtonMeansBlock : ControlPolarity::ButtonMeansAllow;
  g.threatSeq++;
}

void resolveThreat() {
  ThreatItem& t = g.threat;
  if (!t.active) {
    return;
  }

  const bool intentBlock = effectiveIntentBlock(t.latchedPress, t.polarity);
  int delta = 0;
  const char* msg = "";
  uint16_t col = 0xFFFF;

  if (t.isMalicious) {
    if (intentBlock) {
      delta = 10;
      msg = "OK BLOCK";
      col = 0x07E0;
    } else {
      delta = -12;
      msg = "MISSED";
      col = 0xF800;
    }
  } else {
    if (!intentBlock) {
      delta = 5;
      msg = "OK ALLOW";
      col = 0x07E0;
    } else {
      delta = -8;
      msg = "FALSE+";
      col = 0xF81F;
    }
  }

  if (delta > 0) {
    g.streakCorrect++;
    if (g.streakCorrect >= 5) {
      g.score += 10;
      g.streakCorrect = 0;
      msg = "STREAK+10";
      col = 0xFFE0;
    }
    g.score += delta;
  } else if (delta < 0) {
    g.streakCorrect = 0;
    g.score += delta;
    if (g.score < 0) {
      g.score = 0;
    }
    g.lives--;
    if (g.lives <= 0) {
      g.lives = 0;
      g.gameOver = true;
    }
  }

  if (delta != 0) {
    showFeedback(msg, col, delta < 0 ? 520 : 320);
  }

  t.active = false;
  g.threatsResolved++;
  if ((g.threatsResolved % 6) == 0) {
    g.level++;
  }
}

}  // namespace

namespace Game {

void init() {
  randomSeed(esp_random() ^ millis());
  g = GameState{};
  g.threat = ThreatItem{};
  Events::init();
  s_prevBoot = Input::isButtonDown();
}

void update(float dt) {
  (void)dt;

  if (g.showSplash) {
    const bool down = Input::isButtonDown();
    if (!s_prevBoot && down) {
      g.showSplash = false;
    }
    s_prevBoot = down;
    return;
  }

  if (millis() >= g.feedbackUntilMs) {
    g.feedbackLine = nullptr;
  }

  if (g.gameOver) {
    const bool down = Input::isButtonDown();
    if (!s_prevBoot && down) {
      init();
      g.showSplash = false;
    }
    s_prevBoot = down;
    return;
  }

  ThreatItem& t = g.threat;
  if (!t.active) {
    spawnNewThreat();
    s_prevBoot = Input::isButtonDown();
    return;
  }

  const bool down = Input::isButtonDown();
  if (!s_prevBoot && down) {
    t.latchedPress = true;
    resolveThreat();
    s_prevBoot = down;
    return;
  }
  s_prevBoot = down;

  if (!t.timerRunning) {
    return;
  }

  if (t.deadlineMs == 0) {
    t.deadlineMs = millis() + t.durationMs;
  }

  if (static_cast<int32_t>(millis() - t.deadlineMs) >= 0) {
    resolveThreat();
  }
}

GameState& state() {
  return g;
}

}  // namespace Game
