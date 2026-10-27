#include "buzzer.h"

Buzzer::Buzzer(uint8_t pin) : pin_(pin) {}

void Buzzer::begin() {
  pinMode(pin_, OUTPUT);
  mute();
}

void Buzzer::beep(uint16_t frequency, uint16_t durationMs) {
  tone(pin_, frequency);
  delay(durationMs);
  mute();
}

void Buzzer::shortBeep() {
  beep(1800, 100);
}

void Buzzer::openBeep() {
  beep(2200, 80);
  delay(80);
  beep(2200, 80);
}

void Buzzer::alarmBeep() {
  for (uint8_t i = 0; i < 3; ++i) {
    beep(2600, 100);
    delay(70);
  }
}

void Buzzer::mute() {
  noTone(pin_);
  digitalWrite(pin_, LOW);
}
