#include "config.h"
#include "camera_init.h"
#include "motion_detect.h"

#include <esp_camera.h>
#include <esp_heap_caps.h>
#include <string.h>

static const int W = FRAME_WIDTH;
static const int H = FRAME_HEIGHT;
static const int STRIDE = FRAME_STRIDE;
static const int THRESH = MOTION_THRESHOLD;
static const int MIN_PIXELS = MOTION_MIN_PIXELS;

static uint8_t* prevGray = nullptr;
static uint8_t* curGray = nullptr;
static uint8_t* rgbBuf = nullptr;
static int consecutiveMotionFrames = 0;
static const int MIN_CONSECUTIVE_FRAMES = 2;

bool initMotionDetect() {
    size_t graySz = (size_t)(W * H);
    size_t rgbSz = graySz * 3;

    prevGray = (uint8_t*)heap_caps_malloc(graySz, MALLOC_CAP_SPIRAM);
    curGray  = (uint8_t*)heap_caps_malloc(graySz, MALLOC_CAP_SPIRAM);
    rgbBuf   = (uint8_t*)heap_caps_malloc(rgbSz, MALLOC_CAP_SPIRAM);

    if (!prevGray || !curGray || !rgbBuf) {
        cleanupMotionDetect();
        return false;
    }
    memset(prevGray, 0, graySz);
    return true;
}

static uint8_t toGray(const uint8_t* px) {
    return (uint8_t)(((uint16_t)px[0] * 77 +
                      (uint16_t)px[1] * 150 +
                      (uint16_t)px[2] * 29) >> 8);
}

motion_result_t detectMotion(camera_fb_t* fb) {
    motion_result_t r = {};

    bool ok = fmt2rgb888(fb->buf, fb->len, fb->format, rgbBuf);
    if (!ok) return r;

    size_t sz = (size_t)(W * H);
    for (int i = 0; i < (int)sz; i++) {
        curGray[i] = toGray(rgbBuf + i * 3);
    }

    int32_t sumX = 0, sumY = 0;
    uint32_t count = 0;

    for (int y = 0; y < H; y += STRIDE) {
        int rowOff = y * W;
        int rowChanged = 0;

        for (int x = 0; x < W; x += STRIDE) {
            int idx = rowOff + x;
            int diff = (int)curGray[idx] - (int)prevGray[idx];
            if (diff < 0) diff = -diff;
            if (diff > THRESH) {
                rowChanged++;
            }
        }

        // Filter: skip entire row if too many pixels changed (EMI noise band)
        if (rowChanged > (W / STRIDE / 3)) {
            continue;
        }

        for (int x = 0; x < W; x += STRIDE) {
            int idx = rowOff + x;
            int diff = (int)curGray[idx] - (int)prevGray[idx];
            if (diff < 0) diff = -diff;
            if (diff > THRESH) {
                count++;
                sumX += x;
                sumY += y;
            }
        }
    }

    uint8_t* tmp = prevGray;
    prevGray = curGray;
    curGray = tmp;

    r.count = count;
    if (count >= (uint32_t)MIN_PIXELS) {
        consecutiveMotionFrames++;
    } else {
        consecutiveMotionFrames = 0;
    }
    r.motionDetected = (consecutiveMotionFrames >= MIN_CONSECUTIVE_FRAMES);
    if (r.motionDetected && count > 0) {
        r.cx = (int)(sumX / (int32_t)count);
        r.cy = (int)(sumY / (int32_t)count);

        int third = W / 3;
        if (r.cx < third) {
            r.zone = ZONE_LEFT;
        } else if (r.cx < third * 2) {
            r.zone = ZONE_CENTER;
        } else {
            r.zone = ZONE_RIGHT;
        }
    } else {
        r.zone = ZONE_NONE;
    }
    return r;
}

int getConsecutiveFrames() {
    return consecutiveMotionFrames;
}

void resetMotionDetect() {
    if (prevGray) memset(prevGray, 0, (size_t)(W * H));
}

void cleanupMotionDetect() {
    if (prevGray) { heap_caps_free(prevGray); prevGray = nullptr; }
    if (curGray)  { heap_caps_free(curGray);  curGray = nullptr; }
    if (rgbBuf)   { heap_caps_free(rgbBuf);   rgbBuf = nullptr; }
}

const char* zoneToStr(motion_zone_t z) {
    switch (z) {
        case ZONE_LEFT:   return "LEFT";
        case ZONE_CENTER: return "CENTER";
        case ZONE_RIGHT:  return "RIGHT";
        default:          return "NONE";
    }
}
