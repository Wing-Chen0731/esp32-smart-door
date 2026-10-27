#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

#include "logger.h"

class WiFiManager {
 public:
  WiFiManager(const char* wifiSsid, const char* wifiPassword = "")
        : ssid_(wifiSsid),
        password_(wifiPassword),
        lastReconnectAttempt_(0),
        wasConnected_(false) {}

  bool begin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_, password_);
    lastReconnectAttempt_ = millis();
    LOG_INFO("WiFi", String("Connecting to ") + ssid_ + "...");
    return false;  // Connection proceeds asynchronously; never wait in setup().
  }

  void loop() {
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !wasConnected_) {
      wasConnected_ = true;
      LOG_INFO("WiFi", String("Connected! IP: ") + WiFi.localIP().toString());
      return;
    }
    if (!connected && wasConnected_) {
      wasConnected_ = false;
      LOG_WARN("WiFi", "Connection lost; local door control remains available");
    }
    if (connected) {
      return;
    }

    const unsigned long now = millis();
    if (now - lastReconnectAttempt_ < kReconnectIntervalMs) {
      return;
    }

    lastReconnectAttempt_ = now;
    LOG_WARN("WiFi", "Disconnected. Reconnecting...");
    WiFi.reconnect();
  }

  bool isConnected() const {
    return WiFi.status() == WL_CONNECTED;
  }

 private:
  static constexpr unsigned long kReconnectIntervalMs = 10000;

  const char* ssid_;
  const char* password_;
  unsigned long lastReconnectAttempt_;
  bool wasConnected_;
};

#endif
