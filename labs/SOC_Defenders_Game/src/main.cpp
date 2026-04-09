#include <Arduino.h>

#include "board_led.h"
#include "display.h"
#include "game.h"
#include "input.h"

void setup() {
  BoardLed::init();

  Serial.begin(115200);
  // Native USB CDC: host needs time to attach before the first prints (and before risky init).
  delay(1500);
  Serial.println();
  Serial.println(F("CyberInvaders: start"));
  Serial.flush();

  BoardLed::setRgb(0, 80, 0);

  Display::init();
  Serial.println(F("CyberInvaders: display OK"));
  Serial.flush();

  BoardLed::setRgb(0, 0, 80);

  Input::init();
  Game::init();
  Serial.println(F("CyberInvaders: loop"));
  Serial.flush();
}

void loop() {
  static uint32_t lastMs = millis();
  const uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  if (dt > 0.08f) {
    dt = 0.08f;
  }
  lastMs = now;

  Input::update();
  Game::update(dt);
  Display::draw(Game::state());
  BoardLed::tickHeartbeat(now);
  delay(2);
}
