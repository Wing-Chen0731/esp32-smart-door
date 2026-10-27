#!/usr/bin/env sh
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/desktop/build"
mkdir -p "$BUILD_DIR"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread \
  -I"$ROOT_DIR/desktop/include" \
  "$ROOT_DIR/desktop/src/main.cpp" "$ROOT_DIR/desktop/src/simulator.cpp" \
  -o "$BUILD_DIR/smartdoor-simulator"
printf 'Build successful: %s\n' "$BUILD_DIR/smartdoor-simulator"
