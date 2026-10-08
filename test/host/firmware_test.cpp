#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// Run the actual firmware with fake GPIO, WiFi, NVS and a controllable clock.
// libc localtime/mktime remain real so timezone rules are exercised too.
#define time firmwareTestTime
#include "../../src/esp32/main.cpp"
#undef time

void tick(uint32_t ms) {
  testClockUs += uint64_t(ms) * 1000;
  loop();
}

void setTime(int year, int month, int day, int hour, int minute, int second = 0) {
  struct tm value = {};
  value.tm_year = year - 1900;
  value.tm_mon = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_min = minute;
  value.tm_sec = second;
  value.tm_isdst = -1;
  testEpoch = mktime(&value);
}

void boot() {
  for (int& pin : testPins) { pin = HIGH; }
  setenv("TZ", TIMEZONE, 1);
  tzset();
  setup();
  assert(testEnableWasHighBeforeOutput);
  assert(testPins[ENABLE_PIN] == HIGH);
  tick(31); // Arm the initially released button.
}

void press(bool opening = true) {
  testPins[DIR_BUTTON] = opening ? HIGH : LOW;
  testPins[BUTTON_PIN] = LOW;
  tick(1);
  tick(31);
}

void release() {
  testPins[BUTTON_PIN] = HIGH;
  tick(1);
  tick(31);
}

void alarm(int day = 8, int second = 0) {
  setTime(2026, 10, day, 6, 30, second);
  tick(251);
}

void stopped() {
  assert(!isMoving());
  assert(testPins[ENABLE_PIN] == HIGH);
  assert(testPins[STEP_PIN] == LOW);
}

void offlineManual() {
  boot();
  press();
  assert(motorState == MotorState::OPENING);
  tick(1);
  assert(testRisingSteps > 0);
  release();
  stopped();
  tick(10000);
  assert(WiFi.attempts == 2);
  WiFi.connectionStatus = WL_CONNECTED;
  tick(1);
  assert(testTimeConfigurations == 1);
  tick(1);
  assert(testTimeConfigurations == 1);
}

void bootHeld() {
  for (int& pin : testPins) { pin = HIGH; }
  testPins[BUTTON_PIN] = LOW;
  setup();
  tick(100);
  stopped();
  release();
  press();
  assert(isMoving());
}

void bounceAndRelease() {
  boot();
  testPins[BUTTON_PIN] = LOW;
  tick(1);
  tick(10);
  testPins[BUTTON_PIN] = HIGH;
  tick(1);
  testPins[BUTTON_PIN] = LOW;
  tick(1);
  tick(20);
  stopped();
  tick(11);
  assert(isMoving());
  testPins[BUTTON_PIN] = HIGH;
  tick(1); // Release stops without waiting 30 ms.
  stopped();
  testPins[BUTTON_PIN] = LOW;
  tick(1);
  tick(31);
  stopped(); // No restart from bounce without a stable release.
}

void manualAfterAlarm() {
  boot();
  alarm();
  assert(motorState == MotorState::OPENING);
  assert(testSavedDate == 20261008 && testStorageWrites == 1);
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  tick(1);
  stopped();
  press(false);
  assert(motorState == MotorState::CLOSING);
  release();
  stopped();
  assert(testStorageWrites == 1);
}

void fullMinuteAndNextDate() {
  boot();
  alarm(8, 58);
  assert(isMoving()); // The old first-five-seconds restriction is gone.
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  tick(1);
  testPins[KILL_SWITCH_OPENED_PIN] = HIGH;
  tick(251);
  stopped();
  assert(testStorageWrites == 1);
  alarm(9);
  assert(isMoving());
  assert(testSavedDate == 20261009 && testStorageWrites == 2);
}

void persistedAndRollback() {
  testSavedDate = 20261008;
  boot();
  alarm();
  stopped();
  assert(testStorageWrites == 0);
  setTime(2026, 10, 7, 6, 30);
  tick(251);
  stopped();
  assert(testStorageWrites == 0);
}

void timeoutLatch() {
  boot();
  press();
  tick(5000);
  assert(motorState == MotorState::FAULT);
  stopped();
  unsigned pulses = testRisingSteps;
  tick(1000);
  release();
  press(false);
  alarm();
  assert(motorState == MotorState::FAULT);
  stopped();
  assert(testRisingSteps == pulses);
}

void contradictoryLimits() {
  boot();
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  testPins[KILL_SWITCH_CLOSED_PIN] = LOW;
  press();
  assert(motorState == MotorState::FAULT);
  stopped();
  assert(testRisingSteps == 0);
}

