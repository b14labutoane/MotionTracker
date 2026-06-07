#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "motion_detect.h"

bool initWebServer();
void updateMotionResult(const motion_result_t* m);
void stopWebServer();

void setWebState(int state, motion_zone_t zone, int stepperAngle);
int getWebState();
motion_zone_t getWebZone();
int getWebStepperAngle();

void setSharedJPEG(const uint8_t* data, size_t len);
const uint8_t* getSharedJPEG(size_t* outLen);

#endif
