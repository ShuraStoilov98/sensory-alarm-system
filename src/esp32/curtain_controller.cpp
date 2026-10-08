#include "curtain_controller.h"
#include "config.h"
#include <Arduino.h>

using namespace Config;

void CurtainController::begin() {
  // Set output latches before configuring outputs to avoid an enable glitch.
  digitalWrite(ENABLE_PIN, HIGH);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(KILL_SWITCH_CLOSED_PIN, INPUT_PULLUP);
  pinMode(KILL_SWITCH_OPENED_PIN, INPUT_PULLUP);
}

CurtainStatus CurtainController::status() const {
  const bool opened = digitalRead(KILL_SWITCH_OPENED_PIN) == LOW;
  const bool closed = digitalRead(KILL_SWITCH_CLOSED_PIN) == LOW;
  const CurtainPosition position = opened && closed ? CurtainPosition::CONFLICT :
      opened ? CurtainPosition::OPEN : closed ? CurtainPosition::CLOSED :
      CurtainPosition::UNKNOWN;
  return {motion_, position, source_, fault_};
}

bool CurtainController::isMoving() const {
  return motion_ == MotorState::OPENING || motion_ == MotorState::CLOSING;
}

void CurtainController::disableDriver() {
  digitalWrite(ENABLE_PIN, HIGH);
  digitalWrite(STEP_PIN, LOW);
  stepHigh_ = false;
  source_ = ControlSource::NONE;
}

void CurtainController::stop(const char* reason) {
  disableDriver();
  // Stop never acknowledges or clears a latched fault.
  if (motion_ != MotorState::FAULT) {
    motion_ = MotorState::IDLE;
  }
  Serial.println(reason);
}

void CurtainController::latchFault(FaultReason reason, const char* message) {
  disableDriver();
  motion_ = MotorState::FAULT;
  fault_ = reason;
  Serial.println(message);
}

CommandResult CurtainController::request(Direction direction, ControlSource source) {
  if (motion_ == MotorState::FAULT) {
    return CommandResult::FAULT_LATCHED;
  }
  const CurtainPosition position = status().position;
  if (position == CurtainPosition::CONFLICT) {
    latchFault(FaultReason::CONTRADICTORY_LIMITS,
               "FAULT: both limits active; inspect wiring and restart");
    return CommandResult::FAULT_LATCHED;
  }
  if (isMoving()) {
    return CommandResult::BUSY;
  }
  if (source != ControlSource::MANUAL && source != ControlSource::ALARM) {
    return CommandResult::INVALID_SOURCE;
  }
  const bool opening = direction == Direction::OPEN;
  const bool held = digitalRead(BUTTON_PIN) == LOW;
  if ((source == ControlSource::ALARM && held) ||
      (source == ControlSource::MANUAL &&
       (!held || (digitalRead(DIR_BUTTON) == HIGH) != opening))) {
    return CommandResult::MANUAL_OVERRIDE;
  }
  if (position == (opening ? CurtainPosition::OPEN : CurtainPosition::CLOSED)) {
    return CommandResult::ALREADY_AT_TARGET;
  }

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, opening ? HIGH : LOW);
  motion_ = opening ? MotorState::OPENING : MotorState::CLOSING;
  source_ = source;
  stepHigh_ = false;
  movementStartedMs_ = millis();
  lastStepUs_ = micros();
  digitalWrite(ENABLE_PIN, LOW);
  Serial.println(opening ? "Opening curtain" : "Closing curtain");
  return CommandResult::STARTED;
}

void CurtainController::update() {
  const CurtainPosition position = status().position;
  if (position == CurtainPosition::CONFLICT && motion_ != MotorState::FAULT) {
    latchFault(FaultReason::CONTRADICTORY_LIMITS,
               "FAULT: both limits active; inspect wiring and restart");
  }
  if (!isMoving()) {
    return;
  }
  const bool opening = motion_ == MotorState::OPENING;
  if (position == (opening ? CurtainPosition::OPEN : CurtainPosition::CLOSED)) {
    stop("Stopped: destination limit reached");
    return;
  }
  if (source_ == ControlSource::MANUAL && digitalRead(BUTTON_PIN) != LOW) {
    // Raw release stops immediately; debounce is needed only to start.
    stop("Stopped: manual button released");
    return;
  }
  if (source_ == ControlSource::MANUAL &&
      (digitalRead(DIR_BUTTON) == HIGH) != opening) {
    stop("Stopped: direction changed; release and press to run again");
    return;
  }
  if (static_cast<uint32_t>(millis() - movementStartedMs_) >= MOTOR_TIMEOUT_MS) {
    latchFault(FaultReason::TRAVEL_TIMEOUT,
               "FAULT: movement timed out; inspect mechanism and restart");
    return;
  }
  const uint32_t nowUs = micros();
  if (static_cast<uint32_t>(nowUs - lastStepUs_) >= STEP_HALF_PERIOD_US) {
    // Emit one edge, never a catch-up burst after a delayed loop.
    lastStepUs_ = nowUs;
    stepHigh_ = !stepHigh_;
    digitalWrite(STEP_PIN, stepHigh_ ? HIGH : LOW);
  }
}
