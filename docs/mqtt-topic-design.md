# MQTT 主题设计

固件可连接配置的 MQTT Broker。示例配置使用公共测试 Broker `test.mosquitto.org:1883`；它是共享的课堂服务，不适用于敏感数据或生产门禁。主题根路径由 `SMARTDOOR_TOPIC_PREFIX` 配置，本文以默认值 `smartdoor` 为例。

| 主题 | 方向 | 代码行为 | Retain | 示例 | 用途 |
|---|---|---|---|---|---|
| `smartdoor/status` | ESP32 → Broker | 发布 QoS 0 | 是 | `LOCKED` | 当前门状态和周期状态 |
| `smartdoor/event` | ESP32 → Broker | 发布 QoS 0 | 否 | `Door opened by authorized RFID` | 一次性开门、关门事件 |
| `smartdoor/alarm` | ESP32 → Broker | 发布 QoS 0 | 否 | `Unauthorized card: 55:66:77:88` | 非法卡、未知指令等告警 |
| `smartdoor/command` | Broker → ESP32 | 订阅 QoS 1 | 否 | `OPEN` / `CLOSE` / `STATUS` | 远程门禁控制 |
| `smartdoor/lwt` | Broker → 订阅者 | LWT QoS 1 | 是 | `online` / `offline` | 异常离线检测 |

状态和 LWT 使用 Retained，新的订阅者无需等待下一次上报即可获得最新在线状态。事件和告警是瞬时消息，不保留旧消息。

说明：本项目使用 PubSubClient。它的常规 `publish()` 接口发布 QoS 0；因此事件和告警当前也无法保证 QoS 1 到达。指令主题以 QoS 1 订阅，LWT 配置 QoS 1 + Retain。QoS 1 的订阅表示 Broker 到客户端的交付语义，不会自动提高设备发布消息的 QoS。需要可靠告警时，必须更换支持 QoS 1 发布的客户端库，并验证确认、重试、重复消息处理和断网缓存策略。

## 验证命令

1. 订阅 `smartdoor/status`。
2. 订阅 `smartdoor/event`、`smartdoor/alarm`、`smartdoor/lwt`。
3. 向 `smartdoor/command` 发布 `OPEN`，观察绿灯和状态消息。
4. 向 `smartdoor/command` 发布 `CLOSE`，观察红灯和状态消息。
5. 停止 Wokwi 仿真，等待 Broker 心跳超时，观察 `smartdoor/lwt` 的 `offline`。
