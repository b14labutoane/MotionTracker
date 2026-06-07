#include "stepper_tracker.h"
#include "config.h"

static const uint8_t HALF_SEQ[8] = {
    0b1001, 0b1000, 0b1100, 0b0100,
    0b0110, 0b0010, 0b0011, 0b0001
};

static int posStep = 0;

static void coils(uint8_t p) {
    digitalWrite(STEPPER_IN1, (p >> 3) & 1);
    digitalWrite(STEPPER_IN2, (p >> 2) & 1);
    digitalWrite(STEPPER_IN3, (p >> 1) & 1);
    digitalWrite(STEPPER_IN4,  p       & 1);
}

static void release() { coils(0); }

static int stepsToDeg()      { return (int32_t)posStep * 360 / STEPS_PER_REV; }
static int degToSteps(int d) { return (int32_t)d * STEPS_PER_REV / 360; }

static void runTo(int target) {
    if (target == posStep) return;
    int dir = target > posStep ? 1 : -1;
    int n = abs(target - posStep);
    for (int i = 0; i < n; i++) {
        posStep += dir;
        coils(HALF_SEQ[((posStep % 8) + 8) % 8]);
        delayMicroseconds(STEP_DELAY_US);
        if (i % 500 == 499) delay(1);
    }
    release();
    Serial.print("[STEP] deg=");
    Serial.println(stepsToDeg());
}

void initStepper() {
    pinMode(STEPPER_IN1, OUTPUT);
    pinMode(STEPPER_IN2, OUTPUT);
    pinMode(STEPPER_IN3, OUTPUT);
    pinMode(STEPPER_IN4, OUTPUT);
    release();
    posStep = 0;
    Serial.println("[STEP] init at 0 deg");
}

void stepperMoveTo(int degrees) {
    degrees = constrain(degrees, STEPPER_MIN_DEG, STEPPER_MAX_DEG);
    Serial.print("[STEP] moveTo ");
    Serial.print(degrees);
    Serial.println(" deg");
    runTo(degToSteps(degrees));
}

void stepperMoveDelta(int degrees) {
    int target = stepsToDeg() + degrees;
    Serial.print("[STEP] delta ");
    Serial.print(degrees);
    Serial.print(" -> ");
    Serial.print(target);
    Serial.println(" deg");
    stepperMoveTo(target);
}

int getStepperDeg() {
    return stepsToDeg();
}

void returnToCenter() {
    stepperMoveTo(0);
}
