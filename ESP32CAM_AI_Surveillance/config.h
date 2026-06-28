#pragma once
#include "Arduino.h"

// ─── WIFI ─────────────────────────────────────────────────────
#define WIFI_SSID        "Redmi Note 8 Pro"
#define WIFI_PASSWORD    "hello1245"

// ─── FIREBASE ─────────────────────────────────────────────────
#define FIREBASE_HOST          "ai-smart-surveillance-system-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY       "AIzaSyDf11aHr3ukM6xxlZ-5HX8xNAw2vKgeVSg"
#define FIREBASE_USER_EMAIL    "manaalpervaiz6@gmail.com"
#define FIREBASE_USER_PASSWORD "manaal6666"

// ─── DEVICE ───────────────────────────────────────────────────
#define DEVICE_ID    "esp32cam-01"
#define FIRMWARE_VER "3.0.0"

// ─── NTP ──────────────────────────────────────────────────────
#define NTP_SERVER      "pool.ntp.org"
#define GMT_OFFSET_SEC  18000   // UTC+5 Pakistan
#define DST_OFFSET_SEC  0

// ─── PIN MAP ──────────────────────────────────────────────────
#define PIR_PIN         15
#define YELLOW_LED_PIN  13
#define RED_LED_PIN     14
#define BUZZER_PIN       2
#define WHITE_LED_PIN    4

// ─── FACE RECOGNITION SERVER ──────────────────────────────────
#define FACE_SERVER_IP          " 192.168.51.142"
#define FACE_SERVER_PORT        5000
#define FACE_RECOGNITION_MS     8000             // Timeout for face server response


// ─── CAMERA PINS (AI Thinker ESP32-CAM) ──────────────────────
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22

// ─── TIMING (ms) ──────────────────────────────────────────────
#define COMMAND_POLL_MS    2000
#define HEARTBEAT_MS      10000
#define MOTION_COOLDOWN_MS 8000
#define LED_TIMEOUT_MS     5000
#define ALARM_BEEP_MS       200
#define ALARM_INTERVAL_MS  1000

// ─── MOTION DETECTION TUNING ──────────────────────────────────
// Pixel difference threshold to consider a pixel "changed"
#define MOTION_PIXEL_THRESHOLD  30
// % of pixels that must change to trigger motion event (0–100)
#define MOTION_TRIGGER_PERCENT   8

// ─── STATE MACHINE ────────────────────────────────────────────
enum SystemState {
  STATE_DISARMED,
  STATE_ARMED,
  STATE_ALARM,
  STATE_SILENT
};

// ─── GLOBAL STATE (defined in .ino, declared extern here) ─────
extern SystemState    sysState;
extern bool           lightOn;
extern bool           alarmMuted;
extern bool           fbConnected;
extern String         faceServerIp;
extern unsigned long  ledOffTime;
extern unsigned long  alarmNextBeep;
extern long           logCounter;
