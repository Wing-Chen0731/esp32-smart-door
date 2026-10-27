# Verification plan

## Automated checks available without hardware

Run `./desktop/test.sh`. The tests cover:

| Scenario | Expected result |
|---|---|
| GPIO: short contact bounce | Bounce is filtered; door remains locked |
| GPIO: held exit button | One debounced edge only; LEDs follow lock state; repeat press is ignored until release |
| PWM buzzer state | Tone frequency state changes; mute clears the output indicator |
| RFID: authorized / unauthorized UID | Authorized UID opens; unauthorized UID keeps lock and raises a warning/alarm |
| UART commands | `OPEN/CLOSE/STATUS/LIST` work; `ADD/REMOVE` update the model whitelist; malformed UIDs and unknown commands are rejected |
| Remote `OPEN` and timer advance | Unlocked, then automatically locked |
| Locked door sensor reports open | `TAMPER` and alarm path |
| Clear tamper with door open | Rejected |
| Close door then clear | Returns to `NORMAL` |
| Three RFID health failures | Enters `FAULT` |
| Exit button while RFID is faulted | Local egress remains available |
| Wi-Fi disconnect | Remote command is not accepted |
| Local button while offline | Local control still works |
| Simulated power loss | Retained LWT model changes to `offline` |

The browser page exposes one shared state model across GPIO, RFID/UART, Wi-Fi/MQTT, and reliability modules. Automated checks also cover the RFID failure threshold, the Wi-Fi/MQTT dependency, and watchdog recovery while preserving active fault conditions. A UI smoke test should verify that all four module tabs switch, controls call `/api/action`, the shared log updates, and reset clears counters and restores the default whitelist. All GPIO/PWM/SPI/UART behavior here is a desktop software model, not physical signal simulation.

GitHub Actions also compiles the desktop application and ESP32 firmware. Firmware CI does not emulate electronics or test a broker connection.

## Manual firmware integration checks

With an ESP32 board, supported RFID reader, and a private MQTT broker:

1. Confirm boot and safe locked output; disconnect the broker and confirm local exit control still responds.
2. Scan an authorized and unauthorized credential; inspect the log and corresponding topics.
3. Publish `OPEN`, `CLOSE`, and `STATUS` to the device's command topic; confirm allowed state transitions.
4. Disconnect network power without a graceful MQTT disconnect; confirm the broker publishes the configured LWT after keepalive timeout.
5. Restore network connectivity and confirm bounded reconnect, resubscription and status report.
6. Measure actual pin levels, lock driver current, thermal behavior, watchdog reset cause, deep-sleep current and recovery before making hardware reliability claims.

Record board revision, firmware commit, broker version/configuration, test setup, expected/observed result, and any deviations with each hardware run.
