# Security and limitations

This is a learning/portfolio project, not a production access-control system.

## Credentials and broker

- Copy `src/secrets.example.h` to the ignored `src/secrets.h` for local settings. Never commit real Wi-Fi passwords, broker credentials, certificates or private keys.
- The example uses Wokwi guest Wi-Fi and the public `test.mosquitto.org` broker for short experiments. The broker is shared, unauthenticated and unencrypted on port 1883. Do not send sensitive data or rely on it for access control.
- Change the MQTT client ID and topic namespace when several students use the public broker. For a real deployment, use a private broker, unique credentials, TLS certificate validation, per-device ACLs and a provisioning/rotation plan.
- Do not expose the desktop dashboard server to a LAN. It binds only to `127.0.0.1` and has no authentication; the firmware MQTT interface also has no command authorization beyond parsing known command strings.

## Device safety

- A four-byte RFID UID is not cryptographic authentication and can be cloned. Use a secure credential and backend policy for real access-control products.
- A software state named `LOCKED` is not proof that a physical lock engaged. Production hardware needs a suitable driver, current/thermal protection, door/bolt feedback, safe egress, and failure analysis.
- The desktop dashboard can simulate tamper, RFID failure, watchdog recovery, retained state and an LWT scenario. It cannot verify GPIO timing, electrical noise, brownouts, real watchdog reset causes, deep-sleep current, RF coverage or long-duration memory stability.
- The dashboard's QoS and LWT entries are teaching-model labels. Only the ESP32 firmware connected to a real broker exercises actual MQTT transport, and even that does not make the public demo broker production-safe.
- Hardware values in course materials (resistors, capacitors, pull-ups, wiring and power) must be checked against the selected components' datasheets and measurements.

## Known engineering gaps

1. Desktop and firmware currently contain separate state implementations; they can drift. Extracting a shared, allocation-light state machine is a planned improvement.
2. Firmware alarm/event publishes use QoS 0 because PubSubClient's standard `publish()` API does not send QoS 1. Use a library with QoS 1 publish support or add an explicitly tested protocol layer before claiming reliable alarm delivery.
3. The sample command protocol is plain text and has no authenticated sender, replay protection or role model.
4. Firmware validation in CI is a compile check. GPIO timing and reader behavior still need a physical test plan when hardware becomes available.
