#include "display.h"

#include <Arduino.h>
#include <climits>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "game.h"

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7735S _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;

 public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 27000000;
      cfg.freq_read = 16000000;
      cfg.pin_sclk = 5;
      cfg.pin_mosi = 3;
      cfg.pin_miso = -1;
      cfg.pin_dc = 2;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = 4;
      cfg.pin_rst = 1;
      cfg.pin_busy = -1;
      cfg.memory_width = 80;
      cfg.memory_height = 160;
      cfg.panel_width = 80;
      cfg.panel_height = 160;
      cfg.offset_x = 26;
      cfg.offset_y = 1;
      cfg.offset_rotation = 2;
      cfg.invert = true;
      cfg.rgb_order = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = 38;
      cfg.invert = true;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};

static LGFX lcd;
static LGFX_Sprite cmdSprite(&lcd);

namespace {

constexpr int kScreenW = 160;
constexpr int kBarX = 2;
constexpr int kBarY = 68;
constexpr int kBarW = 156;
constexpr int kBarH = 8;
constexpr int kCmdY = 34;
constexpr int kCmdH = 18;
constexpr int kTimeY = 54;
constexpr int kTimeClearW = 56;
constexpr int kTimeClearH = 12;
constexpr int kScrollSpeed = 40;
bool s_spriteReady = false;

uint16_t s_lastThreatSeq = 0xFFFF;
int s_lastScore = INT_MIN;
int s_lastLives = INT_MIN;
int s_lastLevel = INT_MIN;
const char* s_lastThreatText = nullptr;
bool s_wasFeedback = false;
bool s_gameOverDrawn = false;
bool s_splashDrawn = false;
uint32_t s_lastTimePrintMs = 0;

int s_cmdTextW = 0;
int s_scrollX = 0;
uint32_t s_lastScrollMs = 0;
bool s_scrolledIn = false;

void drawShield(int cx, int cy) {
  lcd.fillTriangle(cx, cy - 14, cx - 12, cy - 6, cx + 12, cy - 6, TFT_CYAN);
  lcd.fillRect(cx - 12, cy - 6, 25, 12, TFT_CYAN);
  lcd.fillTriangle(cx - 12, cy + 6, cx + 12, cy + 6, cx, cy + 16, TFT_CYAN);
  lcd.fillRect(cx - 2, cy - 10, 4, 8, TFT_WHITE);
  lcd.fillRect(cx - 5, cy - 7, 10, 4, TFT_WHITE);
}

void drawVirusAt(LGFX_Device& target, int cx, int cy, float squish) {
  const int rx = static_cast<int>(8.0f * (1.0f + squish * 0.35f));
  const int ry = static_cast<int>(8.0f * (1.0f - squish * 0.3f));
  target.fillEllipse(cx, cy, rx, ry, TFT_RED);
  target.fillCircle(cx - 3, cy - 2, 2, TFT_BLACK);
  target.fillCircle(cx + 3, cy - 2, 2, TFT_BLACK);
  target.drawLine(cx - 4, cy + 3, cx + 4, cy + 3, TFT_BLACK);
  const float spikeR = 11.0f * (1.0f - squish * 0.15f);
  for (int a = 0; a < 360; a += 45) {
    float r = 3.14159f * a / 180.0f;
    float srx = spikeR * (1.0f + squish * 0.35f);
    float sry = spikeR * (1.0f - squish * 0.3f);
    int sx = cx + static_cast<int>(srx * cosf(r));
    int sy = cy + static_cast<int>(sry * sinf(r));
    target.fillCircle(sx, sy, 2, TFT_RED);
  }
}

void drawSkull(int cx, int cy) {
  lcd.fillRoundRect(cx - 10, cy - 12, 20, 20, 6, TFT_WHITE);
  lcd.fillRect(cx - 6, cy + 5, 12, 6, TFT_WHITE);
  lcd.fillRect(cx - 5, cy - 6, 4, 5, TFT_BLACK);
  lcd.fillRect(cx + 1, cy - 6, 4, 5, TFT_BLACK);
  lcd.fillTriangle(cx - 2, cy + 1, cx + 2, cy + 1, cx, cy + 4, TFT_BLACK);
  lcd.drawFastVLine(cx - 3, cy + 7, 4, TFT_BLACK);
  lcd.drawFastVLine(cx, cy + 7, 4, TFT_BLACK);
  lcd.drawFastVLine(cx + 3, cy + 7, 4, TFT_BLACK);
}

constexpr int kShieldX = 30;
constexpr int kShieldY = 22;
constexpr int kVirusY = 22;
constexpr int kWallX = 56;
constexpr int kVirusStartX = 140;
constexpr int kVirusAreaX = 42;
constexpr int kVirusAreaW = 118;
constexpr int kVirusAreaY = 4;
constexpr int kVirusAreaH = 38;

float s_virusX = kVirusStartX;
float s_virusVel = 0;
float s_virusSquish = 0;
uint32_t s_lastSplashMs = 0;

enum class VirusState : uint8_t { Charging, Squishing, Bouncing };
VirusState s_virusState = VirusState::Charging;
uint32_t s_squishStartMs = 0;

void drawSplashBackground() {
  lcd.fillScreen(TFT_BLACK);
  drawShield(kShieldX, kShieldY);

  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::top_center);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.drawString("SOC Defenders", kScreenW / 2, 46);

  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.drawString("Press BTN", kScreenW / 2, 66);
}

