#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include "secrets.h"

// Matches docs/Electrical.md. All switches are active LOW.
constexpr int STEP_PIN = 25;
constexpr int DIR_PIN = 26;
constexpr int ENABLE_PIN = 27;
constexpr int BUTTON_PIN = 14;
constexpr int DIR_BUTTON = 13;
constexpr int KILL_SWITCH_CLOSED_PIN = 33;
constexpr int KILL_SWITCH_OPENED_PIN = 32;

constexpr int ALARM_HOUR = 6;
constexpr int ALARM_MINUTE = 30;
// Bulgaria: UTC+2 in winter, UTC+3 in summer.
constexpr const char* TIMEZONE = "EET-2EEST,M3.5.0/3,M10.5.0/4";
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr uint32_t STEP_HALF_PERIOD_US = 500;
constexpr uint32_t MOTOR_TIMEOUT_MS = 5000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t WIFI_RETRY_MS = 10000;
constexpr uint32_t TIME_CHECK_MS = 250;

enum class MotorState { IDLE, OPENING, CLOSING, FAULT };
MotorState motorState = MotorState::IDLE;
bool manualMovement = false;
bool stepHigh = false;
uint32_t movementStartedMs = 0;
uint32_t lastStepUs = 0;

bool rawButtonPressed = false;
bool stableButtonPressed = false;
// A button held during boot must be released before it can start movement.
bool buttonArmed = false;
bool manualOverrideSinceTimeCheck = false;
uint32_t buttonChangedMs = 0;

Preferences alarmStorage;
bool alarmStorageReady = false;
int32_t lastAlarmDate = 0;
bool alarmDatePending = false;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastTimeCheckMs = 0;
bool wifiWasConnected = false;

bool isFullyOpen() {
  return digitalRead(KILL_SWITCH_OPENED_PIN) == LOW;
}

bool isFullyClosed() {
  return digitalRead(KILL_SWITCH_CLOSED_PIN) == LOW;
}

bool isMoving() {
  return motorState == MotorState::OPENING || motorState == MotorState::CLOSING;
}

void stopMotor(const char* reason, bool fault = false) {
  digitalWrite(ENABLE_PIN, HIGH);
  digitalWrite(STEP_PIN, LOW);
  stepHigh = false;
  manualMovement = false;
  motorState = fault ? MotorState::FAULT : MotorState::IDLE;
  Serial.println(reason);
}

void startMovement(bool opening, bool manual) {
  if (motorState != MotorState::IDLE) {
    return;
  }
  if (isFullyOpen() && isFullyClosed()) {
    stopMotor("FAULT: both limits active; inspect wiring and restart", true);
    return;
  }
  if (opening ? isFullyOpen() : isFullyClosed()) {
    Serial.println("Movement blocked: destination limit already active");
    return;
  }

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, opening ? HIGH : LOW);
  motorState = opening ? MotorState::OPENING : MotorState::CLOSING;
  manualMovement = manual;
  stepHigh = false;
  movementStartedMs = millis();
  lastStepUs = micros();
  digitalWrite(ENABLE_PIN, LOW);
  Serial.println(opening ? "Opening curtain" : "Closing curtain");
}

void updateMotor() {
  if (isFullyOpen() && isFullyClosed() && motorState != MotorState::FAULT) {
    stopMotor("FAULT: both limits active; inspect wiring and restart", true);
  }
  if (!isMoving()) {
    return;
  }

  const bool opening = motorState == MotorState::OPENING;
  if (opening ? isFullyOpen() : isFullyClosed()) {
    stopMotor("Stopped: destination limit reached");
    return;
  }
  // Stop on the raw release immediately; debounce is only needed to start.
  if (manualMovement && digitalRead(BUTTON_PIN) != LOW) {
    stopMotor("Stopped: manual button released");
    return;
  }
  if (manualMovement && (digitalRead(DIR_BUTTON) == HIGH) != opening) {
    stopMotor("Stopped: direction changed; release and press to run again");
    return;
  }
  if (static_cast<uint32_t>(millis() - movementStartedMs) >= MOTOR_TIMEOUT_MS) {
    stopMotor("FAULT: movement timed out; inspect mechanism and restart", true);
    return;
  }

  const uint32_t nowUs = micros();
  if (static_cast<uint32_t>(nowUs - lastStepUs) >= STEP_HALF_PERIOD_US) {
    // Never burst out catch-up pulses after a delayed loop.
    lastStepUs = nowUs;
    stepHigh = !stepHigh;
    digitalWrite(STEP_PIN, stepHigh ? HIGH : LOW);
  }
}

