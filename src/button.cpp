#include "button.h"

Button::Button(uint8_t pin, unsigned long debounceMs)
    : pin_(pin),
      debounceMs_(debounceMs),
      stableState_(HIGH),
      lastReading_(HIGH),
      lastChangeMs_(0),
      justPressed_(false) {}

void Button::begin() {
  pinMode(pin_, INPUT_PULLUP);
  stableState_ = digitalRead(pin_);
  lastReading_ = stableState_;
  lastChangeMs_ = millis();
}

void Button::update() {
  justPressed_ = false;
  const int reading = digitalRead(pin_);

  if (reading != lastReading_) {
    lastReading_ = reading;
    lastChangeMs_ = millis();
  }

  if (millis() - lastChangeMs_ >= debounceMs_ && reading != stableState_) {
    stableState_ = reading;
    if (stableState_ == LOW) {
      justPressed_ = true;
    }
  }
}

bool Button::wasJustPressed() {
  const bool pressed = justPressed_;
  justPressed_ = false;
  return pressed;
}
