#include "alarm_scheduler.h"
#include "config.h"
#include <Arduino.h>
#include <time.h>

using namespace Config;

void AlarmScheduler::begin() {
  storageReady_ = storage_.begin("curtain-alarm", false);
  if (storageReady_) {
    lastDate_ = storage_.getInt("last-date", 0);
  } else {
    Serial.println("Alarm storage unavailable; manual control remains enabled");
  }
}

void AlarmScheduler::update(CurtainController& controller, ManualInput& manualInput) {
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - lastCheckMs_) < TIME_CHECK_MS) {
    return;
  }
  lastCheckMs_ = nowMs;
  const bool manualOverride = manualInput.takeActivity();
  if (datePending_ && !controller.isMoving()) {
    datePending_ = false;
    if (!storageReady_ || storage_.putInt("last-date", lastDate_) != sizeof(lastDate_)) {
      Serial.println("Cannot persist skipped alarm date");
    }
  }
  time_t now;
  time(&now);
  struct tm localTime;
  // getLocalTime() can wait even with a zero timeout; read the clock directly.
  if (!localtime_r(&now, &localTime) || localTime.tm_year <= 2016 - 1900) {
    return;
  }
  const int32_t date = (localTime.tm_year + 1900) * 10000 +
                       (localTime.tm_mon + 1) * 100 + localTime.tm_mday;
  if (localTime.tm_hour != ALARM_HOUR || localTime.tm_min != ALARM_MINUTE ||
      date <= lastDate_) {
    return;
  }

  // Consume this date even on a manual override, fault or already-open curtain.
  lastDate_ = date;
  const CurtainStatus status = controller.status();
  if (manualOverride || status.motion != MotorState::IDLE ||
      digitalRead(BUTTON_PIN) == LOW || status.position == CurtainPosition::OPEN) {
    // Flash writes are deferred until travel ends to avoid disrupting pulses.
    datePending_ = true;
    Serial.println("Alarm skipped: manual control, limit or fault");
    return;
  }
  // Persist before automatic movement so a reboot cannot cause an auto retry.
  if (!storageReady_ || storage_.putInt("last-date", date) != sizeof(date)) {
    Serial.println("Alarm skipped: cannot persist daily attempt");
    return;
  }
  const CommandResult result = controller.request(Direction::OPEN, ControlSource::ALARM);
  if (result != CommandResult::STARTED) {
    Serial.println("Alarm request blocked by controller state");
  }
}
