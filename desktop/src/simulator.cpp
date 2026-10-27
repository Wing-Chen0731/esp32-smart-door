#include "simulator.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
bool validUidFormat(const std::string& uid) {
    int bytes = 0;
    int digitsInByte = 0;
    for (unsigned char c : uid) {
        if (c == ':') {
            if (digitsInByte != 2) return false;
            ++bytes;
            digitsInByte = 0;
        } else if (std::isxdigit(c)) {
            if (++digitsInByte > 2) return false;
        } else return false;
    }
    if (digitsInByte != 2) return false;
    ++bytes;
    return bytes == 4 || bytes == 7;
}
}

Simulator::Simulator() {
    retained_["smartdoor/lwt"] = "online";
    log("智能门禁软件模型已启动；当前运行于桌面模式。");
    log("[INFO] GPIO 驱动模型就绪：LED、出门按钮与 PWM 蜂鸣器。");
    log("[INFO] RFID/SPI 与 UART 命令模型就绪；默认授权卡 01:02:03:04。");
    publish("smartdoor/lwt", "online", 1, true);
    publish("smartdoor/status", statusText(), 0, true);
    log("本地 MQTT 消息模型已就绪，主题前缀：smartdoor/。");
}

std::string Simulator::modeName() const {
    switch (mode_) {
        case Mode::Tamper: return "TAMPER";
        case Mode::Fault: return "FAULT";
        default: return "NORMAL";
    }
}

std::string Simulator::statusText() const {
    return std::string(locked_ ? "LOCKED" : "UNLOCKED") + "," + modeName() +
           (doorOpen_ ? ",DOOR_OPEN" : ",DOOR_CLOSED") +
           (rfidHealthy_ ? ",RFID_OK" : ",RFID_FAULT");
}

void Simulator::log(const std::string& text) {
    events_.push_front("[t+" + std::to_string(virtualSeconds_) + "s] " + text);
    if (events_.size() > 40) events_.pop_back();
}

void Simulator::publish(const std::string& topic, const std::string& payload, int qos, bool retained) {
    if (!wifi_ || !mqtt_) return;
    Message m{topic, payload, qos, retained, virtualSeconds_};
    messages_.insert(messages_.begin(), m);
    if (messages_.size() > 40) messages_.pop_back();
    if (retained) retained_[topic] = payload;
    log("MQTT → " + topic + " = " + payload + "  (QoS " + std::to_string(qos) +
        (retained ? ", retained" : "") + ")");
}

void Simulator::event(const std::string& text, bool alarm) {
    publish(alarm ? "smartdoor/alarm" : "smartdoor/event", text, 0);
    log(std::string(alarm ? "报警：" : "事件：") + text);
}

void Simulator::openDoor(const std::string& source) {
    if (mode_ == Mode::Tamper || (mode_ == Mode::Fault && source != "EXIT_BUTTON")) {
        log("拒绝开门：系统状态为 " + modeName() + "，需先恢复正常。");
        event("OPEN_REJECTED_" + modeName(), mode_ == Mode::Tamper);
        return;
    }
    if (!locked_) { log("门已经解锁。"); return; }
    locked_ = false;
    relockAt_ = virtualSeconds_ + 3;
    setBuzzer("success");
    log("开门成功（" + source + "）；3 个虚拟秒后自动落锁。");
    event("DOOR_OPENED_BY_" + source);
    publish("smartdoor/status", statusText(), 0, true);
}

void Simulator::closeDoor(const std::string& source) {
    if (mode_ == Mode::Tamper) {
        log("TAMPER 状态下拒绝 CLOSE；门磁恢复关闭后发送 CLEAR。");
        return;
    }
    locked_ = true;
    relockAt_ = 0;
    setBuzzer("lock");
    log("门已落锁（" + source + "）。");
    publish("smartdoor/status", statusText(), 0, true);
}

