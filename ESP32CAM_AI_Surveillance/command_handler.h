#pragma once
#include "config.h"
#include "hardware.h"
#include "firebase_helper.h"
#include "motion_detect.h"

// ─── Clear pending command on Firebase ────────────────────────
inline void clearPendingCommand() {
  Firebase.RTDB.setString(&fbdo,
    ("/commands/" + String(DEVICE_ID) + "/pending").c_str(),
    "NONE");
}

// ─── Main Command Dispatcher ──────────────────────────────────
inline void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  Serial.println("[CMD] Received: " + cmd);

  // ── ARM ─────────────────────────────────────────────────────
  if (cmd == "ARM") {
    sysState   = STATE_ARMED;
    alarmMuted = false;
    setRedLED(false);
    setYellowLED(false);
    setAlarm(false);
    resetMotionBaseline();
    logEvent("arm", "command", "Armed via app/voice");
    updateMitApp("System", "Armed", 0.0, "arm");

  // ── DISARM ──────────────────────────────────────────────────
  } else if (cmd == "DISARM") {
    sysState   = STATE_DISARMED;
    alarmMuted = false;
    setRedLED(false);
    setYellowLED(false);
    setAlarm(false);
    logEvent("disarm", "command", "Disarmed via app/voice");
    updateMitApp("System", "Disarmed", 0.0, "disarm");

  // ── SILENT ──────────────────────────────────────────────────
  } else if (cmd == "SILENT") {
    alarmMuted = true;
    setAlarm(false);
    if (sysState == STATE_ALARM) sysState = STATE_ARMED;
    logEvent("silent", "command", "Alarm silenced");
    updateMitApp("System", "Silenced", 0.0, "silent");

  // ── ALARM ───────────────────────────────────────────────────
  } else if (cmd == "ALARM") {
    sysState      = STATE_ALARM;
    alarmMuted    = false;
    alarmNextBeep = millis();
    setRedLED(true);
    logEvent("alarm", "command", "Alarm triggered via app/voice");
    updateMitApp("System", "ALARM ACTIVE", 0.0, "alarm");

  // ── STATUS ──────────────────────────────────────────────────
  } else if (cmd == "STATUS") {
    String st = stateToString();
    updateMitApp("System", st, 0.0, "status");
    updateDeviceStatus();
    logEvent("status", "command", "Status: " + st);

  // ── LIGHTON ─────────────────────────────────────────────────
  } else if (cmd == "LIGHTON") {
    lightOn = true;
    setLight(true);
    logEvent("light_on", "command", "Light turned on");
    updateMitApp("System", "Light ON", 0.0, "light");

  // ── LIGHTOFF ────────────────────────────────────────────────
  } else if (cmd == "LIGHTOFF") {
    lightOn = false;
    setLight(false);
    logEvent("light_off", "command", "Light turned off");
    updateMitApp("System", "Light OFF", 0.0, "light");

  // ── RESETBASELINE — recalibrate motion reference ─────────────
  } else if (cmd == "RESETBASELINE") {
    resetMotionBaseline();
    logEvent("baseline_reset", "command", "Motion baseline reset");
    updateMitApp("System", "Baseline reset", 0.0, "baseline");

  // ── REBOOT ──────────────────────────────────────────────────
  } else if (cmd == "REFRESHIP") {
    updateFaceServerIpFromFirebase();
    logEvent("refresh_ip", "command", "Face server IP: " + faceServerIp);
    updateMitApp("System", "Face IP: " + faceServerIp, 0.0, "settings");

  } else if (cmd == "REBOOT") {
    logEvent("reboot", "command", "Reboot commanded");
    clearPendingCommand();
    delay(500);
    ESP.restart();

  // ── TEST ────────────────────────────────────────────────────
  } else if (cmd == "TEST") {
    Serial.println("[CMD] Test trigger — querying face recognition server...");
    queryFaceRecognitionServer();

  // ── NONE / empty — no action ─────────────────────────────────
  } else if (cmd == "NONE" || cmd == "") {
    return;

  // ── Unknown command ──────────────────────────────────────────
  } else {
    Serial.println("[CMD] Unknown: " + cmd);
  }

  // Clear pending command after handling
  clearPendingCommand();
}
