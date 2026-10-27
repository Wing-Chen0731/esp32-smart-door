#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

sh ./desktop/test.sh
sh ./desktop/build.sh

BASE_URL="http://127.0.0.1:8765"
if curl -fsS "$BASE_URL/api/state" 2>/dev/null | grep -q '"mode"'; then
  echo "复用已运行的智能门禁实验台：$BASE_URL"
  open "$BASE_URL"
  exit 0
fi

SERVER_PID=""
stop_server() {
  if [ -n "$SERVER_PID" ]; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}
trap stop_server EXIT INT TERM

PORT=8765
READY=0
while [ "$PORT" -le 8860 ]; do
  URL="http://127.0.0.1:$PORT"
  ./desktop/build/smartdoor-simulator --port "$PORT" &
  SERVER_PID=$!
  attempt=0
  while [ "$attempt" -lt 50 ]; do
    if curl -fsS "$URL/api/state" 2>/dev/null | grep -q '"mode"'; then
      READY=1
      break
    fi
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
      wait "$SERVER_PID" 2>/dev/null || true
      break
    fi
    attempt=$((attempt + 1))
    sleep 0.1
  done
  if [ "$READY" -eq 1 ]; then break; fi
  if kill -0 "$SERVER_PID" 2>/dev/null; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  SERVER_PID=""
  PORT=$((PORT + 1))
done

if [ "$READY" -ne 1 ]; then
  echo "未能在 8765-8860 范围内启动服务，请检查端口或防火墙设置。" >&2
  exit 1
fi

echo "实验台已就绪：$URL"
open "$URL"
echo "验证平台正在运行。按 Ctrl+C 停止本地服务。"
wait "$SERVER_PID"
