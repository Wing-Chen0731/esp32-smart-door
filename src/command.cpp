#include "command.h"

CommandParser::CommandParser()
    : inputBuffer_(""), currentCommand_(""), commandReady_(false) {}

void CommandParser::loop() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputBuffer_.length() > 0) {
        currentCommand_ = inputBuffer_;
        inputBuffer_ = "";
        commandReady_ = true;
      }
    } else {
      inputBuffer_ += c;
    }
  }
}

bool CommandParser::available() const {
  return commandReady_;
}

String CommandParser::getCommand() const {
  return currentCommand_;
}

void CommandParser::clear() {
  currentCommand_ = "";
  commandReady_ = false;
}
