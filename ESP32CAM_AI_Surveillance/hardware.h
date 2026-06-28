#pragma once
#include "config.h"
#include "Arduino.h"

// ── Pin Setup ─────────────────────────────────────────────────
inline void initHardwarePins() {
  pinMode(PIR_PIN,        INPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN,    OUTPUT);
  pinMode(BUZZER_PIN,     OUTPUT);
  pinMode(WHITE_LED_PIN,  OUTPUT);

  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN,    LOW);
  digitalWrite(BUZZER_PIN,     LOW);
  digitalWrite(WHITE_LED_PIN,  LOW);
}

// ── Self-test: blink both LEDs 3× on boot ────────────────────
inline void selfTestBlink() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(100);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN,    HIGH);
    delay(100);
    digitalWrite(RED_LED_PIN,    LOW);
  }
}

// ── LED / Buzzer Controls ─────────────────────────────────────
inline void setYellowLED(bool on) {
  digitalWrite(YELLOW_LED_PIN, on ? HIGH : LOW);
}

inline void setRedLED(bool on) {
  digitalWrite(RED_LED_PIN, on ? HIGH : LOW);
}

inline void setLight(bool on) {
  digitalWrite(WHITE_LED_PIN, on ? HIGH : LOW);
}

inline void setAlarm(bool on) {
  if (!on) digitalWrite(BUZZER_PIN, LOW);
  // Beep pattern managed in loop()
}

// ── State string helper ───────────────────────────────────────
inline String stateToString() {
  switch (sysState) {
    case STATE_ARMED:    return "ARMED";
    case STATE_DISARMED: return "DISARMED";
    case STATE_ALARM:    return "ALARM";
    case STATE_SILENT:   return "SILENT";
    default:             return "UNKNOWN";
  }
}
