#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
 public:
  explicit Buzzer(uint8_t pin);

  void begin();
  void beep(uint16_t frequency, uint16_t durationMs);
  void shortBeep();
  void openBeep();
  void alarmBeep();
  void mute();

 private:
  uint8_t pin_;
};

#endif
