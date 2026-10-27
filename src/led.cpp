#include "led.h"

Led::Led(uint8_t pin) : pin_(pin) {}

void Led::begin() {
  pinMode(pin_, OUTPUT);
  off();
}

void Led::on() {
  digitalWrite(pin_, HIGH);
}

void Led::off() {
  digitalWrite(pin_, LOW);
}

void Led::blink(uint16_t times, uint16_t intervalMs) {
  for (uint16_t i = 0; i < times; ++i) {
    on();
    delay(intervalMs);
    off();
    delay(intervalMs);
  }
}
