#pragma once
#include "config.h"
#include "esp_camera.h"
#include "WiFi.h"

// ─── Camera Init ──────────────────────────────────────────────
inline void initCamera() {
  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0       = Y2_GPIO_NUM;
  cfg.pin_d1       = Y3_GPIO_NUM;
  cfg.pin_d2       = Y4_GPIO_NUM;
  cfg.pin_d3       = Y5_GPIO_NUM;
  cfg.pin_d4       = Y6_GPIO_NUM;
  cfg.pin_d5       = Y7_GPIO_NUM;
  cfg.pin_d6       = Y8_GPIO_NUM;
  cfg.pin_d7       = Y9_GPIO_NUM;
  cfg.pin_xclk     = XCLK_GPIO_NUM;
  cfg.pin_pclk     = PCLK_GPIO_NUM;
  cfg.pin_vsync    = VSYNC_GPIO_NUM;
  cfg.pin_href     = HREF_GPIO_NUM;
  cfg.pin_sscb_sda = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn     = PWDN_GPIO_NUM;
  cfg.pin_reset    = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_CIF;   // 400x296 - clearer faces, still lighter than VGA
    cfg.jpeg_quality = 10;              // lower value = better JPEG detail
    cfg.fb_count     = 1;               // 1 buffer only — critical for SSL heap
    Serial.println("[CAM] PSRAM found - CIF mode (face-recognition tuned)");
  } else {
    cfg.frame_size   = FRAMESIZE_QQVGA; // 160×120
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
    Serial.println("[CAM] No PSRAM - QQVGA mode");
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("[CAM] Init FAILED: 0x%x — rebooting\n", err);
    delay(1000);
    ESP.restart();
  }

  // Image quality tweaks for clearer face-recognition captures
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s,  2);
    s->set_contrast(s,    2);
    s->set_saturation(s,  0);
    s->set_whitebal(s,    1);
    s->set_awb_gain(s,    1);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s,        1);
    s->set_gainceiling(s, (gainceiling_t)4);
  }

  Serial.println("[CAM] Camera initialized OK");
}

// ─── WiFi Init ────────────────────────────────────────────────
inline void initWiFi() {
  Serial.printf("[WiFi] Connecting to: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempts > 40) {
      Serial.println("\n[WiFi] Timeout — rebooting");
      ESP.restart();
    }
  }
  Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}