void resetVirusCharge() {
  s_virusX = kVirusStartX;
  s_virusVel = -(55.0f + static_cast<float>(random(60)));
  s_virusSquish = 0;
  s_virusState = VirusState::Charging;
}

void updateSplashVirus() {
  const uint32_t now = millis();
  const float dt = (now - s_lastSplashMs) / 1000.0f;
  s_lastSplashMs = now;
  if (dt > 0.1f || dt <= 0.0f) {
    return;
  }

  switch (s_virusState) {
    case VirusState::Charging:
      s_virusX += s_virusVel * dt;
      s_virusSquish = 0;
      if (s_virusX <= static_cast<float>(kWallX)) {
        s_virusX = static_cast<float>(kWallX);
        s_virusState = VirusState::Squishing;
        s_squishStartMs = now;
      }
      break;

    case VirusState::Squishing: {
      const uint32_t elapsed = now - s_squishStartMs;
      if (elapsed < 120) {
        s_virusSquish = static_cast<float>(elapsed) / 120.0f;
      } else if (elapsed < 220) {
        s_virusSquish = 1.0f - static_cast<float>(elapsed - 120) / 100.0f;
      } else {
        s_virusSquish = 0;
        s_virusVel = 60.0f + static_cast<float>(random(40));
        s_virusState = VirusState::Bouncing;
      }
      break;
    }

    case VirusState::Bouncing:
      s_virusX += s_virusVel * dt;
      if (s_virusX >= static_cast<float>(kVirusStartX)) {
        resetVirusCharge();
      }
      break;
  }

  lcd.fillRect(kVirusAreaX, kVirusAreaY, kVirusAreaW, kVirusAreaH, TFT_BLACK);
  drawShield(kShieldX, kShieldY);
  drawVirusAt(lcd, static_cast<int>(s_virusX), kVirusY, s_virusSquish);
}

void drawHeart(int cx, int cy, uint16_t col) {
  lcd.fillCircle(cx - 3, cy, 3, col);
  lcd.fillCircle(cx + 3, cy, 3, col);
  lcd.fillTriangle(cx - 6, cy + 1, cx + 6, cy + 1, cx, cy + 8, col);
}

