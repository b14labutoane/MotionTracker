#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <Arduino.h>

void initPIR();
bool isMotionDetected();
bool waitForMotion(unsigned long timeoutMs = 0);

#endif
