# ELECTRICAL.md

# Sensory Alarm Module — Electrical Wiring Documentation

## Purpose
This document is the source of truth (SoT) for the electrical wiring of the ESP32-based sensory alarm curtain module.

The accompanying wiring diagram (`Wiring_diagram.png`) is useful for visual orientation but was AI-generated and may contain minor inaccuracies or ambiguities.

Always follow THIS document as the authoritative reference.

---

# System Overview

Main components:

- ESP32 DevKit
- DRV8825 stepper driver
- NEMA17 stepper motor
- 9V DC motor power supply
- USB-powered ESP32
- Manual control button
- Direction switch/button
- Open limit switch
- Closed limit switch

---

# Power Architecture

## ESP32 Power

ESP32 is powered ONLY via its USB connector.

Recommended:

- USB connection to PC during development
- 5V USB wall charger for standalone operation

Do NOT inject 5V directly into random ESP32 pins unless intentionally using VIN.

---

## Motor Power

DRV8825 motor power is supplied separately.

Motor PSU:

- 9V DC
- >= 2A recommended

Connections:

| Power Supply | DRV8825 |
|---|---|
| +9V | VMOT |
| GND | GND |

---

# COMMON GROUND (CRITICAL)

Even though the ESP32 is powered via USB, all grounds MUST be connected together.

Required shared ground connections:

| Source | Destination |
|---|---|
| ESP32 GND pin | DRV8825 GND |
| 9V PSU negative | DRV8825 GND |
| Button/switch grounds | Common GND |

The ESP32 USB ground is internally connected to ESP32 GND pins.

Failure to share grounds will cause unreliable or non-functional STEP/DIR signaling.

---

# ESP32 GPIO Mapping

## DRV8825 Control Pins

| Function | ESP32 Pin | DRV8825 Pin |
|---|---|---|
| STEP | GPIO25 | STEP |
| DIR | GPIO26 | DIR |
| ENABLE | GPIO27 | ENABLE |

---

## Inputs

| Function | ESP32 Pin |
|---|---|
| Manual run button | GPIO14 |
| Direction switch/button | GPIO13 |
| Curtain closed limit switch | GPIO33 |
| Curtain opened limit switch | GPIO32 |

IMPORTANT:

GPIO12 was intentionally avoided because it is an ESP32 boot strapping pin.

GPIO13 is used instead for stable boot behavior.

---

# Input Wiring

All inputs use:

```cpp
INPUT_PULLUP
```

Therefore:

- default state = HIGH
- triggered/pressed state = LOW

No external pull-up resistors are required.

---

## Button Wiring

### Manual Run Button

| Connection | Connection |
|---|---|
| GPIO14 | Button terminal 1 |
| GND | Button terminal 2 |

---

## Direction Switch/Button

| Connection | Connection |
|---|---|
| GPIO13 | Switch terminal 1 |
| GND | Switch terminal 2 |

---

## Limit Switch Wiring

### Closed Limit Switch

| Connection | Connection |
|---|---|
| GPIO33 | Switch terminal 1 |
| GND | Switch terminal 2 |

### Opened Limit Switch

| Connection | Connection |
|---|---|
| GPIO32 | Switch terminal 1 |
| GND | Switch terminal 2 |

Logic convention:

- switch open = HIGH
- switch triggered = LOW

---

# DRV8825 Wiring

## Logic Power

| DRV8825 | Connects To |
|---|---|
| VDD | ESP32 3V3 |
| GND | ESP32 GND |

ESP32 3.3V logic is fully compatible with DRV8825.

No level shifter required.

---

## Driver State Pins

RESET and SLEEP must NOT be connected to GND.

Correct configuration:

| DRV8825 Pin | Connects To |
|---|---|
| RESET | 3V3 |
| SLEEP | 3V3 |
| RESET ↔ SLEEP | tied together |

This keeps the driver awake and operational.

---

## ENABLE Pin

ENABLE is controlled by software.

| DRV8825 | ESP32 |
|---|---|
| ENABLE | GPIO27 |

Logic:

| ENABLE State | Driver State |
|---|---|
| LOW | enabled |
| HIGH | disabled |

Firmware initializes driver disabled at boot.

---

## Microstepping Pins

| Pin |
|---|
| M0 |
| M1 |
| M2 |

Current configuration:

- left unconnected
- defaults to full-step mode

This is acceptable for MVP testing.

---

# DRV8825 Protection Capacitor (IMPORTANT)

A bulk capacitor MUST be installed close to the DRV8825.

Required:

- 100uF electrolytic capacitor
- 25V or higher rating

Connections:

| Capacitor Lead | Connection |
|---|---|
| + | VMOT |
| - | GND |

IMPORTANT:

The small yellow capacitor already present on the DRV8825 module is NOT sufficient.

Failure to add the bulk capacitor can damage the DRV8825 from motor voltage spikes.

---

# NEMA17 Wiring

## IMPORTANT

NEMA17 wire colors vary between manufacturers.

Always verify coil pairs using:

- multimeter continuity mode
- or shaft resistance test

---

## Typical Coil Pairing

Most common mapping:

| Coil | Wires |
|---|---|
| Coil A | Black + Green |
| Coil B | Red + Blue |

---

## DRV8825 Motor Outputs

Physical order on DRV8825:

```text
Top
B2
B1
A1
A2
Bottom
```

Typical mapping:

| DRV8825 Pin | Motor Wire |
|---|---|
| B2 | Red |
| B1 | Blue |
| A1 | Black |
| A2 | Green |

If motor only vibrates or twitches:

- coil pairs are likely incorrect
- re-check continuity

---

# Current Limit Adjustment (IMPORTANT)

DRV8825 current limit potentiometer MUST be adjusted before sustained operation.

Starting recommendation:

- conservative / low current setting
- approximately 0.5–0.8A for typical small NEMA17 motors

Failure to adjust current limit may cause:

- overheating
- driver damage
- motor overheating
- missed steps

---

# Safe Power-On Sequence

## Power ON

1. Connect ESP32 USB power
2. Verify ESP32 boot and serial logs
3. Connect 9V motor power

---

## Power OFF

1. Disconnect 9V motor power first
2. Disconnect ESP32 USB second

---

# Critical Safety Rules

NEVER:

- disconnect motor wires while powered
- modify VMOT wiring while powered
- reverse VMOT polarity
- connect RESET/SLEEP to GND

DRV8825 drivers are sensitive to wiring mistakes and voltage spikes.

---

# Firmware Assumptions

Current firmware assumptions:

```cpp
isFullyOpen()   => GPIO32 LOW
isFullyClosed() => GPIO33 LOW
```

Meaning:

- limit switch triggered = LOW
- normal state = HIGH

The firmware also assumes:

```cpp
ENABLE LOW  => motor enabled
ENABLE HIGH => motor disabled
```

---

# Development / Debugging Notes

Recommended development workflow:

- ESP32 powered from PC USB
- serial monitor at 115200 baud
- motor initially tested unloaded
- 9V motor supply disconnected during first logic tests

PlatformIO monitor configuration:

```ini
monitor_speed = 115200
```

---

# Referenced Files

- `Wiring_diagram.png`
- `src/esp32/main.cpp`
- `platformio.ini`

