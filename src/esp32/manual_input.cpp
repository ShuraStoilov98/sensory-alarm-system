#include "manual_input.h"
#include "config.h"
#include <Arduino.h>

using namespace Config;

void ManualInput::begin() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIR_BUTTON, INPUT_PULLUP);
  rawPressed_ = digitalRead(BUTTON_PIN) == LOW;
  stablePressed_ = rawPressed_;
  changedMs_ = millis();
  // A held button at boot requires a stable release before it can run.
}

bool ManualInput::takeActivity() {
  const bool activity = activity_;
  activity_ = false;
  return activity;
}

void ManualInput::update(CurtainController& controller) {
  const uint32_t nowMs = millis();
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed != rawPressed_) {
    activity_ = true;
    rawPressed_ = pressed;
    changedMs_ = nowMs;
    if (pressed && controller.isMoving() &&
        controller.status().source == ControlSource::ALARM) {
      controller.stop("Stopped: manual override; release and press to run manually");
      armed_ = false;
    }
  }
  if (static_cast<uint32_t>(nowMs - changedMs_) < BUTTON_DEBOUNCE_MS) {
    return;
  }
  if (!pressed) {
    stablePressed_ = false;
    armed_ = true;
    return;
  }
  if (!stablePressed_) {
    stablePressed_ = true;
    if (armed_) {
      armed_ = false;
      const Direction direction = digitalRead(DIR_BUTTON) == HIGH ?
          Direction::OPEN : Direction::CLOSE;
      const CommandResult result = controller.request(direction, ControlSource::MANUAL);
      if (result != CommandResult::STARTED) {
        Serial.println("Manual request blocked by controller state");
      }
    }
  }
}
