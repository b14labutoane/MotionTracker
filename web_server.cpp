#include "web_server.h"
#include "config.h"
#include "stepper_tracker.h"
#include "motion_detect.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_camera.h>
#include <esp_heap_caps.h>
#include <string.h>

extern volatile int stepperPendingDelta;

static httpd_handle_t s_server = NULL;
static volatile motion_result_t s_latestMotion = {};

static uint8_t* s_jpegBuf[2] = {nullptr, nullptr};
static volatile int s_writeIdx = 0;
static volatile size_t s_jpegLen[2] = {0};

void setSharedJPEG(const uint8_t* data, size_t len) {
    if (!s_jpegBuf[0]) return;
    if (len > (size_t)(FRAME_WIDTH * FRAME_HEIGHT)) return;
    int idx = s_writeIdx;
    memcpy(s_jpegBuf[idx], data, len);
    s_jpegLen[idx] = len;
    s_writeIdx = 1 - idx;
}

const uint8_t* getSharedJPEG(size_t* outLen) {
    int idx = 1 - s_writeIdx;
    *outLen = s_jpegLen[idx];
    return s_jpegBuf[idx];
}

static volatile int s_state = 0;
static volatile motion_zone_t s_zone = ZONE_NONE;
static volatile int s_stepperAngle = 0;

void setWebState(int state, motion_zone_t zone, int stepperAngle) {
    s_state = state;
    s_zone = zone;
    s_stepperAngle = stepperAngle;
}

int getWebState() { return s_state; }
motion_zone_t getWebZone() { return s_zone; }
int getWebStepperAngle() { return s_stepperAngle; }

void updateMotionResult(const motion_result_t* m) {
    if (m) {
        memcpy((void*)&s_latestMotion, m, sizeof(motion_result_t));
    }
}

static const char* stateToStr(int s) {
    switch (s) {
        case 0: return "WAIT_PIR";
        case 1: return "TRACKING";
        default: return "?";
    }
}

static const char* rotationStr(motion_zone_t z) {
    switch (z) {
        case ZONE_LEFT: return "<< LEFT";
        case ZONE_RIGHT: return "RIGHT >>";
        case ZONE_CENTER: return "CENTER";
        default: return "---";
    }
}

static esp_err_t rootHandler(httpd_req_t* req) {
    const char html[] =
        "<!DOCTYPE html><html><head><title>Motion Tracker</title>"
        "<style>"
        "body{background:#222;color:#eee;font-family:sans-serif;text-align:center;padding:15px}"
        "h1{font-size:1.3em;margin:5px}"
        "#stream{width:320px;border:2px solid #555;margin:10px auto;display:none}"
        ".row{display:flex;justify-content:center;align-items:center;gap:15px;margin:8px}"
        ".box{background:#333;padding:8px 14px;border-radius:6px;font-size:1em}"
        ".val{color:#0f0;font-weight:bold}"
        ".btn{padding:10px 22px;font-size:1em;border:none;border-radius:6px;"
        "cursor:pointer;text-decoration:none;color:#fff}"
        ".on{background:#4a4}.off{background:#a44}.toggle{background:#555}"
        "</style></head><body>"
        "<h1>Motion Tracker v2.1</h1>"
        "<img id='stream' src=''>"
        "<div class='row'>"
        "<button class='btn toggle' onclick=\"var i=document.getElementById('stream');"
        "if(i.src.indexOf('stream')>0){i.src='';i.style.display='none';this.textContent='Show Stream'}"
        "else{i.src='/stream?_='+Date.now();i.style.display='block';this.textContent='Hide Stream'}\">Show Stream</button>"
        "</div>"
        "<div class='row'>"
        "<div class='box'>State: <span class='val' id='st'>---</span></div>"
        "<div class='box'>Zone: <span class='val' id='zn'>---</span></div>"
        "<div class='box'>Angle: <span class='val' id='ag'>---</span></div>"
        "<div class='box'>Pixels: <span class='val' id='px'>---</span></div>"
        "</div>"
        "<div class='row'>"
        "<div class='box'>Stepper: <span class='val' id='sv'>---</span></div>"
        "<a href='/stepper?enable=1' class='btn on'>Unlock</a>"
        "<a href='/stepper?enable=0' class='btn off'>Lock at 0&deg;</a>"
        "</div>"
        "<div class='row'>"
        "<a href='/stepper?delta=-10' class='btn toggle'>&laquo; -10&deg;</a>"
        "<a href='/stepper?delta=-5' class='btn toggle'>&lsaquo; -5&deg;</a>"
        "<a href='/stepper?delta=5' class='btn toggle'>+5&deg; &rsaquo;</a>"
        "<a href='/stepper?delta=10' class='btn toggle'>+10&deg; &raquo;</a>"
        "</div>"
        "<script>"
        "var si=document.getElementById('stream');"
        "si.onerror=function(){setTimeout(function(){"
        "if(si.style.display!=='none')si.src='/stream?_='+Date.now();},2000);};"
        "setInterval(function(){"
        "var x=new XMLHttpRequest();x.onload=function(){"
        "if(this.status==200){"
        "var d=JSON.parse(this.responseText);"
        "document.getElementById('st').textContent=d.state;"
        "document.getElementById('zn').textContent=d.zone;"
        "document.getElementById('ag').textContent=d.angle+'\\u00B0';"
        "document.getElementById('px').textContent=d.pixels;"
        "document.getElementById('sv').textContent=d.locked?'LOCKED':'ACTIVE';"
        "}};x.open('GET','/status');x.send();"
        "},1000);"
        "</script></body></html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, strlen(html));
}

