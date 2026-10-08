#pragma once

#include <cstdint>
#include <Preferences.h>
#include "curtain_controller.h"
#include "manual_input.h"

class AlarmScheduler {
public:
  void begin();
  void update(CurtainController& controller, ManualInput& manualInput);
  int32_t lastHandledDate() const { return lastDate_; }

private:
  Preferences storage_;
  bool storageReady_ = false;
  int32_t lastDate_ = 0;
  bool datePending_ = false;
  uint32_t lastCheckMs_ = 0;
};
