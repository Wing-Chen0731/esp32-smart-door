#ifndef LED_H
#define LED_H

#include <Arduino.h>

class Led {
 public:
  explicit Led(uint8_t pin);

  void begin();
  void on();
  void off();
  void blink(uint16_t times, uint16_t intervalMs);

 private:
  uint8_t pin_;
};

#endif
