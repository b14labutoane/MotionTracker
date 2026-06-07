# ESP32-CAM Motion Tracker

PIR-triggered camera with motion detection via frame differencing and servo tracking.

## Overview

1. PIR detects motion → activates tracking
2. Camera captures frames → motion detection via frame differencing
3. SG90 servo rotates camera to follow detected motion

## Hardware

- **Board**: ESP32-CAM AI-Thinker (OV2640 camera built-in)
- **Programmer**: ESP32-CAM-MB baseboard (micro-USB)
- **Motion Sensor**: HC-SR501 PIR
- **Servo**: Tower Pro SG90 (mounts camera for pan rotation)
- **Power**: USB via MB baseboard (5V/2A charger recommended)
- **No SD card needed**

## Pin Configuration

### ESP32-CAM AI-Thinker Pin Usage

| GPIO | Function | Used By |
|------|----------|---------|
| GPIO 0 | Boot mode / XCLK | Camera (pull LOW to flash) |
| GPIO 1 | TX | Serial (free if debug off) |
| GPIO 2 | **Free** | — |
| GPIO 3 | RX | Serial (free if debug off) |
| GPIO 4 | SD card / Flash LED | Free (no SD card) |
| GPIO 5 | SD card CS | Free (no SD card) |
| GPIO 12 | SD card | Free (no SD card) |
| GPIO 13 | **PIR OUT** | HC-SR501 |
| GPIO 14 | **Servo PWM** | SG90 signal |
| GPIO 15 | SD card | Free (no SD card) |
| GPIO 16 | PSRAM | Do not use |
| GPIO 33 | Onboard LED | Output only |
| GPIO 34 | Camera D6 | Input only |
| GPIO 35 | Camera D7 | Input only |
| GPIO 36 | Camera D4 | Input only |
| GPIO 39 | Camera D5 | Input only |

### HC-SR501 PIR Sensor

| PIR Pin | ESP32-CAM GPIO | Notes |
|---------|---------------|-------|
| OUT | GPIO 13 | HIGH = motion detected |
| VCC | 5V | |
| GND | GND | |

### SG90 Servo Motor

| SG90 Wire | ESP32-CAM GPIO | Notes |
|-----------|---------------|-------|
| Orange (Signal) | GPIO 14 | PWM (50Hz, 0.5–2.5ms pulse) |
| Red (VCC) | 5V | External 5V recommended |
| Brown (GND) | GND | Common ground |

## Wiring Diagram

```
                          5V
                           │
        ┌──────────────────┼───────────────────┐
        │                  │                   │
        │           ┌──────┴──────┐            │
        │           │             │            │
        │           │  ESP32-CAM  │            │
        │           │  AI-Thinker │            │
        │           │             │            │
        │           │   OV2640    │            │
        │           │  camera     │            │
        │           │  built-in   │            │
        │           │             │            │
        │           │  GPIO 13 ◄──┼──┐         │
        │           │             │  │         │
        │           │             │  │         │
        │           └─────────────┘  │         │
        │                    ┌───────┴──┐      │
        │                    │ HC-SR501 │      │
        │                    │   PIR    │      │
        │                    │          │      │
        │                    │ OUT ─────┘      │
        │                    │ VCC ◄───────────┤── 5V
        │                    │ GND ◄───────────┤── GND
        │                    └──────────┘      │
        │                                      │
        │           ┌─────────────────┐        │
        │           │    SG90 Servo    │        │
        │           │  (camera mount)  │        │
        │           │                  │        │
        │           │  Orange ─ GPIO 14│        │
        │           │  Red   ◄─────────┤────────┤── 5V
        │           │  Brown ◄─────────┤────────┤── GND
        │           └─────────────────┘        │
        │              ▲                       │
        │              │                       │
        │         ┌────┴────┐                  │
        │         │  Camera │                  │
        │         │ mounted │                  │
        │         │ on servo│                  │
        │         └─────────┘                  │
        │                                      │
        └──────────────────────────────────────┘── GND (common)


  PHYSICAL MOUNTING:
  ─────────────────

           monitoring direction ►►►

           ┌──────────┐
           │  Camera  │────► on servo, rotates left/right
           │  module  │
           └────┬─────┘
                │ mounted on servo horn
     ┌──────────┴──────────┐
     │  SG90    │  HC-SR501│────► same level, side by side
     │  Servo   │   PIR    │      PIR fixed, faces forward
     └────┬─────┴────┬─────┘
          │          │
     ┌────┴──────────┴─────┐
     │        Base         │
     └─────────────────────┘
```

### Connection Summary

```
┌────────────────── ESP32-CAM Setup ──────────────────┐
│                                                      │
│   ESP32-CAM AI-Thinker                               │
│   ┌──────────────┐                                   │
│   │              │  Camera built-in (no wiring)      │
│   │              │                                   │
│   │   GPIO 13 ◄──┼──── HC-SR501 PIR (OUT)           │
│   │              │                                   │
│   │   GPIO 14 ───┼──► SG90 Servo (PWM signal)       │
│   │              │                                   │
│   │   5V      ───┼──► PIR VCC + Servo VCC           │
│   │              │      (external 5V recommended)    │
│   │   GND     ◄──┼──── common ground bus            │
│   └──────────────┘                                   │
│                                                      │
│   Total external wiring: 6 wires                     │
│   - PIR: 3 wires (OUT, VCC, GND)                    │
│   - Servo: 3 wires (Signal, VCC, GND)               │
│                                                      │
└──────────────────────────────────────────────────────┘
```

## Servo Control Reference

| Pulse Width | Angle | Duty Cycle (50Hz) |
|-------------|-------|--------------------|
| 0.5 ms | 0° (left) | 2.5% |
| 1.0 ms | 45° | 5.0% |
| 1.5 ms | 90° (center) | 7.5% |
| 2.0 ms | 135° | 10.0% |
| 2.5 ms | 180° (right) | 12.5% |

## Arduino IDE Setup

### Board Configuration

- **Board**: AI Thinker ESP32-CAM
- **Arduino ESP32 Core**: v2.0.0 (required — face detection headers removed after v2.0.1)

### Required Libraries

| Library | Source | Purpose |
|---------|--------|---------|
| `esp_camera` | arduino-esp32 core | Camera capture |
| `ESP32Servo` | Arduino Library Manager | Servo PWM control |
| `WiFi` | arduino-esp32 core | WiFi AP mode |
| `esp_http_server` | ESP-IDF (bundled) | HTTP server for MJPEG stream |

### Flashing

1. Insert ESP32-CAM into MB baseboard
2. Connect micro-USB cable to PC or USB charger
3. Select board: AI Thinker ESP32-CAM
4. Click Upload in Arduino IDE
5. MB board handles boot mode automatically

## Detection Flow

```
1. Boot → init WiFi AP, camera, servo, HTTP server
2. PIR (GPIO 13) triggers → start scanning
3. Servo scans left-right, camera captures frames
4. Frame differencing detects motion → compute centroid
5. Servo tracks motion centroid (~7 pixels per degree)
6. Web stream: http://192.168.4.1 (bounding box + centroid overlay)
7. No motion for 10 seconds → return to center, wait for PIR
```

## Power Notes

- SG90 draws **100–250mA** under load. If ESP32 resets, use **external 5V** for servo.
- Common **GND** between ESP32-CAM, PIR, and servo is required.
- PIR range: ~3–7m, adjustable via potentiometers on HC-SR501.

## Important Notes

- **No SD card required** — camera frames processed in 4MB PSRAM.
- GPIO 33 has onboard LED — output only, not reliable as input.
- Mount the camera module **directly on the servo horn** for pan rotation.