void Simulator::setDoor(bool open) {
    if (doorOpen_ == open) { log(std::string("门磁当前已经是 ") + (open ? "OPEN" : "CLOSED") + "。"); return; }
    doorOpen_ = open;
    log(std::string("门磁变化：") + (open ? "门打开" : "门关闭") + "。");
    event(open ? "DOOR_SENSOR_OPEN" : "DOOR_SENSOR_CLOSED");
    if (open && locked_ && mode_ != Mode::Tamper) {
        mode_ = Mode::Tamper;
        log("防拆触发：门已锁定但门磁检测到开门，系统进入 TAMPER。");
        event("TAMPER_LOCKED_DOOR_OPEN", true);
    }
    publish("smartdoor/status", statusText(), 0, true);
}

void Simulator::handleCommand(const std::string& raw) {
    std::string c = raw;
    std::transform(c.begin(), c.end(), c.begin(), [](unsigned char x) { return static_cast<char>(std::toupper(x)); });
    if (c == "OPEN") openDoor("MQTT");
    else if (c == "CLOSE") closeDoor("MQTT");
    else if (c == "STATUS") publish("smartdoor/status", statusText(), 0, true);
    else if (c == "CLEAR") {
        if (mode_ == Mode::Tamper && !doorOpen_) {
            mode_ = rfidHealthy_ ? Mode::Normal : Mode::Fault;
            log("TAMPER 已清除，当前状态：" + modeName() + "。");
            event("TAMPER_CLEARED");
            publish("smartdoor/status", statusText(), 0, true);
        } else log("CLEAR 被拒绝：只有门磁已关闭时才能清除 TAMPER。");
    } else log("未知 MQTT 指令：" + raw + "（支持 OPEN/CLOSE/STATUS/CLEAR）。");
}

