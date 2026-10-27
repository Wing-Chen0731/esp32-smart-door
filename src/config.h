#pragma once

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

#ifndef SMARTDOOR_WIFI_SSID
#error "Define SMARTDOOR_WIFI_SSID in src/secrets.h"
#endif
#ifndef SMARTDOOR_WIFI_PASSWORD
#define SMARTDOOR_WIFI_PASSWORD ""
#endif
#ifndef SMARTDOOR_MQTT_BROKER
#error "Define SMARTDOOR_MQTT_BROKER in src/secrets.h"
#endif
#ifndef SMARTDOOR_MQTT_PORT
#define SMARTDOOR_MQTT_PORT 1883
#endif
#ifndef SMARTDOOR_MQTT_CLIENT_ID
#error "Define a unique SMARTDOOR_MQTT_CLIENT_ID in src/secrets.h"
#endif
#ifndef SMARTDOOR_TOPIC_PREFIX
#define SMARTDOOR_TOPIC_PREFIX "smartdoor"
#endif
