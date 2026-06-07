#ifndef MOTION_DETECT_H
#define MOTION_DETECT_H

#include <stdint.h>
#include <esp_camera.h>

typedef enum {
    ZONE_NONE = 0,
    ZONE_LEFT,
    ZONE_CENTER,
    ZONE_RIGHT
} motion_zone_t;

typedef struct {
    bool motionDetected;
    uint32_t count;
    int cx;
    int cy;
    motion_zone_t zone;
} motion_result_t;

bool initMotionDetect();
motion_result_t detectMotion(camera_fb_t* currentFrame);
int getConsecutiveFrames();
void resetMotionDetect();
void cleanupMotionDetect();
const char* zoneToStr(motion_zone_t z);

#endif
