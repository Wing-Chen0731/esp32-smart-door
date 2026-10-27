# 智能门禁系统｜交互式验证平台

本项目提供 macOS 和 Windows 两套启动入口。首次运行都会自动执行测试、构建 C++ 桌面程序并打开浏览器。压缩包包含源码与启动脚本，不包含系统编译器或平台专属可执行文件。

### macOS

需要 Apple Command Line Tools（提供 `clang++`）。如尚未安装，在“终端”运行 `xcode-select --install` 并完成安装。在 VS Code 打开项目后，运行 **SmartDoor: Start local app (build + test + open)**；该任务直接通过 shell 启动，不依赖脚本执行权限。

若要在 Finder 双击 `start.command`，从 GitHub 下载后先在项目根目录终端执行一次 `chmod +x start.command desktop/*.sh`。随后即可双击启动；若系统拦截，右键文件并选择“打开”。也可直接在终端运行 `sh ./start.command`。

### Windows

需要 MSYS2 UCRT64 GCC（提供 `g++`）。在 MSYS2 UCRT64 终端运行 `pacman -S --needed mingw-w64-ucrt-x86_64-gcc`，并将 `C:\msys64\ucrt64\bin` 加入 Windows `PATH`。重新打开 VS Code 后运行 **SmartDoor: Start local app (build + test + open)**；也可在 PowerShell 项目目录执行 `powershell -NoProfile -ExecutionPolicy Bypass -File .\start-windows.ps1`。

使用期间保持启动终端打开；结束时在该终端按 `Ctrl+C`。若需详细操作，请看[快速启动说明](docs/快速启动与使用说明.docx)、[最新学生操作说明](docs/课程实验台学生操作说明_更新版.docx)和[合并实操手册](docs/智能门禁课程实验台_快速启动与第二至第五课实操手册_更新版.docx)。

> 这是离线桌面状态模型，不是 ESP32/Wokwi 硬件仿真器。浏览器中的 MQTT、LWT、堆计数、看门狗和深睡是软件行为模型，不代表真实网络或硬件测量。`src/` 与 `platformio.ini` 是独立 ESP32 固件工程；实际硬件功能需要 ESP32 和对应外设验证。

系统按功能划分为四个模块：GPIO 输入/输出、防抖、LED 与 PWM 蜂鸣器；RFID UID 白名单、SPI 引脚关系、UART 命令与分级日志；Wi-Fi/MQTT 消息流程；故障注入与恢复。各模块共用同一状态和运行日志，点击“重置实验”可恢复初始状态。

注意：GPIO 电平、按钮抖动、PWM 音调、MFRC522/SPI、UART 命令均以模型状态和日志展示，不代表真实电路、声音、串口或 RFID 通信。

更多启动与排错步骤见 [快速开始指南](README-学生先看.md)。
