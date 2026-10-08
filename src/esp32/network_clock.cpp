#include "network_clock.h"
#include "config.h"
#include "secrets.h"
#include <Arduino.h>
#include <WiFi.h>

using namespace Config;

void NetworkClock::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  lastAttemptMs_ = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void NetworkClock::update(bool moving) {
  // Defer network reconfiguration during travel to keep stepping responsive.
  if (moving) {
    return;
  }
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected_) {
    configTzTime(TIMEZONE, NTP_SERVER);
    Serial.println("WiFi connected; NTP service started");
  }
  wasConnected_ = connected;
  const uint32_t nowMs = millis();
  if (!connected && static_cast<uint32_t>(nowMs - lastAttemptMs_) >= WIFI_RETRY_MS) {
    lastAttemptMs_ = nowMs;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("WiFi connection requested");
  }
}
