#pragma once
#include "config.h"
#include "hardware.h"
#include "firebase_helper.h"
#include "esp_camera.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

inline void showKnownFaceSignal() {
  sysState = STATE_ARMED;
  alarmMuted = true;
  setAlarm(false);
  digitalWrite(BUZZER_PIN, LOW);
  alarmNextBeep = 0;
  setRedLED(false);
  setYellowLED(true);
  ledOffTime = millis() + LED_TIMEOUT_MS;
}

inline void showIntruderSignal() {
  sysState = STATE_ALARM;
  alarmMuted = false;
  alarmNextBeep = millis();
  setYellowLED(false);
  setRedLED(true);
  setAlarm(true);
  ledOffTime = 0;
}

inline void showNoFaceSignal() {
  sysState = STATE_ARMED;
  alarmMuted = true;
  setAlarm(false);
  alarmNextBeep = 0;
  setYellowLED(false);
  setRedLED(true);

  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(150);
  }

  ledOffTime = millis() + LED_TIMEOUT_MS;
}

inline void queryFaceRecognitionServer() {
  Serial.println("[FACE] Motion detected - triggering camera capture...");

  // Flash on briefly for capture
  setLight(true);
  delay(350); // wait for exposure/white balance to settle in low light
  camera_fb_t *warmup = esp_camera_fb_get();
  if (warmup) {
    esp_camera_fb_return(warmup);
    delay(80);
  }
  camera_fb_t *fb = esp_camera_fb_get();
  setLight(lightOn); // restore original light state

  if (!fb) {
    Serial.println("[FACE] Camera capture failed!");
    return;
  }

  Serial.printf("[FACE] Frame captured (%d bytes). Connecting to face server...\n", fb->len);

  HTTPClient http;
  updateFaceServerIpFromFirebase();
  String serverUrl = "http://" + faceServerIp + ":" + String(FACE_SERVER_PORT) + "/recognize";
  Serial.println("[FACE] Server URL: " + serverUrl);
  http.begin(serverUrl);
  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(FACE_RECOGNITION_MS);

  // POST the JPEG buffer
  int httpResponseCode = http.POST(fb->buf, fb->len);

  // Free frame buffer immediately
  esp_camera_fb_return(fb);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("[FACE] Server response: " + response);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      JsonArray faces = doc["faces"].as<JsonArray>();
      if (faces.size() > 0) {
        bool hasKnownFace = false;
        String names = "";
        float highestConfidence = 0.0;

        for (JsonObject face : faces) {
          String name = face["name"].as<String>();
          float confidence = face["confidence"].as<float>();

          if (name != "Unknown") {
            hasKnownFace = true;
            if (confidence > highestConfidence) {
              highestConfidence = confidence;
            }
          }
          if (names.length() > 0) names += ", ";
          names += name + " (" + String(confidence, 1) + "%)";
        }

        if (hasKnownFace) {
          Serial.println("[FACE] Access Granted - Known face: " + names);
          showKnownFaceSignal();

          // Log to Firebase RTDB
          logEvent("face_recognized", "security", "Known: " + names);
          updateMitApp("Face Detected", "Known: " + names, highestConfidence / 100.0f, "recognized");
        } else {
          Serial.println("[FACE] Access Denied - Unknown face: " + names);
          showIntruderSignal();

          // Log to Firebase RTDB
          logEvent("unknown_face", "security", "Unknown: " + names);
          updateMitApp("Unknown Face", "Intruder warning!", 0.0f, "alarm");
        }
      } else {
        Serial.println("[FACE] Warning: No faces detected in image.");
        showNoFaceSignal();

        logEvent("motion_alert", "security", "No face detected in motion frame");
        updateMitApp("Unknown Target", "No face detected!", 0.0f, "alarm");
      }
    } else {
      Serial.println("[FACE] JSON parsing failed: " + String(error.c_str()));
    }
  } else {
    Serial.printf("[FACE] HTTP Request failed, error code: %d\n", httpResponseCode);
    // On server error or timeout, sound alert to be safe
    showIntruderSignal();
  }

  http.end();
}
