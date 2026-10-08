#pragma once

#include <cstdint>

enum class MotorState { IDLE, OPENING, CLOSING, FAULT };
enum class CurtainPosition { UNKNOWN, OPEN, CLOSED, CONFLICT };
enum class ControlSource { NONE, MANUAL, ALARM };
enum class Direction { OPEN, CLOSE };
enum class FaultReason { NONE, CONTRADICTORY_LIMITS, TRAVEL_TIMEOUT };
enum class CommandResult {
  STARTED, BUSY, FAULT_LATCHED, ALREADY_AT_TARGET, MANUAL_OVERRIDE, INVALID_SOURCE
};

struct CurtainStatus {
  MotorState motion;
  CurtainPosition position;
  ControlSource source;
  FaultReason fault;
};

// All movement requests share these guards. Only end stops establish position;
// neither an idle motor nor an alarm attempt proves the curtain is open.
class CurtainController {
public:
  void begin();
  void update();
  CommandResult request(Direction direction, ControlSource source);
  void stop(const char* reason);
  bool isMoving() const;
  CurtainStatus status() const;

private:
  void disableDriver();
  void latchFault(FaultReason reason, const char* message);

  MotorState motion_ = MotorState::IDLE;
  ControlSource source_ = ControlSource::NONE;
  FaultReason fault_ = FaultReason::NONE;
  bool stepHigh_ = false;
  uint32_t movementStartedMs_ = 0;
  uint32_t lastStepUs_ = 0;
};
