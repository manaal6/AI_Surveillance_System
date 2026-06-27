#pragma once
#include "config.h"
#include "hardware.h"
#include "Firebase_ESP_Client.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ─── Extern objects (defined in .ino) ─────────────────────────
extern FirebaseData   fbdo;
extern FirebaseAuth   auth;
extern FirebaseConfig fbConfig;
extern long           logCounter;

// ─── Timestamp ────────────────────────────────────────────────
inline long getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return millis() / 1000;
  return (long)mktime(&timeinfo);
}

// ─── Firebase Init ────────────────────────────────────────────
inline void initFirebase() {
  fbConfig.api_key      = FIREBASE_API_KEY;
  fbConfig.database_url = "https://" FIREBASE_HOST;  // FIREBASE_HOST has no https://
  fbConfig.token_status_callback = tokenStatusCallback;

  // Conservative timeouts to avoid blocking loop()
  fbConfig.timeout.serverResponse   = 8 * 1000;
  fbConfig.timeout.socketConnection = 8 * 1000;
  fbConfig.timeout.wifiReconnect    = 8 * 1000;

  auth.user.email    = FIREBASE_USER_EMAIL;
  auth.user.password = FIREBASE_USER_PASSWORD;

  Firebase.begin(&fbConfig, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

  // Small buffers — saves heap for SSL layer
  fbdo.setResponseSize(1024);
  fbdo.setBSSLBufferSize(4096, 2048);
  fbdo.keepAlive(30, 10, 3);


  Serial.print("[Firebase] Authenticating");
  int tries = 0;
  while (!Firebase.ready() && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (Firebase.ready()) {
    Serial.println("\n[Firebase] Connected!");
    fbConnected = true;
  } else {
    Serial.println("\n[Firebase] Auth timeout — continuing");
    fbConnected = false;
  }
}

// ─── Log event ────────────────────────────────────────────────
inline void logEvent(String event, String category, String details) {
  if (!Firebase.ready()) return;
  logCounter++;
  String path = "/logs/" + String(DEVICE_ID) + "/" + String(logCounter);
  FirebaseJson json;
  json.set("event",    event);
  json.set("category", category);
  json.set("details",  details);
  json.set("ts",       (int)(millis() / 1000));
  Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
}

// ─── MIT App update ───────────────────────────────────────────
inline void updateMitApp(String person, String status,
                         float confidence, String event) {
  if (!Firebase.ready()) return;
  String path = "/mit_app/" + String(DEVICE_ID) + "/latest";
  FirebaseJson json;
  json.set("person",     person);
  json.set("status",     status);
  json.set("confidence", (int)(confidence * 100));
  json.set("event",      event);
  json.set("timestamp",  (int)(millis() / 1000));
  Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
}

// ─── Device status ────────────────────────────────────────────
inline void updateDeviceStatus() {
  if (!Firebase.ready()) return;
  String path = "/devices/" + String(DEVICE_ID);
  FirebaseJson json;
  json.set("status",      "online");
  json.set("firmware",    FIRMWARE_VER);
  json.set("ip",          WiFi.localIP().toString());
  json.set("face_server", faceServerIp);
  json.set("last_seen",   (int)(millis() / 1000));
  json.set("arm_state",   stateToString());
  Firebase.RTDB.updateNode(&fbdo, path.c_str(), &json);
}

// ─── Heartbeat ────────────────────────────────────────────────
// Face server IP is stored in Firebase so IP changes do not need firmware upload.
inline void updateFaceServerIpFromFirebase() {
  if (!Firebase.ready()) return;

  String path = "/settings/" + String(DEVICE_ID) + "/face_server_ip";
  if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
    String ip = fbdo.stringData();
    ip.trim();
    if (ip.length() >= 7) {
      faceServerIp = ip;
      Serial.println("[FACE] Using server IP from Firebase: " + faceServerIp);
      return;
    }
  }

  Firebase.RTDB.setString(&fbdo, path.c_str(), faceServerIp);
  Serial.println("[FACE] Using default server IP: " + faceServerIp);
}

inline void sendHeartbeat() {
  if (!Firebase.ready()) return;
  String path = "/system_health/" + String(DEVICE_ID);
  FirebaseJson json;
  json.set("uptime_s",  (int)(millis() / 1000));
  json.set("free_heap", (int)ESP.getFreeHeap());
  json.set("state",     stateToString());
  json.set("wifi_rssi", (int)WiFi.RSSI());
  Firebase.RTDB.updateNode(&fbdo, path.c_str(), &json);
}

// ─── Command poll — uses fbdo (never shared concurrently) ─────
void handleCommand(String cmd);  // defined in command_handler.h

inline void pollFirebaseCommand() {
  if (!Firebase.ready()) {
    Serial.println("[CMD] Firebase not ready");
    return;
  }

  String path = "/commands/" + String(DEVICE_ID) + "/pending";

  if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
    String cmd = fbdo.stringData();
    Serial.println("[CMD] Got: '" + cmd + "'");
    if (cmd != "NONE" && cmd != "") {
      handleCommand(cmd);
    }
  } else {
    Serial.println("[CMD] Poll error: " + fbdo.errorReason());
  }
}
