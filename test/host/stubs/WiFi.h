#pragma once

constexpr int WIFI_STA = 1;
constexpr int WL_CONNECTED = 3;

struct WiFiStub {
  int connectionStatus = 0;
  unsigned attempts = 0;
  void mode(int) {}
  void setAutoReconnect(bool) {}
  void begin(const char*, const char*) { ++attempts; }
  void disconnect() { connectionStatus = 0; }
  int status() const { return connectionStatus; }
};
inline WiFiStub WiFi;
