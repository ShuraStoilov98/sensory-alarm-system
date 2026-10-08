#pragma once

#include <cstdint>
#include "curtain_controller.h"

class ManualInput {
public:
  void begin();
  void update(CurtainController& controller);
  bool takeActivity();

private:
  bool rawPressed_ = false;
  bool stablePressed_ = false;
  bool armed_ = false;
  bool activity_ = false;
  uint32_t changedMs_ = 0;
};
