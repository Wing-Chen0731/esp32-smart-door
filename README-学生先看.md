# 智能门禁系统｜快速开始指南

本项目无需 Wokwi、实体 ESP32、外网或 MQTT Broker 即可运行桌面状态模型。首次运行会在本机编译桌面程序，然后自动打开浏览器。

## macOS 启动

1. 将整个文件夹解压到本地磁盘，不要只拷贝其中某个文件。
2. 确认电脑已安装 Apple Command Line Tools（提供 `clang++`）。如果没有，在“终端”运行 `xcode-select --install` 并完成安装。
3. 双击文件夹里的 `start.command`。若 macOS 阻止首次打开：在 Finder 中对它点右键，选择“打开”，再确认一次。
4. 等待终端出现“测试通过”“Build successful”和实验台地址；浏览器会自动打开终端显示的本机地址。默认端口为 8765，如果被占用会依次尝试后续端口。
5. 使用期间不要关闭启动终端。完成后回到该终端按 `Control+C` 停止服务。

也可以在 VS Code 中打开本文件夹，按 `⌘⇧P`，运行 **Tasks: Run Task**，选择 **SmartDoor: Start local app (build + test + open)**。

## Windows 启动

1. 安装 MSYS2，并打开 **MSYS2 UCRT64** 终端。
2. 在该终端运行 `pacman -S --needed mingw-w64-ucrt-x86_64-gcc` 安装 C++ 编译器。
3. 将 `C:\msys64\ucrt64\bin` 添加到 Windows `PATH`，然后重新打开 VS Code。
4. 解压整个项目文件夹，在 VS Code 打开该文件夹。按 `Ctrl+Shift+P`，运行 **Tasks: Run Task**，选择 **SmartDoor: Start local app (build + test + open)**。
5. 也可以在 PowerShell 项目目录运行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\start-windows.ps1`。
6. 等待测试与构建成功、浏览器打开后，保持终端运行；结束时按 `Ctrl+C`。

如果 PowerShell 无法找到 `g++`，请确认 `C:\msys64\ucrt64\bin` 已加入 `PATH`，然后完全退出并重新打开 VS Code。

## 浏览器打不开时

- 等终端明确显示实验台地址后，复制完整地址到浏览器；不要自行假定端口号。
- 若显示“复用已运行的实验台”，说明本机已有服务；该任务会打开网页后结束，原服务仍需在启动它的终端按 `Control+C` 停止。
- 若出现编译错误，macOS 请确认已安装 Apple Command Line Tools；Windows 请确认 MSYS2 UCRT64 GCC 已安装且 `g++` 在 `PATH` 中，然后重新运行对应启动入口。
- 若端口被占用，先找到旧实验台的终端并停止它，再重新启动。

## 功能模块怎么使用

顶部提供 GPIO 与外设、RFID 与串口、Wi‑Fi 与 MQTT、故障与可靠性四个功能模块。可观察按钮防抖、LED/PWM 状态，测试虚拟 RFID UID，并向桌面模型发送 UART 风格指令。所有模块共享同一门锁状态和运行日志；开始新的测试场景前点击“重置实验”可恢复初始状态。

观察时按顺序看：

1. 顶部模式和门锁/门磁状态；
2. 实验日志中发生的先后过程；
3. 主题消息表中记录的 topic、payload 等。

如果预期现象没有出现，先检查当前模式、网络标签和实验日志，再重置重试。

## 本实验台能做什么、不能做什么

本地网页连接一个 C++ 状态模型，可检查门禁状态机、授权、自动落锁、TAMPER、RFID 故障恢复，以及 Wi‑Fi/MQTT 消息流程。日志和消息都由本机模型生成。

它不是 ESP32 芯片/电路仿真器：不会驱动真实 GPIO 或 RFID 模块，不会连接真实 Wi‑Fi、MQTT Broker，也不会测量真实堆内存、看门狗复位或深睡电流。LWT、QoS、堆计数、看门狗和深睡均为软件行为模型。

## 可选：ESP32 固件

`src/` 和 `platformio.ini` 包含独立的 ESP32 固件工程。它与浏览器桌面模型是两个独立构建目标。要编译固件，可用 VS Code + PlatformIO 打开本文件夹并构建 `esp32dev` 环境；首次构建需要网络下载平台和库。验证 GPIO、读卡器、Wi‑Fi、MQTT 和功耗仍需要相应实体硬件。

项目只包含公开示例配置 `src/secrets.example.h`。请勿把私人 Wi‑Fi 密码、Broker 凭据或其他秘密提交到 GitHub。
