#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

API_PORT="${API_PORT:-8079}"
WEB_PORT="${WEB_PORT:-8080}"
TCP_PORT="${TCP_PORT:-2025}"
RUN_DIR="$SCRIPT_DIR/.run"

echo "=============================="
echo "启动药品仓储监测系统"
echo "=============================="

if [[ ! -f ".venv/bin/activate" ]]; then
    echo "未找到完整的 Linux 虚拟环境，请先执行："
    echo "  python3 -m venv .venv"
    echo "  source .venv/bin/activate"
    echo "  python -m pip install -r requirements.txt"
    exit 1
fi

# shellcheck disable=SC1091
source .venv/bin/activate
mkdir -p "$RUN_DIR"
export TCP_PORT

start_service() {
    local name="$1"
    local log_file="$2"
    shift 2

    local pid_file="$RUN_DIR/$name.pid"
    if [[ -f "$pid_file" ]]; then
        local old_pid
        old_pid="$(cat "$pid_file")"
        if kill -0 "$old_pid" 2>/dev/null; then
            echo "$name 已在运行，PID=$old_pid"
            return 0
        fi
        rm -f "$pid_file"
    fi

    nohup "$@" > "$log_file" 2>&1 &
    local new_pid=$!
    echo "$new_pid" > "$pid_file"
    echo "$name 已启动，PID=$new_pid"
}

start_service "tcp" "tcp.log" python tcp_server.py
start_service "api" "api.log" python -m uvicorn main:app --host 127.0.0.1 --port "$API_PORT"

export API_BASE_URL="http://127.0.0.1:$API_PORT"
start_service "web" "web.log" python -m uvicorn web:app --host 0.0.0.0 --port "$WEB_PORT"

sleep 2
failed=0
for service in tcp api web; do
    pid_file="$RUN_DIR/$service.pid"
    pid="$(cat "$pid_file")"
    if ! kill -0 "$pid" 2>/dev/null; then
        echo "$service 启动失败，请检查对应日志"
        failed=1
    fi
done

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi

echo "=============================="
echo "启动完成"
echo "TCP 端口：$TCP_PORT"
echo "API 地址：http://127.0.0.1:$API_PORT/"
echo "网页地址：http://服务器地址:$WEB_PORT/"
echo "=============================="
