#!/bin/bash
set -eu

PACKAGE_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PACKAGE_DIR"

echo "智能门禁系统｜交互式验证平台"
echo "先运行桌面测试并编译，再打开本地实验台。"

if ! command -v c++ >/dev/null 2>&1; then
  echo
  echo "未找到 C++ 编译器。请在终端运行：xcode-select --install"
  echo "完成 Apple Command Line Tools 安装后，再双击本文件。"
  read -r -p "按回车关闭本窗口..." _
  exit 1
fi

sh ./desktop/start-class.sh
