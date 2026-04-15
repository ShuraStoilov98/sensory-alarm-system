#include <WiFi.h>
#include <time.h>
#include <secrets.h>  // Include WiFi credentials from a separate file for security

#define ALARM_OUTPUT_PIN 15  // GPIO to trigger Arduino

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// NTP server and timezone offset
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;         // UTC+3 for Bulgaria (summer)
const int daylightOffset_sec = 0;

// Time of last sync
unsigned long lastSyncMillis = 0;
const unsigned long syncInterval = 24UL * 60UL * 60UL * 1000UL;  // 24 hours

// Set alarm time (24-hour format)
int alarmHour = 7;
int alarmMinute = 15;
bool alarmTriggeredToday = false;

// Function to connect to WiFi with a timeout
bool connectWiFi(unsigned long timeoutMs = 10000) {
  WiFi.begin(ssid, password);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(500);
    Serial.print(".");
  }

  return WiFi.status() == WL_CONNECTED;
}

// Function to sync time using NTP
void syncTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Skipping NTP sync (no WiFi)");
    return;
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synced via NTP.");
    lastSyncMillis = millis();
  } else {
    Serial.println("Failed to obtain time.");
  }
}

// Setup function runs once at startup
void setup() {
  Serial.begin(115200);
  pinMode(ALARM_OUTPUT_PIN, OUTPUT);
  digitalWrite(ALARM_OUTPUT_PIN, LOW);

  // Connect to WiFi
  if (connectWiFi()) {
      Serial.println("\nWiFi connected");
    } 
  else {
    Serial.println("\nWiFi FAILED");
  }

  // Initial time sync
  syncTime();
}

// Main loop runs repeatedly
void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    delay(1000);
    return;
  }

  // Wifi reconnection logic
  if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi lost, reconnecting...");
  connectWiFi();
  }

  // Print current time
  Serial.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Check for alarm trigger
  if (timeinfo.tm_hour == alarmHour &&
    timeinfo.tm_min == alarmMinute &&
    timeinfo.tm_sec < 5 &&   // 5-second window
    !alarmTriggeredToday) {
      digitalWrite(ALARM_OUTPUT_PIN, HIGH);
      delay(100);  // 100 ms pulse
      digitalWrite(ALARM_OUTPUT_PIN, LOW);
      Serial.println("Alarm triggered!");
      alarmTriggeredToday = true;
    }

  // Reset daily trigger flag at midnight
  static int lastDay = -1;
  if (timeinfo.tm_mday != lastDay) {
    alarmTriggeredToday = false;
    lastDay = timeinfo.tm_mday;
  }

  // Resync time every 24 hours
  if (millis() - lastSyncMillis >= syncInterval) {
    syncTime();
  }

  delay(1000);
}