void drawHudLines(GameState& g) {
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::top_left);

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setCursor(2, 2);
  lcd.printf("Score:%d", g.score);

  for (int i = 0; i < g.lives; i++) {
    drawHeart(120 + i * 14, 5, TFT_RED);
  }

  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.setCursor(2, 18);
  if (g.threat.active) {
    if (g.threat.polarity == ControlPolarity::ButtonMeansBlock) {
      lcd.print("BTN=BLOCK");
    } else {
      lcd.print("BTN=ALLOW");
    }
  } else {
    lcd.print("--------");
  }
}

void initCommandScroll(const GameState& g) {
  s_scrolledIn = false;
  if (!g.threat.active || !g.threat.text) {
    s_cmdTextW = 0;
    s_scrollX = 0;
    return;
  }
  lcd.setFont(&fonts::Font2);
  s_cmdTextW = lcd.textWidth(g.threat.text);
  s_scrollX = kScreenW;
  s_lastScrollMs = millis();
}

void ensureSprite() {
  if (!s_spriteReady) {
    cmdSprite.setColorDepth(16);
    cmdSprite.createSprite(kScreenW, kCmdH);
    s_spriteReady = true;
  }
}

void drawCommandLine(GameState& g) {
  ensureSprite();
  cmdSprite.fillSprite(TFT_BLACK);
  if (g.threat.active && g.threat.text) {
    cmdSprite.setFont(&fonts::Font2);
    cmdSprite.setTextColor(TFT_GREEN, TFT_BLACK);
    cmdSprite.setTextDatum(textdatum_t::top_left);
    cmdSprite.setCursor(s_scrollX, 0);
    cmdSprite.print(g.threat.text);
  }
  cmdSprite.pushSprite(0, kCmdY);
}

void updateScroll(GameState& g) {
  if (s_scrolledIn) {
    return;
  }
  const uint32_t now = millis();
  const uint32_t elapsed = now - s_lastScrollMs;
  if (elapsed < 30) {
    return;
  }
  s_lastScrollMs = now;

  s_scrollX -= static_cast<int>((kScrollSpeed * elapsed) / 1000);

  int stopX;
  if (s_cmdTextW <= kScreenW) {
    stopX = (kScreenW - s_cmdTextW) / 2;
  } else {
    stopX = 0;
  }

  if (s_scrollX <= stopX) {
    s_scrollX = stopX;
    s_scrolledIn = true;
    g.threat.timerRunning = true;
  }
}

void fullStaticRedraw(GameState& g) {
  lcd.fillScreen(TFT_BLACK);
  drawHudLines(g);
  initCommandScroll(g);
  drawCommandLine(g);
  lcd.fillRect(kBarX, kBarY, kBarW, kBarH, 0x2104);
  lcd.fillRect(2, kTimeY - 2, kTimeClearW, kTimeClearH, TFT_BLACK);
  s_lastTimePrintMs = 0;
}

void drawTimerBar(GameState& g) {
  if (!g.threat.active || g.threat.durationMs == 0) {
    return;
  }
  const uint32_t now = millis();
  int32_t rem = static_cast<int32_t>(g.threat.deadlineMs - now);
  if (rem < 0) {
    rem = 0;
  }
  const uint32_t dur = g.threat.durationMs;
  int w = static_cast<int>((static_cast<int64_t>(rem) * kBarW) / static_cast<int64_t>(dur));
  if (w > kBarW) {
    w = kBarW;
  }

  lcd.fillRect(kBarX, kBarY, kBarW, kBarH, 0x2104);
  uint16_t col = 0xF800;
  if (dur > 0) {
    const uint32_t pct = (static_cast<uint32_t>(rem) * 100U) / dur;
    if (pct > 50U) {
      col = 0x07E0;
    } else if (pct > 25U) {
      col = 0xFFE0;
    }
  }
  if (w > 0) {
    lcd.fillRect(kBarX, kBarY, w, kBarH, col);
  }
}

