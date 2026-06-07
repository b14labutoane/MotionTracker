#ifndef CAMERA_INIT_H
#define CAMERA_INIT_H

#include <esp_camera.h>

bool initCamera();
camera_fb_t* captureFrame();

#endif
