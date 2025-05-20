import serial
import time
import random

# 打开串口 COM4（替换成你实际串口号）
ser = serial.Serial("COM13", baudrate=115200, timeout=2)

# Blynk Token 和目标服务器
token = "bUs5D2Zh_mK3N0wALJ5C2UqCCllc-Eq3"
host = "ny3.blynk.cloud"



while True:
    # 1. 建立 TCP 连接
    ser.write(f'AT+QIOPEN=1,0,"TCP","{host}",80,0,1\r\n'.encode())
    time.sleep(2)
    print("[QIOPEN] ", ser.read_all().decode(errors="ignore"))
    # 生成 0–100 之间的随机整数
    value = random.randint(0, 100)
    print(f"[INFO] Sending value: {value}")

    # 2. 启动发送数据模式
    ser.write(b"AT+QISEND=0\r\n")
    time.sleep(0.5)
    ser.read_all()  # 清理 >

    # 3. 构造并发送 HTTP 请求
    http_path = f"/external/api/update?token={token}&v1={value}"
    http_header = (
        f"GET {http_path} HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    ser.write(http_header.encode())
    time.sleep(0.2)

    # 4. 发送 Ctrl+Z (0x1A) 作为结束符
    ser.write(b"\x1a")
    time.sleep(3)

    # 5. 接收并打印服务器响应
    response = ser.read_all().decode(errors="ignore")
    print("[RESPONSE]\n", response)

    # 可选：重连或循环使用当前连接（根据模块行为决定是否需要重连）
    time.sleep(1)  # 每隔 1 秒发送一次
