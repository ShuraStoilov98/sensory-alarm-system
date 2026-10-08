#pragma once

#include <cstdint>

namespace Config {
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

} // namespace Config