void destinationAndDirection() {
  boot();
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  press();
  stopped();
  assert(testRisingSteps == 0);
  release();
  press(false); // Travel away from an active opposite limit is allowed.
  assert(motorState == MotorState::CLOSING);
  testPins[DIR_BUTTON] = HIGH;
  tick(1);
  stopped();
  tick(50);
  stopped(); // A held button cannot reverse/restart a stopped move.
}

void automaticOverride() {
  boot();
  alarm();
  testPins[BUTTON_PIN] = LOW;
  tick(1);
  stopped(); // Any raw button press stops automatic travel.
  tick(31);
  stopped();
  release();
  press(false);
  assert(motorState == MotorState::CLOSING);
}

void manualSuppressesAlarm() {
  boot();
  press(false);
  alarm();
  assert(motorState == MotorState::CLOSING);
  assert(testStorageWrites == 0); // No flash write while moving.
  release();
  tick(251);
  stopped();
  assert(testSavedDate == 20261008 && testStorageWrites == 1);
  tick(251);
  stopped(); // Releasing during the alarm minute does not start auto travel.
}

void persistenceFailure() {
  testStorageWriteSucceeds = false;
  boot();
  alarm();
  stopped();
  testStorageWriteSucceeds = true;
  tick(251);
  stopped(); // No retry of today's failed attempt.
  press();
  assert(isMoving()); // Manual control does not require NVS.
}

void briefManualOverride() {
  boot();
  setTime(2026, 10, 8, 6, 30);
  press(false);
  release();
  stopped();
  tick(251);
  stopped(); // A complete press/release between clock polls still suppresses auto.
  assert(lastAlarmDate == 20261008);
}

void missingStorage() {
  testStorageAvailable = false;
  boot();
  alarm();
  stopped();
  press();
  assert(isMoving());
}

void microsRolloverAndNoBurst() {
  boot();
  testClockUs = uint64_t(UINT32_MAX) - 400;
  press();
  lastStepUs = UINT32_MAX - 400;
  testClockUs = uint64_t(UINT32_MAX) + 200;
  // Reset the movement's milliseconds to isolate micros rollover.
  movementStartedMs = millis();
  updateMotor();
  assert(testRisingSteps == 1);
  testClockUs += 50000;
  updateMotor();
  updateMotor();
  assert(testRisingSteps == 1); // One falling edge, no catch-up burst.
}

void millisRollover() {
  boot();
  testClockUs = (uint64_t(UINT32_MAX) - 100) * 1000;
  press();
  tick(4999);
  assert(isMoving());
  tick(1);
  assert(motorState == MotorState::FAULT);
  stopped();
}

void seasonalTime() {
  boot();
  WiFi.connectionStatus = WL_CONNECTED;
  tick(1);
  for (int month : {1, 7}) {
    setTime(2026, month, 8, 6, 30);
    struct tm utc;
    gmtime_r(&testEpoch, &utc);
    assert(utc.tm_hour == (month == 1 ? 4 : 3));
    assert(utc.tm_min == 30);
  }
}

void noLateCatchup() {
  boot();
  setTime(2026, 10, 8, 6, 31);
  tick(251);
  stopped();
  assert(testStorageWrites == 0);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  struct Scenario { const char* name; void (*run)(); };
  const Scenario scenarios[] = {
    {"offline_manual_and_reconnect", offlineManual},
    {"held_button_at_boot", bootHeld},
    {"debounce_and_immediate_release", bounceAndRelease},
    {"manual_after_alarm", manualAfterAlarm},
    {"full_alarm_minute_and_next_date", fullMinuteAndNextDate},
    {"persisted_date_and_clock_rollback", persistedAndRollback},
    {"timeout_latches_fault", timeoutLatch},
    {"contradictory_limits", contradictoryLimits},
    {"destination_limit_and_direction_change", destinationAndDirection},
    {"manual_interrupts_automatic_travel", automaticOverride},
    {"manual_suppresses_alarm_without_flash_stall", manualSuppressesAlarm},
    {"brief_manual_override_between_clock_polls", briefManualOverride},
    {"failed_persistence_preserves_manual_control", persistenceFailure},
    {"missing_storage_preserves_manual_control", missingStorage},
    {"micros_rollover_and_no_pulse_burst", microsRolloverAndNoBurst},
    {"millis_rollover_timeout", millisRollover},
    {"winter_and_summer_timezone", seasonalTime},
    {"no_late_alarm_catchup", noLateCatchup},
  };
  for (const auto& scenario : scenarios) {
    if (std::strcmp(argv[1], scenario.name) == 0) {
      scenario.run();
      std::printf("PASS %s\n", scenario.name);
      return 0;
    }
  }
  return 2;
}