bool Simulator::isAuthorized(const std::string& rawUid) const {
    std::string uid = rawUid;
    std::transform(uid.begin(), uid.end(), uid.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return std::find(whitelist_.begin(), whitelist_.end(), uid) != whitelist_.end();
}

void Simulator::handleSerialCommand(const std::string& raw) {
    std::istringstream input(raw);
    std::string command;
    input >> command;
    std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    if (command.empty()) return;

    log("UART RX: " + raw);
    if (command == "OPEN") {
        log("[INFO] UART: 执行 OPEN。");
        openDoor("UART");
    } else if (command == "CLOSE") {
        closeDoor("UART");
    } else if (command == "STATUS") {
        log("[INFO] UART: 状态=" + statusText() + "。");
    } else if (command == "LIST") {
        log("[INFO] UART: 白名单共 " + std::to_string(whitelist_.size()) + " 张卡。");
        for (const auto& uid : whitelist_) log("  " + uid);
    } else if (command == "ADD" || command == "REMOVE") {
        std::string uid;
        input >> uid;
        std::transform(uid.begin(), uid.end(), uid.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (!validUidFormat(uid)) {
            log("[ERROR] UART: UID 格式无效；使用 4 字节或 7 字节十六进制，如 01:02:03:04。");
        } else if (command == "ADD") {
            if (std::find(whitelist_.begin(), whitelist_.end(), uid) != whitelist_.end()) {
                log("[WARN] AUTH: UID 已在白名单中：" + uid);
            } else {
                whitelist_.push_back(uid);
                log("[INFO] AUTH: 已添加授权 UID：" + uid);
            }
        } else {
            const auto found = std::find(whitelist_.begin(), whitelist_.end(), uid);
            if (found == whitelist_.end()) log("[WARN] AUTH: UID 不在白名单中：" + uid);
            else {
                whitelist_.erase(found);
                log("[INFO] AUTH: 已移除授权 UID：" + uid);
            }
        }
    } else {
        log("[ERROR] UART: 未知指令“" + raw + "”；支持 OPEN/CLOSE/STATUS/LIST/ADD/REMOVE。");
    }
}

void Simulator::pressExitButton(bool holdUntilRelease) {
    if (exitButtonPressed_) {
        log("[DEBUG] BUTTON: 按钮仍处于按下状态；边沿检测忽略重复触发。");
        return;
    }
    exitButtonPressed_ = true;
    log("[DEBUG] GPIO15 原始电平 LOW；启动 50 ms 软件防抖确认。");
    log("[INFO] BUTTON: 电平稳定 50 ms，确认一次有效的按下沿。");
    ++acceptedButtonPresses_;
    if (sleeping_) {
        sleeping_ = false;
        log("本地按钮唤醒模拟设备；真实深睡状态下 Wi-Fi/MQTT 不会保持连接。");
    }
    if (locked_) openDoor("EXIT_BUTTON"); else closeDoor("EXIT_BUTTON");
    if (!holdUntilRelease) {
        exitButtonPressed_ = false;
        log("[DEBUG] GPIO15 原始电平 HIGH；按钮释放，等待下一次下降沿。");
    }
}

void Simulator::setBuzzer(const std::string& tone) {
    if (tone == "mute") {
        buzzerOn_ = false;
        buzzerFrequencyHz_ = 0;
        log("[INFO] BUZZER: PWM 已关闭（静音）；桌面模型只显示频率状态，不播放真实声音。");
        return;
    }
    buzzerOn_ = true;
    buzzerFrequencyHz_ = tone == "alarm" ? 3200 : tone == "lock" ? 1600 : 2400;
    log("[INFO] BUZZER: LEDC PWM 输出 " + std::to_string(buzzerFrequencyHz_) +
        " Hz 提示音；此处为逻辑指示，不产生真实声音。");
}

void Simulator::tick(int seconds) {
    seconds = std::max(1, std::min(seconds, 3600));
    virtualSeconds_ += static_cast<unsigned long long>(seconds) * static_cast<unsigned long long>(timerScale_);
    if (!locked_ && relockAt_ && virtualSeconds_ >= relockAt_) closeDoor("自动落锁");
    if (virtualSeconds_ - lastStatusAt_ >= 10) {
        lastStatusAt_ = virtualSeconds_;
        publish("smartdoor/status", statusText(), 0, true);
    }
    log("虚拟时间推进 " + std::to_string(seconds * timerScale_) + " 秒。");
}

void Simulator::action(const std::string& name, const std::string& value) {
    if (name == "button") {
        pressExitButton(false);
    } else if (name == "l2-press") {
        pressExitButton(true);
    } else if (name == "l2-release") {
        if (!exitButtonPressed_) log("按钮已处于释放状态。");
        else {
            exitButtonPressed_ = false;
            log("[DEBUG] GPIO15 原始电平 HIGH；稳定释放后重新允许下一次按下沿。");
        }
    } else if (name == "l2-bounce") {
        ++filteredButtonBounces_;
        log("[DEBUG] GPIO15 抖动序列：LOW → HIGH → LOW → HIGH（小于 50 ms）。");
        log("[INFO] BUTTON: 防抖窗口过滤了触点抖动；没有产生开门动作。");
    } else if (name == "l2-tone") {
        setBuzzer(value);
    } else if (name == "card") {
        if (!rfidHealthy_) { log("RFID 读卡器处于 FAULT，刷卡不可用；本地出门按钮仍可工作。"); return; }
        std::string uid = value;
        std::transform(uid.begin(), uid.end(), uid.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        log("[DEBUG] RFID: MFRC522 经 SPI 读取 UID=" + uid + "；SDA 引脚在本方案中作 CS/SS。");
        if (isAuthorized(uid)) {
            log("[INFO] AUTH: Authorized card " + uid + ".");
            openDoor("RFID");
        } else {
            log("[WARN] AUTH: Unauthorized card " + uid + "; door remains locked.");
            setBuzzer("alarm");
            event("ACCESS_DENIED_UID_" + uid, true);
        }
    } else if (name == "door-open") setDoor(true);
    else if (name == "door-close") setDoor(false);
    else if (name == "remote") {
        if (!wifi_ || !mqtt_) { log("远程指令未送达：Wi-Fi 或 MQTT 已断开；本地按钮仍可用。"); return; }
        log("订阅 smartdoor/command 收到：" + value);
        handleCommand(value);
    } else if (name == "serial") {
        handleSerialCommand(value);
    } else if (name == "wifi") {
        wifi_ = !wifi_;
        log(std::string("模拟 Wi-Fi ") + (wifi_ ? "已连接；MQTT 自动恢复。" : "断开；进入非阻塞等待重连。"));
        if (!wifi_) mqtt_ = false;
        else { mqtt_ = true; publish("smartdoor/lwt", "online", 1, true); publish("smartdoor/status", statusText(), 0, true); }
    } else if (name == "mqtt") {
        if (!wifi_) {
            mqtt_ = false;
            log("Wi-Fi 已断开，MQTT 客户端无法连接 Broker。");
            return;
        }
        mqtt_ = !mqtt_;
        log(std::string("模拟 MQTT Broker ") + (mqtt_ ? "可用，客户端重新连接并订阅指令。" : "不可用；设备进入非阻塞重连。"));
        if (mqtt_ && wifi_) { publish("smartdoor/lwt", "online", 1, true); publish("smartdoor/status", statusText(), 0, true); }
    } else if (name == "power-cut") {
        if (wifi_ && mqtt_) publish("smartdoor/lwt", "offline", 1, true);
        wifi_ = mqtt_ = false;
        log("模拟异常断电：清洁断开流程未执行；本地 Broker 根据遗嘱设置 offline。");
    } else if (name == "rfid-fail") {
        ++badReads_;
        log("RFID 健康检查失败 " + std::to_string(badReads_) + "/3；正在模拟重试。");
        if (badReads_ >= 3) {
            rfidHealthy_ = false;
            mode_ = mode_ == Mode::Tamper ? Mode::Tamper : Mode::Fault;
            event("RFID_READER_FAULT_LOCAL_BUTTON_AVAILABLE", true);
            publish("smartdoor/status", statusText(), 0, true);
        }
    } else if (name == "rfid-recover") {
        rfidHealthy_ = true; badReads_ = 0;
        if (mode_ == Mode::Fault) mode_ = Mode::Normal;
        log("RFID 自检恢复；系统状态：" + modeName() + "。");
        event("RFID_RECOVERED");
        publish("smartdoor/status", statusText(), 0, true);
    } else if (name == "clear") handleCommand("CLEAR");
    else if (name == "tick") tick(value.empty() ? 1 : std::stoi(value));
    else if (name == "speed") {
        int speed = std::stoi(value);
        timerScale_ = speed == 60 || speed == 3600 ? speed : 1;
        log("演示时间倍率设为 " + std::to_string(timerScale_) + "x；这不是等时长的真实稳定性测试。");
    } else if (name == "sleep") {
        sleeping_ = !sleeping_;
        if (sleeping_) { wifi_ = mqtt_ = false; log("模拟深睡：Wi-Fi/MQTT 连接关闭；这是软件状态展示，不是真实硬件功耗测量。"); }
        else { log("模拟唤醒；Wi-Fi/MQTT 重新连接。"); wifi_ = mqtt_ = true; publish("smartdoor/lwt", "online", 1, true); }
    } else if (name == "watchdog") {
        log("看门狗演示：模拟主循环卡死并等待监督超时。");
        event("WATCHDOG_RESET_DEMO", true);
        locked_ = true;
        relockAt_ = 0;
        mode_ = doorOpen_ ? Mode::Tamper : (rfidHealthy_ ? Mode::Normal : Mode::Fault);
        log("模拟复位原因：TASK_WATCHDOG；设备恢复到安全锁定状态，当前模式为 " + modeName() + "。");
        publish("smartdoor/status", statusText(), 0, true);
    } else if (name == "heap") {
        simulatedHeapUsed_ += 512;
        simulatedHeapPeak_ = std::max(simulatedHeapPeak_, simulatedHeapUsed_);
        log("模拟固件堆占用 +512 B（" + std::to_string(simulatedHeapUsed_) + " B）；此指标不是电脑进程内存。");
    } else if (name == "reset") {
        locked_ = true; doorOpen_ = false; mode_ = Mode::Normal; badReads_ = 0;
        exitButtonPressed_ = false; buzzerOn_ = false; buzzerFrequencyHz_ = 0;
        acceptedButtonPresses_ = 0; filteredButtonBounces_ = 0;
        whitelist_ = {"01:02:03:04"};
        rfidHealthy_ = true; sleeping_ = false; wifi_ = mqtt_ = true;
        timerScale_ = 1; virtualSeconds_ = 0; relockAt_ = 0; lastStatusAt_ = 0;
        simulatedHeapUsed_ = 18432; simulatedHeapPeak_ = 18432;
        events_.clear(); messages_.clear();
        retained_["smartdoor/lwt"] = "online";
        log("实验已复位：状态、虚拟时间、资源计数和本轮日志已清空。");
        publish("smartdoor/lwt", "online", 1, true); publish("smartdoor/status", statusText(), 0, true);
    }
}

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: if (c < 0x20) out << ' '; else out << static_cast<char>(c);
        }
    }
    return out.str();
}

