#ifndef CONFIG_H
#define CONFIG_H

// Pin definitions
#define PIR_PIN 4
#define STEPPER_IN1 12
#define STEPPER_IN2 13
#define STEPPER_IN3 15
#define STEPPER_IN4 14
#define LED_PIN 33

// WiFi AP credentials
#define WIFI_SSID "MotionTracker"
#define WIFI_PASS "12345678"

// Camera settings
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define FRAME_STRIDE 4
#define JPEG_QUALITY 12

// Motion detection thresholds
#define MOTION_THRESHOLD 25
#define MOTION_MIN_PIXELS 40

// Stepper parameters (28BYJ-48 via ULN2003)
#define STEPS_PER_REV 4096      // half-steps per revolution
#define STEP_DELAY_US 2000      // microseconds per step (~500 steps/sec)
#define STEPPER_MIN_DEG -90
#define STEPPER_MAX_DEG 90
#define TRACK_STEP 5            // degrees per tracking correction
#define MANUAL_STEP 5           // degrees per manual button
#define PIXELS_PER_DEGREE 7

// Timing
#define TRACK_TIMEOUT_MS 10000
#define LOST_TIMEOUT_MS  2000
#define DETECT_INTERVAL_MS 100

// Camera pins (AI-Thinker ESP32-CAM)
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SDA 26
#define CAM_PIN_SCL 27
#define CAM_PIN_D0 5
#define CAM_PIN_D1 18
#define CAM_PIN_D2 19
#define CAM_PIN_D3 21
#define CAM_PIN_D4 36
#define CAM_PIN_D5 39
#define CAM_PIN_D6 34
#define CAM_PIN_D7 35
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

// HTTP server
#define HTTP_PORT 80
#define STREAM_PORT 81

extern volatile bool stepperLocked;
extern volatile int stepperPendingDelta;

#endif
