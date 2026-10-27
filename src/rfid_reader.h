#ifndef RFID_READER_H
#define RFID_READER_H

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

class RFIDReader {
 public:
  RFIDReader(uint8_t ssPin, uint8_t rstPin);

  void begin();
  void loop();
  bool available() const;
  String getUIDString() const;
  void clearCard();
  bool isReady() const;

 private:
  MFRC522 mfrc522_;
  String currentUID_;
  bool newCardPresent_;
  bool ready_;
};

#endif
