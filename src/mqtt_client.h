#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "logger.h"

#define MQTT_TOPIC_STATUS SMARTDOOR_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_EVENT SMARTDOOR_TOPIC_PREFIX "/event"
#define MQTT_TOPIC_COMMAND SMARTDOOR_TOPIC_PREFIX "/command"
#define MQTT_TOPIC_ALARM SMARTDOOR_TOPIC_PREFIX "/alarm"
#define MQTT_TOPIC_LWT SMARTDOOR_TOPIC_PREFIX "/lwt"

#define CMD_OPEN "OPEN"
#define CMD_CLOSE "CLOSE"
#define CMD_STATUS "STATUS"

using CommandCallback = void (*)(String command);

class MQTTClient {
 public:
  MQTTClient(const char* brokerAddr, uint16_t brokerPort, const char* id);

  void begin();
  void loop();
    bool isConnected();
  bool connect();

  bool publish(const char* topic, const char* message, bool retained = false);
  bool subscribe(const char* topic, uint8_t qos = 1);

  void onCommandReceived(CommandCallback callback);
  void reportStatus(const char* status);
  void reportEvent(const String& event);
  void reportAlarm(const String& alarm);

 private:
  static constexpr unsigned long kReconnectIntervalMs = 5000;

  static void staticCallback(char* topic, byte* payload, unsigned int length);

  WiFiClient wifiClient_;
  PubSubClient mqttClient_;
  const char* broker_;
  uint16_t port_;
  const char* clientId_;
  CommandCallback onCommand_;
  unsigned long lastReconnectAttempt_;

  static MQTTClient* instance_;
};

#endif
