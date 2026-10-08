#pragma once

#include <cstdint>

class NetworkClock {
public:
  void begin();
  void update(bool moving);

private:
  uint32_t lastAttemptMs_ = 0;
  bool wasConnected_ = false;
};
