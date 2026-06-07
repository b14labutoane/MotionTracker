#include "pir_sensor.h"
#include "config.h"

static bool lastLoggedState = false;

void initPIR() {
  pinMode(PIR_PIN, INPUT_PULLDOWN);
}

bool isMotionDetected() {
  static unsigned long lastPirHighTime = 0;
  static bool confirmedPir = false;
  unsigned long now = millis();

  bool rawState = digitalRead(PIR_PIN) == HIGH;

  if (rawState) {
    if (lastPirHighTime == 0) {
      lastPirHighTime = now;
    }
    if (now - lastPirHighTime >= 200) {
      confirmedPir = true;
    }
  } else {
    lastPirHighTime = 0;
    confirmedPir = false;
  }

  if (confirmedPir != lastLoggedState) {
    lastLoggedState = confirmedPir;
    Serial.print("PIR_");
    Serial.println(confirmedPir ? "ON" : "OFF");
  }

  return confirmedPir;
}

bool waitForMotion(unsigned long timeoutMs) {
  unsigned long start = millis();

  while (true) {
    if (isMotionDetected()) {
      return true;
    }

    if (timeoutMs > 0 && (millis() - start >= timeoutMs)) {
      return false;
    }

    yield();
  }
}
