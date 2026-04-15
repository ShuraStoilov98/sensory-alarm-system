# Sensory Alarm System Build

## 📌 Overview

This project implements a **smart curtain automation system** powered by an ESP32.
It combines:

* Time-based automation (NTP via WiFi)
* Manual control (button + direction switch)
* Safety constraints (end-stop switches)
* Stepper motor control (DRV8825 + NEMA17)

The system opens/closes curtains either:

1. Automatically at a scheduled time (alarm)
2. Manually via a physical button

---

## 🧱 Current Architecture

### Hardware

* **ESP32** (main controller)
* **DRV8825** stepper driver
* **NEMA17 motor**
* **2x limit switches** (fully open / fully closed)
* **1x manual button**
* **1x direction switch**

### Pin Mapping

```cpp
// Motor
STEP_PIN = 25
DIR_PIN = 26
ENABLE_PIN = 27

// Inputs
BUTTON_PIN = 14
DIR_BUTTON = 12
KILL_SWITCH_CLOSED_PIN = 33
KILL_SWITCH_OPENED_PIN = 32
```

---

## ⚙️ Software Design

### Core Responsibilities

* WiFi connection + reconnection
* NTP time sync (daily)
* Alarm trigger logic
* Manual override control
* Motor driving with safety checks

---

### Key Modules

#### 1. Motor Control

* `stepMotor()` generates step pulses
* `openCurtain()` / `closeCurtain()` handle movement
* Safety:

  * End-stop switches
  * Timeout (15s max run)

---

#### 2. Input Handling

* Pull-up configuration (`INPUT_PULLUP`)
* Debounced button input
* Direction read via switch

---

#### 3. Time & Alarm

* NTP sync via `pool.ntp.org`
* Daily alarm trigger (hour + minute)
* Reset logic per day

---

#### 4. WiFi Management

* Initial connect with timeout
* Periodic reconnect (guarded)
* Sync only when connected

---

## 🛡️ Safety Features

* ✅ Motor timeout (prevents infinite run)
* ✅ End-stop detection (hardware limits)
* ✅ Debounced inputs
* ✅ WiFi reconnect guard (prevents spam)
* ✅ Alarm trigger protection (once per day)

---

## ⚠️ Known Limitations (Current Version)

* ❌ Blocking motor control (`while` loops)
* ❌ Blocking WiFi connect (partial)
* ❌ 1-second loop delay (reduced responsiveness)
* ❌ No concurrency (motor blocks system)

---

## 🧠 Design Decisions

### Why ESP32 only?

* Eliminated Arduino Mega (redundant)
* Reduced system complexity
* Improved reliability (no inter-device comms)
* Enabled future features (OTA, app control)

---

### Why `INPUT_PULLUP`?

* Prevents floating inputs
* More stable hardware behavior
* Simplifies wiring (no external resistors)

---

### Why time window (`tm_sec < 5`)?

* Prevents missed alarms due to loop timing

---

## 🚀 Next Steps

### 🔥 High Priority

* [ ] Convert to **non-blocking architecture**
* [ ] Implement **state machine** (IDLE / OPENING / CLOSING)
* [ ] Remove `delay()` usage
* [ ] Make motor stepping time-based (`micros()`)

---

### ⚡ Medium Priority

* [ ] Add motor acceleration (ramp up/down)
* [ ] Improve WiFi reconnect (fully non-blocking)
* [ ] Add persistent config (alarm time, etc.)

---

### 🧪 Testing

* [ ] Validate end-stop reliability
* [ ] Test timeout behavior
* [ ] Simulate WiFi loss
* [ ] Long-run stability test

---

## 🔮 Future Ideas

* Mobile app control (via WiFi)
* OTA firmware updates
* Light sensor integration (auto open at sunrise)
* Multi-curtain system
* Voice assistant integration

---

## 📂 Project Structure

```
sensory-alarm-system-build/
├── src/
│   └── esp32/
│       └── main.cpp
├── include/
│   ├── secrets.h         # ignored in git
│   └── secrets.example.h # template
├── platformio.ini
├── README.md
└── .gitignore
```

---

## 🔐 Secrets Handling

WiFi credentials stored in:

```
include/secrets.h
```

Example:

```cpp
#define WIFI_SSID "your_wifi"
#define WIFI_PASSWORD "your_password"
```

This file is excluded from version control.

---

## ✅ Current Status

✔ Functional
✔ Safe (basic protections in place)
✔ Simplified architecture (ESP32 only)
⚠ Not yet non-blocking
🚀 Ready for next upgrade

---

## 🧠 Key Learning Milestones

* Transition from Arduino IDE → PlatformIO
* Proper project structure + Git integration
* Understanding of embedded constraints
* Introduction to state machines (next step)

---

## 👣 Next Session

👉 Implement **non-blocking state machine architecture**

This will:

* Remove all blocking code
* Enable concurrent behavior
* Upgrade system to production-grade design

---
