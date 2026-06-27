#pragma once
/*
 * ─────────────────────────────────────────────────────────────
 *  MOTION DETECTION via Frame Differencing
 * ─────────────────────────────────────────────────────────────
 *  How it works:
 *   1. Capture a GRAYSCALE frame (FRAMESIZE_QVGA, 320×240)
 *   2. Compare each pixel against the previous frame
 *   3. Count pixels that changed by > MOTION_PIXEL_THRESHOLD
 *   4. If changed_pixels / total_pixels > MOTION_TRIGGER_PERCENT
 *      → real motion event (not noise)
 *
 *  Why not face recognition?
 *   esp_face_detect.h was removed from ESP32 Arduino Core v2.x+
 *   Frame differencing is more reliable on constrained hardware
 *   and doesn't require deprecated ESP-WHO libraries.
 * ─────────────────────────────────────────────────────────────
 */

#include "config.h"
#include "hardware.h"
#include "firebase_helper.h"
#include "esp_camera.h"

// ── Reference frame buffer (stored in heap) ───────────────────
static uint8_t *prevFrame     = nullptr;
static size_t   prevFrameSize = 0;
static bool     prevFrameValid = false;

// ── Capture a grayscale snapshot for motion comparison ────────
// Returns malloc'd buffer that caller must free(), or nullptr on fail.
static uint8_t* captureGrayscale(size_t &outW, size_t &outH) {
  // Temporarily switch to GRAYSCALE + small frame for speed
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return nullptr;

  // Save current format
  framesize_t  origSize = (framesize_t)s->status.framesize;
  pixformat_t  origFmt  = PIXFORMAT_JPEG; // camera was inited as JPEG

  s->set_framesize(s, FRAMESIZE_QVGA);   // 320×240
  s->set_pixformat(s, PIXFORMAT_GRAYSCALE);

  delay(100); // let AEC settle

  camera_fb_t *fb = esp_camera_fb_get();

  // Restore original settings
  s->set_pixformat(s, origFmt);
  s->set_framesize(s, origSize);

  if (!fb) return nullptr;

  outW = fb->width;
  outH = fb->height;
  size_t len = fb->len;

  uint8_t *buf = (uint8_t *)malloc(len);
  if (buf) memcpy(buf, fb->buf, len);

  esp_camera_fb_return(fb);
  return buf;
}

// ── Compare two grayscale buffers, return % changed pixels ────
static float compareFrames(const uint8_t *a, const uint8_t *b, size_t len) {
  if (!a || !b || len == 0) return 0.0f;

  uint32_t changed = 0;
  for (size_t i = 0; i < len; i++) {
    int diff = (int)a[i] - (int)b[i];
    if (diff < 0) diff = -diff;
    if (diff > MOTION_PIXEL_THRESHOLD) changed++;
  }
  return (float)changed / (float)len * 100.0f;
}

// ── Main motion capture — called on PIR trigger ───────────────
inline void doMotionCapture() {
  // Flash on briefly for better capture
  setLight(true);
  delay(80);

  size_t  w = 0, h = 0;
  uint8_t *frame = captureGrayscale(w, h);

  setLight(lightOn); // restore

  if (!frame) {
    Serial.println("[MOTION] Capture failed");
    return;
  }

  size_t frameLen = w * h;
  float  motionPct = 0.0f;
  bool   realMotion = false;

  if (prevFrameValid && prevFrame && prevFrameSize == frameLen) {
    motionPct = compareFrames(prevFrame, frame, frameLen);
    realMotion = (motionPct >= MOTION_TRIGGER_PERCENT);
    Serial.printf("[MOTION] Changed pixels: %.1f%%  threshold: %d%%\n",
                  motionPct, MOTION_TRIGGER_PERCENT);
  } else {
    // First frame — store as reference, not a real trigger
    Serial.println("[MOTION] Reference frame captured");
  }

  // Update reference frame
  if (prevFrame) free(prevFrame);
  prevFrame      = frame;
  prevFrameSize  = frameLen;
  prevFrameValid = true;

  if (!realMotion) return;

  // ── Real motion confirmed ─────────────────────────────────
  Serial.printf("[MOTION] ALERT! %.1f%% pixels changed\n", motionPct);

  setRedLED(true);
  setYellowLED(false);
  ledOffTime = millis() + LED_TIMEOUT_MS;

  if (sysState == STATE_ARMED) {
    sysState      = STATE_ALARM;
    alarmNextBeep = millis();
    alarmMuted    = false;
  }

  // Write intruder alert to Firebase
  String alertPath = "/alerts/" + String(DEVICE_ID) +
                     "/motion_" + String(getTimestamp());
  Firebase.RTDB.setFloat(&fbdo, alertPath.c_str(), motionPct);

  // Write to MIT App path
  updateMitApp("Motion", "INTRUDER ALERT", motionPct / 100.0f, "alarm");

  logEvent("motion_alert", "security",
           "Motion detected: " + String(motionPct, 1) + "% changed");
}

// ── Call this when you want to reset the motion baseline ──────
inline void resetMotionBaseline() {
  if (prevFrame) {
    free(prevFrame);
    prevFrame = nullptr;
  }
  prevFrameValid = false;
  prevFrameSize  = 0;
  Serial.println("[MOTION] Baseline reset");
}
