#pragma once

#include <cstddef>
#include <cstdint>

inline int32_t testSavedDate = 0;
inline unsigned testStorageWrites = 0;
inline bool testStorageAvailable = true;
inline bool testStorageWriteSucceeds = true;

struct Preferences {
  bool begin(const char*, bool) { return testStorageAvailable; }
  int32_t getInt(const char*, int32_t fallback) { return testSavedDate ? testSavedDate : fallback; }
  size_t putInt(const char*, int32_t value) {
    if (!testStorageWriteSucceeds) { return 0; }
    testSavedDate = value;
    ++testStorageWrites;
    return sizeof(value);
  }
};
