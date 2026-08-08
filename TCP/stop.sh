#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_DIR="$SCRIPT_DIR/.run"

echo "=============================="
echo "停止药品仓储监测系统"
echo "=============================="

stop_service() {
    local name="$1"
    local expected="$2"
    local pid_file="$RUN_DIR/$name.pid"

    if [[ ! -f "$pid_file" ]]; then
        echo "$name 未运行"
        return 0
    fi

    local pid
    pid="$(cat "$pid_file")"
    if ! kill -0 "$pid" 2>/dev/null; then
        echo "$name 已停止，清理旧 PID 文件"
        rm -f "$pid_file"
        return 0
    fi

    local command_line
    command_line="$(ps -p "$pid" -o args= 2>/dev/null || true)"
    if [[ "$command_line" != *"$expected"* ]]; then
        echo "$name 的 PID 已被其他进程占用，未发送停止信号：$pid"
        rm -f "$pid_file"
        return 1
    fi

    kill "$pid"
    for _ in {1..20}; do
        if ! kill -0 "$pid" 2>/dev/null; then
            rm -f "$pid_file"
            echo "$name 已停止"
            return 0
        fi
        sleep 0.25
    done

    echo "$name 未在规定时间内退出，发送强制停止信号"
    kill -KILL "$pid"
    rm -f "$pid_file"
}

stop_service "web" "uvicorn web:app"
stop_service "api" "uvicorn main:app"
stop_service "tcp" "tcp_server.py"

echo "=============================="
echo "系统已停止"
echo "=============================="
