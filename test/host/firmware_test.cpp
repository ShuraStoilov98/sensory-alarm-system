#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// Production modules are compiled separately with fake GPIO, WiFi, NVS and time.
// libc localtime/mktime remain real so timezone rules are exercised too.
#include "../../src/esp32/config.h"
#include "../../src/esp32/curtain_controller.h"
#include "../../src/esp32/alarm_scheduler.h"
#include <WiFi.h>

using namespace Config;
extern CurtainController controller;
extern AlarmScheduler alarmScheduler;
void setup();
void loop();

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
  assert(!controller.isMoving());
  assert(testPins[ENABLE_PIN] == HIGH);
  assert(testPins[STEP_PIN] == LOW);
}

void offlineManual() {
  boot();
  press();
  assert(controller.status().motion == MotorState::OPENING);
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
  assert(controller.isMoving());
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
  assert(controller.isMoving());
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
  assert(controller.status().motion == MotorState::OPENING);
  assert(testSavedDate == 20261008 && testStorageWrites == 1);
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  tick(1);
  stopped();
  press(false);
  assert(controller.status().motion == MotorState::CLOSING);
  release();
  stopped();
  assert(testStorageWrites == 1);
}

void fullMinuteAndNextDate() {
  boot();
  alarm(8, 58);
  assert(controller.isMoving()); // The old first-five-seconds restriction is gone.
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  tick(1);
  testPins[KILL_SWITCH_OPENED_PIN] = HIGH;
  tick(251);
  stopped();
  assert(testStorageWrites == 1);
  alarm(9);
  assert(controller.isMoving());
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
  assert(controller.status().motion == MotorState::FAULT);
  stopped();
  unsigned pulses = testRisingSteps;
  tick(1000);
  release();
  press(false);
  alarm();
  assert(controller.status().motion == MotorState::FAULT);
  stopped();
  assert(testRisingSteps == pulses);
}

void contradictoryLimits() {
  boot();
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  testPins[KILL_SWITCH_CLOSED_PIN] = LOW;
  press();
  assert(controller.status().motion == MotorState::FAULT);
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
  assert(controller.status().motion == MotorState::CLOSING);
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
  assert(controller.status().motion == MotorState::CLOSING);
}

void manualSuppressesAlarm() {
  boot();
  press(false);
  alarm();
  assert(controller.status().motion == MotorState::CLOSING);
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
  assert(controller.isMoving()); // Manual control does not require NVS.
}

void briefManualOverride() {
  boot();
  setTime(2026, 10, 8, 6, 30);
  press(false);
  release();
  stopped();
  tick(251);
  stopped(); // A complete press/release between clock polls still suppresses auto.
  assert(alarmScheduler.lastHandledDate() == 20261008);
}

void missingStorage() {
  testStorageAvailable = false;
  boot();
  alarm();
  stopped();
  press();
  assert(controller.isMoving());
}

void microsRolloverAndNoBurst() {
  boot();
  testClockUs = uint64_t(UINT32_MAX) - 400;
  testPins[BUTTON_PIN] = LOW;
  assert(controller.request(Direction::OPEN, ControlSource::MANUAL) == CommandResult::STARTED);
  testClockUs = uint64_t(UINT32_MAX) + 200;
  controller.update();
  assert(testRisingSteps == 1);
  testClockUs += 50000;
  controller.update();
  controller.update();
  assert(testRisingSteps == 1); // One falling edge, no catch-up burst.
}

