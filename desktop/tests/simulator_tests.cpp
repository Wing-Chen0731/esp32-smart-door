#include "simulator.hpp"

#include <iostream>
#include <string>

namespace {
int failures = 0;

void expect(bool condition, const char* label) {
    if (condition) std::cout << "[PASS] " << label << '\n';
    else { std::cerr << "[FAIL] " << label << '\n'; ++failures; }
}

bool has(const Simulator& simulator, const std::string& fragment) {
    return simulator.stateJson().find(fragment) != std::string::npos;
}
}

int main() {
    {
        Simulator sim;
        expect(has(sim, "\"topic\":\"smartdoor/status\",\"payload\":\"LOCKED,NORMAL,DOOR_CLOSED,RFID_OK\",\"qos\":0,\"retained\":true"),
               "retained status is published with QoS 0");
        expect(has(sim, "\"topic\":\"smartdoor/lwt\",\"payload\":\"online\",\"qos\":1,\"retained\":true"),
               "Last Will status uses QoS 1 and retain");
        sim.action("remote", "OPEN");
        expect(has(sim, "\"locked\":false"), "simulated remote OPEN unlocks the door");
        sim.action("tick", "3");
        expect(has(sim, "\"locked\":true"), "door auto-locks after its timeout");
    }
    {
        Simulator sim;
        sim.action("door-open");
        expect(has(sim, "\"mode\":\"TAMPER\""), "opening a locked door enters TAMPER");
        sim.action("remote", "CLEAR");
        expect(has(sim, "\"mode\":\"TAMPER\""), "TAMPER cannot clear while the sensor is open");
        sim.action("door-close");
        sim.action("remote", "CLEAR");
        expect(has(sim, "\"mode\":\"NORMAL\""), "TAMPER clears after the door is closed");
    }
    {
        Simulator sim;
        sim.action("rfid-fail");
        expect(has(sim, "RFID 健康检查失败 1/3") && has(sim, "\"mode\":\"NORMAL\""),
               "one RFID failure remains below the fault threshold");
        sim.action("rfid-fail");
        expect(has(sim, "RFID 健康检查失败 2/3") && has(sim, "\"mode\":\"NORMAL\""),
               "two RFID failures remain below the fault threshold");
        sim.action("rfid-fail");
        expect(has(sim, "\"mode\":\"FAULT\""), "repeated RFID failures enter FAULT");
        sim.action("button");
        expect(has(sim, "\"locked\":false"), "local exit remains available during RFID FAULT");
    }
    {
        Simulator sim;
        sim.action("wifi");
        sim.action("mqtt");
        expect(has(sim, "\"wifi\":false,\"mqtt\":false") && has(sim, "MQTT 客户端无法连接 Broker"),
               "MQTT cannot report connected while Wi-Fi is offline");
        sim.action("remote", "OPEN");
        expect(has(sim, "\"locked\":true"), "remote command is not accepted while offline");
        sim.action("button");
        expect(has(sim, "\"locked\":false"), "local exit remains available while offline");
    }
    {
        Simulator sim;
        sim.action("l2-bounce");
        expect(has(sim, "\"locked\":true") && has(sim, "\"bouncesFiltered\":1"),
               "GPIO bounce is filtered without opening the door");
        sim.action("l2-press");
        sim.action("l2-press");
        expect(has(sim, "\"locked\":false") && has(sim, "\"buttonPresses\":1"),
               "held exit button produces one debounced press edge");
        expect(has(sim, "\"redLed\":false,\"greenLed\":true"),
               "LED indicators follow the door state");
        sim.action("l2-release");
        sim.action("l2-tone", "alarm");
        expect(has(sim, "\"buzzerOn\":true,\"buzzerHz\":3200"),
               "PWM buzzer state exposes the selected tone frequency");
        sim.action("l2-tone", "mute");
        expect(has(sim, "\"buzzerOn\":false,\"buzzerHz\":0"),
               "PWM buzzer can be switched off");
    }
    {
        Simulator sim;
        sim.action("card", "01:02:03:04");
        expect(has(sim, "\"locked\":false") && has(sim, "[INFO] AUTH: Authorized card"),
               "authorized RFID UID opens the door with an INFO log");
        sim.action("reset");
        sim.action("card", "55:66:77:88");
        expect(has(sim, "\"locked\":true") && has(sim, "[WARN] AUTH: Unauthorized card"),
               "unauthorized RFID UID is rejected with a WARN log");
        sim.action("serial", "ADD 55:66:77:88");
        sim.action("serial", "LIST");
        expect(has(sim, "\"whitelist\":[\"01:02:03:04\",\"55:66:77:88\"]"),
               "UART ADD and LIST commands update and report the whitelist");
        sim.action("serial", "ADD 12:34");
        expect(has(sim, "[ERROR] UART: UID 格式无效") &&
                   has(sim, "\"whitelist\":[\"01:02:03:04\",\"55:66:77:88\"]"),
               "UART rejects malformed UIDs without changing the whitelist");
        sim.action("serial", "OPEN");
        expect(has(sim, "\"locked\":false"), "UART OPEN command unlocks the door");
        sim.action("serial", "STATUS");
        expect(has(sim, "UART: 状态=UNLOCKED"), "UART STATUS reports the current state");
        sim.action("serial", "CLOSE");
        expect(has(sim, "\"locked\":true"), "UART CLOSE command relocks the door");
        sim.action("serial", "NOPE");
        expect(has(sim, "[ERROR] UART: 未知指令"), "unknown UART command produces an ERROR log");
        sim.action("reset");
        expect(has(sim, "\"whitelist\":[\"01:02:03:04\"]"),
               "reset restores the default RFID whitelist");
    }
    {
        Simulator sim;
        sim.action("power-cut");
        expect(has(sim, "\"smartdoor/lwt\":\"offline\""), "power-loss scenario sets retained LWT offline");
    }
    {
        Simulator sim;
        sim.action("speed", "60");
        sim.action("tick", "1");
        sim.action("heap");
        sim.action("watchdog");
        sim.action("reset");
        const auto resetState = sim.stateJson();
        expect(has(sim, "\"time\":0,\"scale\":1,\"heapUsed\":18432,\"heapPeak\":18432"),
               "reset restores virtual time and teaching heap counters");
        expect(resetState.find("WATCHDOG_RESET_DEMO") == std::string::npos,
               "reset clears messages and logs from the previous experiment");
        sim.action("watchdog");
        expect(has(sim, "WATCHDOG_RESET_DEMO") && has(sim, "\"locked\":true"),
               "watchdog demo performs a safety reset into the locked state");
        expect(has(sim, "\"topic\":\"smartdoor/status\",\"payload\":\"LOCKED,NORMAL,DOOR_CLOSED,RFID_OK\""),
               "watchdog publishes the post-reset safe status");
        sim.action("sleep");
        expect(has(sim, "\"wifi\":false,\"mqtt\":false"),
               "deep-sleep demo disconnects Wi-Fi and MQTT");
        sim.action("sleep");
        expect(has(sim, "\"wifi\":true,\"mqtt\":true"),
               "wake demo reconnects Wi-Fi and MQTT");
    }
    {
        Simulator sim;
        sim.action("rfid-fail"); sim.action("rfid-fail"); sim.action("rfid-fail");
        sim.action("watchdog");
        expect(has(sim, "\"mode\":\"FAULT\"") && has(sim, "LOCKED,FAULT,DOOR_CLOSED,RFID_FAULT"),
               "watchdog keeps RFID fault visible until the reader is recovered");
        sim.action("door-open");
        expect(has(sim, "\"mode\":\"TAMPER\""),
               "door tamper condition takes priority over an existing RFID fault");
        sim.action("watchdog");
        expect(has(sim, "\"mode\":\"TAMPER\"") && has(sim, "LOCKED,TAMPER,DOOR_OPEN,RFID_FAULT"),
               "watchdog preserves the active door tamper condition");
    }
    if (failures) std::cerr << failures << " test(s) failed.\n";
    else std::cout << "All simulator checks passed.\n";
    return failures ? 1 : 0;
}
