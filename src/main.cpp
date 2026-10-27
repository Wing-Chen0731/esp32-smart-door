#include <Arduino.h>

#include "button.h"
#include "buzzer.h"
#include "command.h"
#include "config.h"
#include "led.h"
#include "logger.h"
#include "mqtt_client.h"
#include "rfid_reader.h"
#include "wifi_manager.h"

namespace {

constexpr uint8_t RED_LED_PIN = 2;
constexpr uint8_t GREEN_LED_PIN = 4;
constexpr uint8_t BUZZER_PIN = 12;
constexpr uint8_t EXIT_BUTTON_PIN = 15;
constexpr uint8_t RFID_SS_PIN = 5;
constexpr uint8_t RFID_RST_PIN = 21;

constexpr unsigned long DOOR_OPEN_MS = 3000;
constexpr unsigned long STATUS_REPORT_MS = 30000;
constexpr unsigned long SERIAL_HEARTBEAT_MS = 2000;

constexpr char WIFI_SSID[] = SMARTDOOR_WIFI_SSID;
constexpr char WIFI_PASSWORD[] = SMARTDOOR_WIFI_PASSWORD;
constexpr char MQTT_BROKER[] = SMARTDOOR_MQTT_BROKER;
constexpr uint16_t MQTT_PORT = SMARTDOOR_MQTT_PORT;
constexpr char MQTT_CLIENT_ID[] = SMARTDOOR_MQTT_CLIENT_ID;

enum DoorState { DOOR_LOCKED, DOOR_OPEN };

Led redLed(RED_LED_PIN);
Led greenLed(GREEN_LED_PIN);
Button exitButton(EXIT_BUTTON_PIN);
Buzzer buzzer(BUZZER_PIN);
RFIDReader rfid(RFID_SS_PIN, RFID_RST_PIN);
CommandParser commandParser;
WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);
MQTTClient mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);

DoorState doorState = DOOR_LOCKED;
unsigned long doorOpenedAt = 0;
unsigned long lastStatusReportAt = 0;
unsigned long lastSerialHeartbeatAt = 0;

const String whitelist[] = {
    "01:02:03:04",  // Wokwi Blue Card
    "11:22:33:44",
    "aa:bb:cc:dd",
};
constexpr size_t WHITELIST_SIZE = sizeof(whitelist) / sizeof(whitelist[0]);

const char* doorStateName() {
  return doorState == DOOR_LOCKED ? "LOCKED" : "UNLOCKED";
}

bool isAuthorized(String uid) {
  uid.toLowerCase();
  for (size_t i = 0; i < WHITELIST_SIZE; ++i) {
    if (whitelist[i] == uid) {
      return true;
    }
  }
  return false;
}

void lockDoor(bool playSound, const char* source = "system") {
  greenLed.off();
  redLed.on();
  doorState = DOOR_LOCKED;

  if (playSound) {
    buzzer.shortBeep();
  }

  LOG_INFO("DOOR", String("Locked by ") + source);
  mqtt.reportStatus("LOCKED");
}

void openDoor(const char* source) {
  redLed.off();
  greenLed.on();
  doorState = DOOR_OPEN;
  doorOpenedAt = millis();

  LOG_INFO("DOOR", String("Opened by ") + source);
  mqtt.reportStatus("UNLOCKED");
  mqtt.reportEvent(String("Door opened by ") + source);
  buzzer.openBeep();
}

void rejectUnauthorizedCard(const String& uid) {
  LOG_WARN("AUTH", String("Unauthorized card rejected: ") + uid);
  redLed.blink(3, 120);
  redLed.on();
  buzzer.alarmBeep();
  mqtt.reportAlarm(String("Unauthorized card: ") + uid);
}

void printWhitelist() {
  Serial.println("[INFO] CMD: Whitelist");
  for (size_t i = 0; i < WHITELIST_SIZE; ++i) {
    Serial.println(whitelist[i]);
  }
}