void millisRollover() {
  boot();
  testClockUs = (uint64_t(UINT32_MAX) - 100) * 1000;
  press();
  tick(4999);
  assert(controller.isMoving());
  tick(1);
  assert(controller.status().motion == MotorState::FAULT);
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

void positionAndSource() {
  boot();
  auto status = controller.status();
  assert(status.motion == MotorState::IDLE);
  assert(status.position == CurtainPosition::UNKNOWN);
  assert(status.source == ControlSource::NONE);
  assert(status.fault == FaultReason::NONE);
  testPins[KILL_SWITCH_CLOSED_PIN] = LOW;
  assert(controller.status().position == CurtainPosition::CLOSED);
  press();
  assert(controller.status().source == ControlSource::MANUAL);
  testPins[KILL_SWITCH_CLOSED_PIN] = HIGH;
  tick(1);
  assert(controller.status().position == CurtainPosition::UNKNOWN);
  release();
  assert(controller.status().position == CurtainPosition::UNKNOWN);
  assert(controller.status().source == ControlSource::NONE);
  tick(251); // Consume earlier manual activity before the scheduled minute.
  alarm();
  assert(controller.status().source == ControlSource::ALARM);
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  tick(1);
  assert(controller.status().position == CurtainPosition::OPEN);
  assert(controller.status().source == ControlSource::NONE);
}

void busyCommandDoesNotReverse() {
  boot();
  press();
  const unsigned pulses = testRisingSteps;
  assert(controller.request(Direction::CLOSE, ControlSource::ALARM) == CommandResult::BUSY);
  assert(controller.request(Direction::OPEN, ControlSource::MANUAL) == CommandResult::BUSY);
  assert(controller.status().motion == MotorState::OPENING);
  assert(controller.status().source == ControlSource::MANUAL);
  assert(testPins[DIR_PIN] == HIGH);
  assert(testPins[ENABLE_PIN] == LOW);
  assert(testRisingSteps == pulses);
}

void stopPreservesFault() {
  boot();
  press();
  tick(MOTOR_TIMEOUT_MS);
  assert(controller.status().fault == FaultReason::TRAVEL_TIMEOUT);
  controller.stop("Explicit stop");
  assert(controller.status().motion == MotorState::FAULT);
  assert(controller.status().fault == FaultReason::TRAVEL_TIMEOUT);
  assert(controller.status().source == ControlSource::NONE);
  assert(controller.request(Direction::CLOSE, ControlSource::ALARM) == CommandResult::FAULT_LATCHED);
  stopped();
}

void commandGuards() {
  boot();
  assert(controller.request(Direction::OPEN, ControlSource::NONE) == CommandResult::INVALID_SOURCE);
  assert(controller.request(Direction::OPEN, ControlSource::MANUAL) == CommandResult::MANUAL_OVERRIDE);
  testPins[BUTTON_PIN] = LOW;
  assert(controller.request(Direction::OPEN, ControlSource::ALARM) == CommandResult::MANUAL_OVERRIDE);
  assert(controller.request(Direction::CLOSE, ControlSource::MANUAL) == CommandResult::MANUAL_OVERRIDE);
  testPins[KILL_SWITCH_OPENED_PIN] = LOW;
  assert(controller.request(Direction::OPEN, ControlSource::MANUAL) == CommandResult::ALREADY_AT_TARGET);
  assert(controller.status().position == CurtainPosition::OPEN);
  testPins[KILL_SWITCH_CLOSED_PIN] = LOW;
  assert(controller.status().position == CurtainPosition::CONFLICT);
  assert(controller.request(Direction::CLOSE, ControlSource::ALARM) == CommandResult::FAULT_LATCHED);
  assert(controller.status().fault == FaultReason::CONTRADICTORY_LIMITS);
  testPins[KILL_SWITCH_OPENED_PIN] = HIGH;
  testPins[KILL_SWITCH_CLOSED_PIN] = HIGH;
  controller.update();
  assert(controller.status().motion == MotorState::FAULT);
  assert(controller.status().position == CurtainPosition::UNKNOWN);
  stopped();
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
    {"position_and_control_source", positionAndSource},
    {"busy_command_does_not_reverse", busyCommandDoesNotReverse},
    {"stop_preserves_fault", stopPreservesFault},
    {"state_aware_command_guards", commandGuards},
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