std::string Simulator::stateJson() const {
    std::ostringstream out;
    out << "{\"locked\":" << (locked_ ? "true" : "false")
        << ",\"doorOpen\":" << (doorOpen_ ? "true" : "false")
        << ",\"wifi\":" << (wifi_ ? "true" : "false")
        << ",\"mqtt\":" << (mqtt_ ? "true" : "false")
        << ",\"rfid\":" << (rfidHealthy_ ? "true" : "false")
        << ",\"sleeping\":" << (sleeping_ ? "true" : "false")
        << ",\"buttonPressed\":" << (exitButtonPressed_ ? "true" : "false")
        << ",\"buttonLevel\":\"" << (exitButtonPressed_ ? "LOW" : "HIGH") << "\""
        << ",\"redLed\":" << (locked_ ? "true" : "false")
        << ",\"greenLed\":" << (!locked_ ? "true" : "false")
        << ",\"buzzerOn\":" << (buzzerOn_ ? "true" : "false")
        << ",\"buzzerHz\":" << buzzerFrequencyHz_
        << ",\"buttonPresses\":" << acceptedButtonPresses_
        << ",\"bouncesFiltered\":" << filteredButtonBounces_
        << ",\"mode\":\"" << modeName() << "\",\"time\":" << virtualSeconds_
        << ",\"scale\":" << timerScale_ << ",\"heapUsed\":" << simulatedHeapUsed_
        << ",\"heapPeak\":" << simulatedHeapPeak_ << ",\"retained\":{\"smartdoor/lwt\":\""
        << jsonEscape(retained_.at("smartdoor/lwt")) << "\"},\"whitelist\":[";
    bool firstUid = true;
    for (const auto& uid : whitelist_) {
        if (!firstUid) out << ',';
        firstUid = false;
        out << '"' << jsonEscape(uid) << '"';
    }
    out << "],\"events\":[";
    bool first = true;
    for (const auto& e : events_) { if (!first) out << ','; first = false; out << '"' << jsonEscape(e) << '"'; }
    out << "],\"messages\":[";
    first = true;
    for (const auto& m : messages_) {
        if (!first) out << ','; first = false;
        out << "{\"topic\":\"" << jsonEscape(m.topic) << "\",\"payload\":\"" << jsonEscape(m.payload)
            << "\",\"qos\":" << m.qos << ",\"retained\":" << (m.retained ? "true" : "false")
            << ",\"time\":" << m.time << '}';
    }
    out << "]}";
    return out.str();
}
