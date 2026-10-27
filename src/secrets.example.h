#pragma once

// Copy to secrets.h for local development and replace values as needed.
// Wokwi's guest Wi-Fi is for simulation only. Never put real credentials here
// if secrets.h will be committed to a public repository.
#define SMARTDOOR_WIFI_SSID "Wokwi-GUEST"
#define SMARTDOOR_WIFI_PASSWORD ""

// Public broker is only for short classroom experiments. Prefer a private,
// authenticated broker for any deployment or shared demonstration.
#define SMARTDOOR_MQTT_BROKER "test.mosquitto.org"
#define SMARTDOOR_MQTT_PORT 1883
#define SMARTDOOR_MQTT_CLIENT_ID "smartdoor-demo-change-me"
#define SMARTDOOR_TOPIC_PREFIX "smartdoor"
