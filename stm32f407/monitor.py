#!/usr/bin/env python3
"""
STM32 串口监视器
用于监听 STM32F407 的串口输出
"""

import serial
import serial.tools.list_ports
import time
import sys

def find_stm32_port():
    """自动查找 STM32 设备端口"""
    ports = serial.tools.list_ports.comports()
    
    print("可用串口:")
    for port in ports:
        print(f"  {port.device} - {port.description}")
        # 常见的 STM32 识别标识
        if any(keyword in port.description.upper() for keyword in 
               ['STM32', 'STLINK', 'USB SERIAL']):
            return port.device
    
    return None

def monitor_serial(port_name, baudrate=115200):
    """监听串口输出"""
    try:
        ser = serial.Serial(port_name, baudrate, timeout=1)
        print(f"已连接到 {port_name}, 波特率: {baudrate}")
        print("开始监听串口输出... (Ctrl+C 退出)")
        print("-" * 50)
        
        while True:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    timestamp = time.strftime("%H:%M:%S")
                    print(f"[{timestamp}] {data}")
            time.sleep(0.01)
            
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except KeyboardInterrupt:
        print("\n监听已停止")
    finally:
        if 'ser' in locals():
            ser.close()

def main():
    # 如果指定了端口参数
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        # 自动查找端口
        port = find_stm32_port()
        if not port:
            print("未找到 STM32 设备，请手动指定端口:")
            print("python monitor.py COM端口号")
            return
    
    print(f"尝试连接端口: {port}")
    monitor_serial(port)

if __name__ == "__main__":
    main()
