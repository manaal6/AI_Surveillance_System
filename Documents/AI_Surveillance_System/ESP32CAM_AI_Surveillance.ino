/*
 * ============================================================
 *  AI SURVEILLANCE SYSTEM — ESP32-CAM  v3.1
 *  Firebase RTDB + MIT App Inventor + Google Voice Control
 *  Motion Detection (Frame Diff) | LED + Buzzer Alerts
 * ============================================================
 *  LIBRARIES (Arduino Library Manager):
 *    - Firebase ESP Client  (mobizt)  v4.x
 *    - ArduinoJson                    v7.x
 *  BOARD: AI Thinker ESP32-CAM  (ESP32 Arduino Core v2.x)
 * ============================================================
 */

#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "Firebase_ESP_Client.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"
#include <ArduinoJson.h>

#include "config.h"
#include "camera_init.h"
#include "firebase_helper.h"
#include "motion_detect.h"
#include "hardware.h"
#include "face_recognition_helper.h"
#include "command_handler.h"



// ─── FIREBASE OBJECTS ─────────────────────────────────────────
FirebaseData fbdo;       // Single reused FirebaseData object
FirebaseAuth auth;
FirebaseConfig fbConfig;

// ─── STATE ────────────────────────────────────────────────────
SystemState sysState   = STATE_DISARMED;
bool        lightOn    = false;
bool        alarmMuted = false;
bool        fbConnected = false;
String      faceServerIp = FACE_SERVER_IP;

// ─── TIMING ───────────────────────────────────────────────────
unsigned long lastCommandPoll = 0;
unsigned long lastHeartbeat   = 0;
unsigned long lastMotion      = 0;
unsigned long ledOffTime      = 0;
unsigned long alarmNextBeep   = 0;

long logCounter = 0;

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n=== AI Surveillance System v3.2 Booting ===");

  // Kill Bluetooth — frees ~70KB heap needed for SSL
  btStop();
  setCpuFrequencyMhz(240);  // full speed for SSL handshake

  Serial.printf("[HEAP] After btStop: %d bytes free\n", ESP.getFreeHeap());

  initHardwarePins();
  selfTestBlink();

  initCamera();
  Serial.printf("[HEAP] After camera init: %d bytes free\n", ESP.getFreeHeap());
  initWiFi();

  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  delay(1500);

  logCounter = getTimestamp() * 100;

  initFirebase();
  updateFaceServerIpFromFirebase();

  // Create /commands path with NONE so getString never fails on missing node
  Firebase.RTDB.setString(&fbdo,
    ("/commands/" + String(DEVICE_ID) + "/pending").c_str(), "NONE");

  logEvent("boot", "system", "Device online v" + String(FIRMWARE_VER));
  updateDeviceStatus();

  Serial.printf("[HEAP] Before loop: %d bytes free\n", ESP.getFreeHeap());
  Serial.println("=== System Ready v3.2 — polling every 2s ===");
}

// ════════════════════════════════════════════════════════════
//  MAIN LOOP
// ════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // ── Command poll (dedicated fbdo_cmd object)
  if (now - lastCommandPoll >= COMMAND_POLL_MS) {
    lastCommandPoll = now;
    Serial.printf("[LOOP] Polling... heap=%d\n", ESP.getFreeHeap());
    pollFirebaseCommand();
  }

  // ── Heartbeat
  if (now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    sendHeartbeat();
  }

  // ── Auto-off indicator LEDs
  if (ledOffTime > 0 && now >= ledOffTime) {
    ledOffTime = 0;
    setYellowLED(false);
    setRedLED(false);
  }

  // ── Non-blocking alarm beep
  if (sysState == STATE_ALARM && !alarmMuted) {
    if (now >= alarmNextBeep) {
      static bool buzzerOn = false;
      buzzerOn = !buzzerOn;
      digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
      alarmNextBeep = now + (buzzerOn ? ALARM_BEEP_MS : ALARM_INTERVAL_MS);
    }
  }

  // ── PIR motion detection
  if (digitalRead(PIR_PIN) == HIGH) {
    if (now - lastMotion >= MOTION_COOLDOWN_MS) {
      lastMotion = now;
      if (sysState == STATE_ARMED || sysState == STATE_ALARM) {
        Serial.println("[PIR] Motion detected — triggering face recognition...");
        queryFaceRecognitionServer();
      }
    }
  }

  delay(50);
}
