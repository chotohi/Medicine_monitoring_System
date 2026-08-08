#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""TCP 数据接收服务。

监听 2025 端口，接收单片机发送的换行分隔报文，并把每个药品的最新状态
写入 SQLite。报文格式为：MedNN/湿度/温度/重量\r\n。
"""

import asyncio
import os
import socket
import threading

from tortoise import Tortoise

from models import MedInfo


db_loop = asyncio.new_event_loop()
db_ready = threading.Event()
db_start_error: Exception | None = None


async def init_db():
    """初始化 Tortoise ORM 和数据库表。"""
    await Tortoise.init(
        db_url="sqlite://medinfo.db",
        modules={"models": ["models"]},
    )
    await Tortoise.generate_schemas()


async def update_medinfo(name, humidity, temp, weight):
    """新增药品状态，或覆盖该药品的上一条状态。"""
    await MedInfo.update_or_create(
        defaults={
            "humidity": humidity,
            "temp": temp,
            "weight": weight,
        },
        name=name,
    )


def start_db_loop():
    """在后台线程中运行数据库异步事件循环。"""
    global db_start_error

    asyncio.set_event_loop(db_loop)
    try:
        db_loop.run_until_complete(init_db())
    except Exception as exc:  # 启动线程中的异常需要传回主线程
        db_start_error = exc
        db_ready.set()
        return

    db_ready.set()
    db_loop.run_forever()


class CustomTCPServer:
    """接收并解析药品监测数据。"""

    NAME_MAP = {
        "Med01": "药品A",
        "Med02": "药品B",
        "Med03": "药品C",
    }

    MAX_BUFFER_SIZE = 4096

    @staticmethod
    def process_line(line: str):
        """解析一条报文并将数据库任务提交给后台事件循环。"""
        name, humidity, temp, weight = line.split("/", 3)
        name = name.strip()
        if not name:
            raise ValueError("药品编号不能为空")

        cn_name = CustomTCPServer.NAME_MAP.get(name, name)
        future = asyncio.run_coroutine_threadsafe(
            update_medinfo(
                cn_name,
                float(humidity),
                float(temp),
                float(weight),
            ),
            db_loop,
        )
        future.result(timeout=5)

    @staticmethod
    def handle_client(client_socket, client_address):
        print(f"新的客户端连接：{client_address}")
        buffer = ""

        try:
            while True:
                data = client_socket.recv(1024)
                if not data:
                    break

                buffer += data.decode("ascii", errors="replace")
                if len(buffer) > CustomTCPServer.MAX_BUFFER_SIZE:
                    raise ValueError("接收缓冲区超过限制，已断开客户端")

                # 单片机只发送到字符串结尾前，因此以 \n 而不是 \0 拆包。
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip("\r\0 ")
                    if not line:
                        continue

                    print(f"收到 {client_address}：{line}")
                    try:
                        CustomTCPServer.process_line(line)
                        client_socket.sendall(b"OK\r\n")
                    except Exception as exc:
                        print(f"数据处理失败：{exc}")
                        client_socket.sendall(b"ERROR\r\n")

        except (ConnectionError, OSError, ValueError) as exc:
            print(f"客户端 {client_address} 异常：{exc}")
        finally:
            client_socket.close()
            print(f"客户端 {client_address} 已断开")

    @staticmethod
    def start(host="0.0.0.0", port=2025):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((host, port))
        server.listen(5)

        print(f"TCP 服务已启动，监听 {host}:{port}")

        try:
            while True:
                client, addr = server.accept()
                threading.Thread(
                    target=CustomTCPServer.handle_client,
                    args=(client, addr),
                    daemon=True,
                ).start()
        finally:
            server.close()


if __name__ == "__main__":
    threading.Thread(target=start_db_loop, daemon=True).start()
    if not db_ready.wait(timeout=10):
        raise RuntimeError("数据库初始化超时")
    if db_start_error is not None:
        raise RuntimeError("数据库初始化失败") from db_start_error

    CustomTCPServer.start(
        host=os.getenv("TCP_HOST", "0.0.0.0"),
        port=int(os.getenv("TCP_PORT", "2025")),
    )
