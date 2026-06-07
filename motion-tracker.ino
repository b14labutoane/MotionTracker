#include "config.h"
#include "stepper_tracker.h"
#include "camera_init.h"
#include "pir_sensor.h"
#include "motion_detect.h"
#include "web_server.h"


#include <WiFi.h>
#include <esp_wifi.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

volatile bool stepperLocked = false;
volatile int stepperPendingDelta = 0;

static void mainTask(void* arg) {
    while (WiFi.getMode() == WIFI_MODE_NULL) delay(100);
    delay(500);
    Serial.println("[MAIN] started");

    enum { WAIT_PIR, TRACKING };
    int state = WAIT_PIR;
    unsigned long lastMotionTime = 0;
    unsigned long lastDetectTime = 0;

    while (true) {
        if (stepperPendingDelta != 0) {
            int delta = stepperPendingDelta;
            stepperPendingDelta = 0;
            Serial.print("[MANUAL] delta=");
            Serial.println(delta);
            stepperMoveDelta(delta);
            if (state == WAIT_PIR) { delay(50); continue; }
        }

        int angle = getStepperDeg();

        if (state == WAIT_PIR) {
            if (!stepperLocked && isMotionDetected()) {
                Serial.println("[MAIN] PIR -> TRACKING");
                state = TRACKING;
                lastMotionTime = millis();
                lastDetectTime = millis();
                resetMotionDetect();
            }
            setWebState(0, ZONE_NONE, angle);
            delay(100);
            continue;
        }

        if (isMotionDetected()) {
            lastMotionTime = millis();
        } else if (millis() - lastMotionTime > LOST_TIMEOUT_MS) {
            Serial.println("[MAIN] PIR lost -> center");
            returnToCenter();
            state = WAIT_PIR;
            continue;
        }

        if (millis() - lastDetectTime < DETECT_INTERVAL_MS) {
            delay(10);
            continue;
        }
        lastDetectTime = millis();

        camera_fb_t* fb = captureFrame();
        if (!fb) { delay(50); continue; }

        motion_result_t m = detectMotion(fb);
        setSharedJPEG(fb->buf, fb->len);
        esp_camera_fb_return(fb);
        updateMotionResult(&m);

        Serial.print("[DET] pix=");
        Serial.print(m.count);
        Serial.print(" consec=");
        Serial.print(getConsecutiveFrames());
        Serial.print(" zone=");
        Serial.print(zoneToStr(m.zone));
        Serial.print(" detected=");
        Serial.println(m.motionDetected ? "YES" : "no");

        if (m.motionDetected && !stepperLocked) {
            int delta = 0;
            if (m.zone == ZONE_LEFT)  delta = -TRACK_STEP;
            if (m.zone == ZONE_RIGHT) delta = TRACK_STEP;
            if (delta != 0) {
                stepperMoveDelta(delta);
                angle = getStepperDeg();
            }
        }

        setWebState(1, m.zone, angle);

        if (millis() - lastMotionTime > TRACK_TIMEOUT_MS) {
            Serial.println("[MAIN] timeout -> center");
            returnToCenter();
            state = WAIT_PIR;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("=== Motion Tracker v2.0 (stepper) ===");

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    initStepper();
    Serial.println("Stepper OK");

    if (!initCamera()) {
        Serial.println("Camera FAIL");
    } else {
        Serial.println("Camera OK");
    }

    initPIR();
    Serial.println("PIR OK");

    if (!initMotionDetect()) {
        Serial.println("MotionDetect FAIL");
    } else {
        Serial.println("MotionDetect OK");
    }

    if (!initWebServer()) {
        Serial.println("WebServer FAIL");
    }

    xTaskCreatePinnedToCore(mainTask, "mainTask", 16384, NULL, 1, NULL, 1);
    Serial.println("=== Ready ===");
}

void loop() {
    delay(100);
}