static esp_err_t statusHandler(httpd_req_t* req) {
    char json[128];
    int st = getWebState();
    motion_zone_t zn = getWebZone();
    int ang = getWebStepperAngle();
    uint32_t px = s_latestMotion.count;
    snprintf(json, sizeof(json),
        "{\"state\":\"%s\",\"zone\":\"%s\",\"angle\":%d,\"pixels\":%lu,\"locked\":%s}",
        stateToStr(st), rotationStr(zn), ang, (unsigned long)px,
        stepperLocked ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json, strlen(json));
}

static esp_err_t streamHandler(httpd_req_t* req) {
    esp_err_t res = ESP_OK;
    char partBuf[64];

    res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if (res != ESP_OK) return res;
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while (res == ESP_OK) {
        size_t jpegLen = 0;
        const uint8_t* jpegData = getSharedJPEG(&jpegLen);
        if (!jpegData || jpegLen == 0) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        int hdrLen = snprintf(partBuf, sizeof(partBuf),
            "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            (unsigned int)jpegLen);

        res = httpd_resp_send_chunk(req, partBuf, hdrLen);
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)jpegData, jpegLen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    return res;
}

static esp_err_t stepperHandler(httpd_req_t* req) {
    char query[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[8] = {0};
        if (httpd_query_key_value(query, "enable", val, sizeof(val)) == ESP_OK) {
            if (strcmp(val, "0") == 0) {
                stepperLocked = true;
                returnToCenter();
                Serial.println("Stepper LOCKED at 0 via web");
            } else {
                stepperLocked = false;
                Serial.println("Stepper UNLOCKED via web");
            }
        } else if (httpd_query_key_value(query, "delta", val, sizeof(val)) == ESP_OK) {
            stepperPendingDelta = atoi(val);
            Serial.print("[WEB] delta=");
            Serial.println(stepperPendingDelta);
        }
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

bool initWebServer() {
    size_t bufSz = (size_t)(FRAME_WIDTH * FRAME_HEIGHT);
    s_jpegBuf[0] = (uint8_t*)heap_caps_malloc(bufSz, MALLOC_CAP_SPIRAM);
    s_jpegBuf[1] = (uint8_t*)heap_caps_malloc(bufSz, MALLOC_CAP_SPIRAM);
    if (!s_jpegBuf[0] || !s_jpegBuf[1]) {
        Serial.println("JPEG double-buffer alloc failed");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID);
    esp_wifi_set_ps(WIFI_PS_NONE);
    Serial.println(WiFi.softAPIP());

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_PORT;
    config.max_open_sockets = 4;
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    if (httpd_start(&s_server, &config) != ESP_OK) {
        Serial.println("httpd_start failed");
        return false;
    }

    httpd_uri_t rootUri = { .uri = "/", .method = HTTP_GET, .handler = rootHandler };
    httpd_register_uri_handler(s_server, &rootUri);

    httpd_uri_t statusUri = { .uri = "/status", .method = HTTP_GET, .handler = statusHandler };
    httpd_register_uri_handler(s_server, &statusUri);

    httpd_uri_t streamUri = { .uri = "/stream", .method = HTTP_GET, .handler = streamHandler };
    httpd_register_uri_handler(s_server, &streamUri);

    httpd_uri_t stepperUri = { .uri = "/stepper", .method = HTTP_GET, .handler = stepperHandler };
    httpd_register_uri_handler(s_server, &stepperUri);

    return true;
}

void stopWebServer() {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
