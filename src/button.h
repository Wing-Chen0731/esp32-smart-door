#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button {
 public:
  explicit Button(uint8_t pin, unsigned long debounceMs = 25);

  void begin();
  void update();
  bool wasJustPressed();

 private:
  uint8_t pin_;
  unsigned long debounceMs_;
  int stableState_;
  int lastReading_;
  unsigned long lastChangeMs_;
  bool justPressed_;
};

#endif
