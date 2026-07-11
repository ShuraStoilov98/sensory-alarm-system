#include <WiFi.h>
#include <time.h>
#include <secrets.h>  // Include WiFi credentials from a separate file for security

// -- Config pins -- //
// Motor driver
#define STEP_PIN 25
#define DIR_PIN 26
#define ENABLE_PIN 27

// Inputs
#define BUTTON_PIN 14
#define DIR_BUTTON 13
#define KILL_SWITCH_CLOSED_PIN 33
#define KILL_SWITCH_OPENED_PIN 32

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// NTP server and timezone offset
constexpr const char* ntpServer = "pool.ntp.org";
constexpr long gmtOffset_sec = 3 * 3600;         // UTC+3 for Bulgaria (summer)
constexpr int daylightOffset_sec = 0;

// Time of last sync
unsigned long lastSyncMillis = 0;
constexpr unsigned long syncInterval = 24UL * 60UL * 60UL * 1000UL;  // 24 hours

// Set alarm time (24-hour format)
int alarmHour = 6;
int alarmMinute = 30;
bool alarmTriggeredToday = false;

// Motor settings
constexpr int DELAY_BETWEEN_STEPS = 500; // microseconds
constexpr unsigned long MOTOR_TIMEOUT = 5000; // 5 seconds safety timeout for motor operations

// Function to step the motor
void stepMotor() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(DELAY_BETWEEN_STEPS);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(DELAY_BETWEEN_STEPS);
}

// -- Helper functions for clean logic -- //
// Check if curtain is fully open (left kill switch triggered as per hardware setup)
bool isFullyOpen() {
  return digitalRead(KILL_SWITCH_OPENED_PIN) == LOW;
}

// Check if curtain is fully closed (right kill switch triggered as per hardware setup)
bool isFullyClosed() {
  return digitalRead(KILL_SWITCH_CLOSED_PIN) == LOW;
}

// Drive button for manual motor run
bool isButtonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}

// Get direction from DIR_BUTTON (0 or 1) = left or right as per hardware setup
bool getDirection() {
  return digitalRead(DIR_BUTTON) == HIGH; // keep as-is for now
}

// -- Core motor control logic -- //
// Run motors in the selected direction to open curtain until end position reached
void openCurtain() {
  Serial.println("Opening curtain...");
  digitalWrite(DIR_PIN, HIGH);   // adjust if reversed
  digitalWrite(ENABLE_PIN, LOW);

  unsigned long start = millis();

  while (!isFullyOpen()) {
    stepMotor();

    // Safety timeout to prevent motor from running indefinitely
    if (millis() - start > MOTOR_TIMEOUT) {
      Serial.printf("ERROR: open timeout (KILL_SWITCH_OPENED_PIN %d raw state = %d, needs LOW to stop)\n",
                    KILL_SWITCH_OPENED_PIN, digitalRead(KILL_SWITCH_OPENED_PIN));
      break;
    }
  }

  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("Stopped opening (limit or timeout)");
}

// Run motors in the selected direction to close curtain until end position reached
void closeCurtain() {
  Serial.println("Closing curtain...");
  digitalWrite(DIR_PIN, LOW);   // adjust if reversed
  digitalWrite(ENABLE_PIN, LOW);

  unsigned long start = millis();

  while (!isFullyClosed()) {
    stepMotor();

    // Safety timeout to prevent motor from running indefinitely
    if (millis() - start > MOTOR_TIMEOUT) {
      Serial.printf("ERROR: close timeout (KILL_SWITCH_CLOSED_PIN %d raw state = %d, needs LOW to stop)\n",
                    KILL_SWITCH_CLOSED_PIN, digitalRead(KILL_SWITCH_CLOSED_PIN));
      break;
    }
  }

  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("Stopped closing (limit or timeout)");
}

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
  Serial.begin(115200); // Start serial for debugging

  // -- Setup NEW motor and input pins -- //
  // Motor pins
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH); // Disable motor driver by default

  // Inputs (use pullups for stability)
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIR_BUTTON, INPUT_PULLUP);
  pinMode(KILL_SWITCH_CLOSED_PIN, INPUT_PULLUP);
  pinMode(KILL_SWITCH_OPENED_PIN, INPUT_PULLUP);

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
  // Get current time for alarm checking and daily reset
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    delay(1000);
    return;
  }

  // Wifi reconnection logic
  static unsigned long lastReconnect = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect > 10000) {
    Serial.println("WiFi lost, reconnecting...");
    connectWiFi(3000); // Try to reconnect with a shorter timeout
    lastReconnect = millis();
  }

  // Print current time every 5 seconds for debugging
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    Serial.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lastPrint = millis();
  }

  // ===== MANUAL CONTROL =====
  static unsigned long lastPress = 0;
  if (!alarmTriggeredToday && isButtonPressed() && millis() - lastPress > 300) {
    lastPress = millis();
    const bool direction = getDirection(); // cache direction for consistent behavior during button press + get direction only once per press to avoid issues if user changes direction while holding button

    if (direction == HIGH && !isFullyOpen()) {
      openCurtain();
    }
    else if (direction == LOW && !isFullyClosed()) {
      closeCurtain();
    }
    else {
      Serial.println("Movement blocked (end reached)");
    }
  }

  // Check for alarm trigger
  if (timeinfo.tm_hour == alarmHour &&
    timeinfo.tm_min == alarmMinute &&
    timeinfo.tm_sec < 5 &&   // 5-second window
    !alarmTriggeredToday &&
    !isButtonPressed() &&
    !isFullyOpen()) { // Don't trigger if button is pressed (manual override) or already fully open
      Serial.println("Alarm time reached, opening curtain...");
      openCurtain();  
      alarmTriggeredToday = true;
      Serial.println("Alarm triggered!");    
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