#ifndef STEPPER_TRACKER_H
#define STEPPER_TRACKER_H

void initStepper();
void stepperMoveTo(int degrees);
void stepperMoveDelta(int degrees);
int getStepperDeg();
void returnToCenter();

#endif