void updateButton() {
  const uint32_t nowMs = millis();
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  if (pressed != rawButtonPressed) {
    manualOverrideSinceTimeCheck = true;
    rawButtonPressed = pressed;
    buttonChangedMs = nowMs;
    // Any press can interrupt automatic movement without waiting for debounce.
    if (pressed && isMoving() && !manualMovement) {
      stopMotor("Stopped: manual override; release and press to run manually");
      buttonArmed = false;
    }
  }
  if (static_cast<uint32_t>(nowMs - buttonChangedMs) < BUTTON_DEBOUNCE_MS) {
    return;
  }
  if (!pressed) {
    stableButtonPressed = false;
    buttonArmed = true;
    return;
  }
  if (!stableButtonPressed) {
    stableButtonPressed = true;
    if (buttonArmed) {
      buttonArmed = false;
      startMovement(digitalRead(DIR_BUTTON) == HIGH, true);
    }
  }
}

void updateNetwork() {
  // Defer connection/NTP work during travel to keep step timing responsive.
  if (isMoving()) {
    return;
  }
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wifiWasConnected) {
    // Starts the background SNTP service, including after an offline boot.
    configTzTime(TIMEZONE, NTP_SERVER);
    Serial.println("WiFi connected; NTP service started");
  }
  wifiWasConnected = connected;

  const uint32_t nowMs = millis();
  if (!connected && static_cast<uint32_t>(nowMs - lastWifiAttemptMs) >= WIFI_RETRY_MS) {
    lastWifiAttemptMs = nowMs;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi connection requested");
  }
}

void updateAlarm() {
  const uint32_t nowMs = millis();
  if (static_cast<uint32_t>(nowMs - lastTimeCheckMs) < TIME_CHECK_MS) {
    return;
  }
  lastTimeCheckMs = nowMs;
  const bool manualOverride = manualOverrideSinceTimeCheck;
  manualOverrideSinceTimeCheck = false;
  if (alarmDatePending && !isMoving()) {
    alarmDatePending = false;
    if (!alarmStorageReady || alarmStorage.putInt("last-date", lastAlarmDate) != sizeof(lastAlarmDate)) {
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
      date <= lastAlarmDate) {
    return;
  }

  // Consume this date even on a manual override, fault or already-open curtain.
  lastAlarmDate = date;
  if (manualOverride || motorState != MotorState::IDLE || digitalRead(BUTTON_PIN) == LOW || isFullyOpen()) {
    // Flash writes are deferred until travel ends to avoid disrupting pulses.
    alarmDatePending = true;
    Serial.println("Alarm skipped: manual control, limit or fault");
    return;
  }
  // Persist before automatic movement so a reboot cannot cause an auto retry.
  if (!alarmStorageReady || alarmStorage.putInt("last-date", date) != sizeof(date)) {
    Serial.println("Alarm skipped: cannot persist daily attempt");
    return;
  }
  Serial.println("Alarm time reached");
  startMovement(true, false);
}

void setup() {
  Serial.begin(115200);
  // Set the output latch before enabling the GPIO as an output.
  digitalWrite(ENABLE_PIN, HIGH);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIR_BUTTON, INPUT_PULLUP);
  pinMode(KILL_SWITCH_CLOSED_PIN, INPUT_PULLUP);
  pinMode(KILL_SWITCH_OPENED_PIN, INPUT_PULLUP);

  rawButtonPressed = digitalRead(BUTTON_PIN) == LOW;
  stableButtonPressed = rawButtonPressed;
  buttonChangedMs = millis();
  alarmStorageReady = alarmStorage.begin("curtain-alarm", false);
  if (alarmStorageReady) {
    lastAlarmDate = alarmStorage.getInt("last-date", 0);
  } else {
    Serial.println("Alarm storage unavailable; manual control remains enabled");
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  lastWifiAttemptMs = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Ready: hold manual button to run; release to stop");
}

void loop() {
  updateButton();
  updateMotor();
  updateNetwork();
  updateAlarm();
  // Yield only when idle. Software step timing still needs physical validation.
  if (!isMoving()) {
    delay(1);
  }
}
