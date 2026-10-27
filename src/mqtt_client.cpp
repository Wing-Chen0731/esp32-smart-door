#include "mqtt_client.h"

MQTTClient* MQTTClient::instance_ = nullptr;

MQTTClient::MQTTClient(const char* brokerAddr, uint16_t brokerPort, const char* id)
    : mqttClient_(wifiClient_),
      broker_(brokerAddr),
      port_(brokerPort),
      clientId_(id),
      onCommand_(nullptr),
      lastReconnectAttempt_(0) {
  instance_ = this;
}

void MQTTClient::begin() {
  mqttClient_.setServer(broker_, port_);
  mqttClient_.setCallback(staticCallback);
  mqttClient_.setSocketTimeout(2);
  mqttClient_.setKeepAlive(15);
  connect();
}

void MQTTClient::loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (mqttClient_.connected()) {
      mqttClient_.disconnect();
    }
    return;
  }

  if (!mqttClient_.connected()) {
    const unsigned long now = millis();
    if (now - lastReconnectAttempt_ >= kReconnectIntervalMs) {
      lastReconnectAttempt_ = now;
      LOG_WARN("MQTT", "Disconnected. Attempting reconnect...");
      connect();
    }
    return;
  }

  mqttClient_.loop();
}

bool MQTTClient::connect() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (!mqttClient_.connect(clientId_, MQTT_TOPIC_LWT, 1, true, "offline")) {
    LOG_WARN("MQTT", String("Connect failed, rc=") + mqttClient_.state());
    return false;
  }

  LOG_INFO("MQTT", "Connected to broker");
  subscribe(MQTT_TOPIC_COMMAND, 1);
  publish(MQTT_TOPIC_LWT, "online", true);
  reportStatus("LOCKED");
  lastReconnectAttempt_ = millis();
  return true;
}

bool MQTTClient::isConnected() {
  return mqttClient_.connected();
}

bool MQTTClient::publish(const char* topic, const char* message, bool retained) {
  if (!mqttClient_.connected() || topic == nullptr || message == nullptr) {
    return false;
  }
  return mqttClient_.publish(topic, message, retained);
}

bool MQTTClient::subscribe(const char* topic, uint8_t qos) {
  const bool result = qos == 1 ? mqttClient_.subscribe(topic, 1)
                               : mqttClient_.subscribe(topic);
  if (result) {
    LOG_INFO("MQTT", String("Subscribed to: ") + topic);
  }
  return result;
}

void MQTTClient::onCommandReceived(CommandCallback callback) {
  onCommand_ = callback;
}

void MQTTClient::reportStatus(const char* status) {
  publish(MQTT_TOPIC_STATUS, status, true);
}

void MQTTClient::reportEvent(const String& event) {
  publish(MQTT_TOPIC_EVENT, event.c_str());
}

void MQTTClient::reportAlarm(const String& alarm) {
  publish(MQTT_TOPIC_ALARM, alarm.c_str());
}

void MQTTClient::staticCallback(char* topic, byte* payload, unsigned int length) {
  if (instance_ == nullptr || instance_->onCommand_ == nullptr) {
    return;
  }

  char message[64] = {0};
  const unsigned int copyLength = length < sizeof(message) - 1
                                      ? length
                                      : sizeof(message) - 1;
  memcpy(message, payload, copyLength);

  String command(message);
  command.trim();
  command.toUpperCase();
  LOG_DEBUG("MQTT", String("Received [") + topic + "]: " + command);

  if (String(topic) == MQTT_TOPIC_COMMAND) {
    instance_->onCommand_(command);
  }
}
