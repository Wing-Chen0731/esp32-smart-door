#include "rfid_reader.h"

#include "logger.h"

RFIDReader::RFIDReader(uint8_t ssPin, uint8_t rstPin)
    : mfrc522_(ssPin, rstPin),
      currentUID_(""),
      newCardPresent_(false),
      ready_(false) {}

void RFIDReader::begin() {
  SPI.begin(18, 19, 23, 5);
  mfrc522_.PCD_Init();

  const byte version = mfrc522_.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    ready_ = false;
    LOG_ERROR("RFID", "MFRC522 not found. Check SPI wiring.");
    return;
  }

  ready_ = true;
  Serial.print("[INFO] RFID: MFRC522 initialized. Version 0x");
  Serial.println(version, HEX);
}

void RFIDReader::loop() {
  if (!ready_ || !mfrc522_.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522_.PICC_ReadCardSerial()) {
    return;
  }

  currentUID_ = "";
  for (byte i = 0; i < mfrc522_.uid.size; ++i) {
    if (i > 0) {
      currentUID_ += ":";
    }
    if (mfrc522_.uid.uidByte[i] < 0x10) {
      currentUID_ += "0";
    }
    currentUID_ += String(mfrc522_.uid.uidByte[i], HEX);
  }
  currentUID_.toLowerCase();
  newCardPresent_ = true;

  mfrc522_.PICC_HaltA();
  mfrc522_.PCD_StopCrypto1();
}

bool RFIDReader::available() const {
  return newCardPresent_;
}

String RFIDReader::getUIDString() const {
  return currentUID_;
}

void RFIDReader::clearCard() {
  newCardPresent_ = false;
  currentUID_ = "";
}

bool RFIDReader::isReady() const {
  return ready_;
}