void processCommand(String command, const char* source, bool replyToMqtt) {
  command.trim();
  command.toUpperCase();

  if (command == CMD_OPEN) {
    if (doorState == DOOR_LOCKED) {
      LOG_INFO("CMD", String("OPEN received from ") + source);
      openDoor(source);
    } else {
      LOG_WARN("CMD", "Door already unlocked");
      if (replyToMqtt) {
        mqtt.reportEvent("OPEN ignored: door already unlocked");
      }
    }
    return;
  }

  if (command == CMD_CLOSE) {
    if (doorState == DOOR_OPEN) {
      LOG_INFO("CMD", String("CLOSE received from ") + source);
      lockDoor(true, source);
      mqtt.reportEvent(String("Door closed by ") + source);
    } else {
      LOG_WARN("CMD", "Door already locked");
      if (replyToMqtt) {
        mqtt.reportEvent("CLOSE ignored: door already locked");
      }
    }
    return;
  }

  if (command == CMD_STATUS) {
    Serial.print("[INFO] CMD: Door status = ");
    Serial.println(doorStateName());
    Serial.print("[INFO] CMD: WiFi = ");
    Serial.println(wifiManager.isConnected() ? "CONNECTED" : "OFFLINE");
    Serial.print("[INFO] CMD: MQTT = ");
    Serial.println(mqtt.isConnected() ? "CONNECTED" : "OFFLINE");
    Serial.print("[INFO] CMD: RFID = ");
    Serial.println(rfid.isReady() ? "READY" : "NOT READY");
    if (replyToMqtt) {
      mqtt.reportStatus(doorStateName());
    }
    return;
  }

  if (command == "LIST") {
    printWhitelist();
    return;
  }

  if (command.length() > 0) {
    LOG_WARN("CMD", String("Unknown command: ") + command);
    if (replyToMqtt) {
      mqtt.reportAlarm(String("Unknown command: ") + command);
    }
  }
}

void onMqttCommand(String command) {
  processCommand(command, "mqtt command", true);
}

void handleExitButton() {
  exitButton.update();
  if (!exitButton.wasJustPressed()) {
    return;
  }

  if (doorState == DOOR_LOCKED) {
    LOG_INFO("BUTTON", "Exit button pressed");
    openDoor("exit button");
  } else {
    LOG_WARN("BUTTON", "Door already unlocked, button ignored");
  }
}

void handleRFID() {
  rfid.loop();
  if (!rfid.available()) {
    return;
  }

  const String uid = rfid.getUIDString();
  LOG_DEBUG("RFID", String("Card UID: ") + uid);

  if (doorState == DOOR_OPEN) {
    LOG_WARN("AUTH", "Door already unlocked, card ignored");
  } else if (isAuthorized(uid)) {
    LOG_INFO("AUTH", String("Authorized card: ") + uid);
    openDoor("authorized RFID");
  } else {
    rejectUnauthorizedCard(uid);
  }
  rfid.clearCard();
}

void handleAutoLock() {
  if (doorState == DOOR_OPEN && millis() - doorOpenedAt >= DOOR_OPEN_MS) {
    LOG_INFO("TIMER", "Auto lock timeout");
    lockDoor(true, "timer");
  }
}

void handlePeriodicStatus() {
  if (millis() - lastStatusReportAt >= STATUS_REPORT_MS) {
    lastStatusReportAt = millis();
    mqtt.reportStatus(doorStateName());
  }
}

void handleSerialHeartbeat() {
  if (millis() - lastSerialHeartbeatAt >= SERIAL_HEARTBEAT_MS) {
    lastSerialHeartbeatAt = millis();
    Serial.println("[INFO] SERIAL: Command interface is running");
  }
}

void handleSerialCommand() {
  commandParser.loop();
  if (commandParser.available()) {
    processCommand(commandParser.getCommand(), "serial command", false);
    commandParser.clear();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  redLed.begin();
  greenLed.begin();
  buzzer.begin();
  exitButton.begin();
  lockDoor(false, "boot");

  LOG_INFO("MAIN", "Smart Door System v4.0 starting");
  LOG_INFO("MAIN", "Commands: OPEN, CLOSE, STATUS, LIST");

  rfid.begin();
  if (!rfid.isReady()) {
    LOG_WARN("RFID", "MFRC522 not ready; local button and network remain available");
  }

  wifiManager.begin();
  mqtt.onCommandReceived(onMqttCommand);
  mqtt.begin();
  LOG_INFO("MAIN", "System ready. Local + remote control enabled");
}

void loop() {
  // 网络故障时这些本地任务仍持续运行。
  handleSerialCommand();
  handleExitButton();
  handleRFID();
  handleAutoLock();

  wifiManager.loop();
  mqtt.loop();
  handlePeriodicStatus();
  handleSerialHeartbeat();
  delay(10);
}