void drawTimeRemaining(GameState& g) {
  if (!g.threat.active || g.threat.durationMs == 0) {
    return;
  }
  const uint32_t now = millis();
  if (now - s_lastTimePrintMs < 100) {
    return;
  }
  s_lastTimePrintMs = now;

  int32_t rem = static_cast<int32_t>(g.threat.deadlineMs - now);
  if (rem < 0) {
    rem = 0;
  }
  const float sec = rem / 1000.0f;

  lcd.fillRect(2, kTimeY - 2, kTimeClearW, kTimeClearH, TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setCursor(2, kTimeY);
  lcd.printf("%.1fs", static_cast<double>(sec));
}

void drawGameOverScreen(GameState& g) {
  lcd.fillScreen(TFT_BLACK);

  drawSkull(26, 40);

  lcd.setFont(&fonts::Font4);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.setCursor(46, 24);
  lcd.print("HACKED");

  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setCursor(2, 2);
  lcd.printf("Score:%d", g.score);

  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.setTextDatum(textdatum_t::top_center);
  lcd.drawString("Press BTN", kScreenW / 2, 64);
}

}  // namespace

namespace Display {

void init() {
  s_lastThreatSeq = 0xFFFF;
  s_lastScore = INT_MIN;
  s_lastLives = INT_MIN;
  s_lastLevel = INT_MIN;
  s_lastThreatText = nullptr;
  s_wasFeedback = false;
  s_gameOverDrawn = false;
  s_splashDrawn = false;
  s_lastTimePrintMs = 0;
  s_cmdTextW = 0;
  s_scrollX = 0;

  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
}

int measureTextWidth(const char* text) {
  if (!text) {
    return 0;
  }
  lcd.setFont(&fonts::Font2);
  return lcd.textWidth(text);
}

void draw(GameState& g) {
  if (g.showSplash) {
    if (!s_splashDrawn) {
      drawSplashBackground();
      resetVirusCharge();
      s_lastSplashMs = millis();
      s_splashDrawn = true;
    }
    updateSplashVirus();
    return;
  }
  if (s_splashDrawn) {
    s_splashDrawn = false;
    s_lastThreatSeq = 0xFFFF;
  }

  const bool inFeedback =
      (millis() < g.feedbackUntilMs) && g.feedbackLine && g.feedbackLine[0];

  if (inFeedback) {
    lcd.fillScreen(g.feedbackColor565);
    lcd.setFont(&fonts::Font2);
    lcd.setTextColor(TFT_WHITE, g.feedbackColor565);
    lcd.setCursor(4, 28);
    lcd.print(g.feedbackLine);
    s_wasFeedback = true;
    return;
  }

  const bool exitedFeedback = s_wasFeedback;
  s_wasFeedback = false;

  if (g.gameOver) {
    if (exitedFeedback || !s_gameOverDrawn) {
      drawGameOverScreen(g);
      s_gameOverDrawn = true;
    }
    s_lastThreatSeq = g.threatSeq;
    s_lastScore = g.score;
    s_lastLives = g.lives;
    s_lastLevel = g.level;
    s_lastThreatText = g.threat.text;
    return;
  }

  s_gameOverDrawn = false;

  const bool staticDirty =
      exitedFeedback || (g.threatSeq != s_lastThreatSeq) || (g.score != s_lastScore) ||
      (g.lives != s_lastLives) || (g.level != s_lastLevel) ||
      (g.threat.active && (g.threat.text != s_lastThreatText));

  if (staticDirty) {
    fullStaticRedraw(g);
    s_lastThreatSeq = g.threatSeq;
    s_lastScore = g.score;
    s_lastLives = g.lives;
    s_lastLevel = g.level;
    s_lastThreatText = g.threat.text;
  }

  if (g.threat.active) {
    updateScroll(g);
    drawCommandLine(g);
    if (g.threat.timerRunning) {
      drawTimerBar(g);
      drawTimeRemaining(g);
    }
  }
}

}  // namespace Display
