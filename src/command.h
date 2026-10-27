#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>

class CommandParser {
 public:
  CommandParser();

  void loop();
  bool available() const;
  String getCommand() const;
  void clear();

 private:
  String inputBuffer_;
  String currentCommand_;
  bool commandReady_;
};

#endif
