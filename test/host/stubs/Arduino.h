#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 2;

inline uint64_t testClockUs = 0;
inline time_t testEpoch = 0;
inline int testPins[40] = {};
inline int testPinModes[40] = {};
inline unsigned testRisingSteps = 0;
inline unsigned testTimeConfigurations = 0;
inline bool testEnableWasHighBeforeOutput = false;

inline uint32_t millis() { return static_cast<uint32_t>(testClockUs / 1000); }
inline uint32_t micros() { return static_cast<uint32_t>(testClockUs); }
inline void delay(uint32_t ms) { testClockUs += uint64_t(ms) * 1000; }
inline int digitalRead(int pin) { return testPins[pin]; }
inline void digitalWrite(int pin, int value) {
  if (pin == 25 && value == HIGH && testPins[pin] == LOW) {
    ++testRisingSteps;
  }
  testPins[pin] = value;
}
inline void pinMode(int pin, int mode) {
  if (pin == 27 && mode == OUTPUT) {
    testEnableWasHighBeforeOutput = testPins[pin] == HIGH;
  }
  testPinModes[pin] = mode;
}
inline void configTzTime(const char* zone, const char*) {
  ++testTimeConfigurations;
  setenv("TZ", zone, 1);
  tzset();
}
inline time_t firmwareTestTime(time_t* result) {
  if (result) { *result = testEpoch; }
  return testEpoch;
}

struct SerialStub {
  void begin(unsigned) {}
  void println(const char*) {}
};
inline SerialStub Serial;
