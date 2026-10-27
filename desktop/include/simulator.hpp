#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

struct Message {
    std::string topic;
    std::string payload;
    int qos = 0;
    bool retained = false;
    unsigned long long time = 0;
};

class Simulator {
public:
    Simulator();
    std::string stateJson() const;
    void action(const std::string& name, const std::string& value = "");

private:
    enum class Mode { Normal, Tamper, Fault };
    bool locked_ = true;
    bool doorOpen_ = false;
    bool wifi_ = true;
    bool mqtt_ = true;
    bool rfidHealthy_ = true;
    bool sleeping_ = false;
    bool exitButtonPressed_ = false;
    bool buzzerOn_ = false;
    int buzzerFrequencyHz_ = 0;
    int acceptedButtonPresses_ = 0;
    int filteredButtonBounces_ = 0;
    Mode mode_ = Mode::Normal;
    int badReads_ = 0;
    int timerScale_ = 1;
    unsigned long long virtualSeconds_ = 0;
    unsigned long long relockAt_ = 0;
    unsigned long long lastStatusAt_ = 0;
    unsigned long long simulatedHeapUsed_ = 18432;
    unsigned long long simulatedHeapPeak_ = 18432;
    std::deque<std::string> events_;
    std::vector<Message> messages_;
    std::vector<std::string> whitelist_ = {"01:02:03:04"};
    std::map<std::string, std::string> retained_;

    std::string modeName() const;
    std::string statusText() const;
    void log(const std::string& text);
    void event(const std::string& text, bool alarm = false);
    void publish(const std::string& topic, const std::string& payload, int qos = 0, bool retained = false);
    void handleCommand(const std::string& command);
    void handleSerialCommand(const std::string& command);
    void pressExitButton(bool holdUntilRelease);
    bool isAuthorized(const std::string& uid) const;
    void setBuzzer(const std::string& tone);
    void tick(int seconds);
    void setDoor(bool open);
    void openDoor(const std::string& source);
    void closeDoor(const std::string& source);
};

std::string jsonEscape(const std::string& value);
