#!/usr/bin/env python3
"""
Clawd Mochi 实时状态广播器
读取 state.txt 中的当前状态，通过串口发送给 ESP32。
每 1.5s 发心跳，状态变化即时发送。
"""

import serial
import time
import os
import sys

# 配置
COM_PORT = "COM3"
BAUD = 115200
STATE_FILE = os.path.join(os.path.dirname(__file__), "state.txt")
HEARTBEAT_INTERVAL = 1.5  # 秒
MIN_STATE_CHANGE = 0.8    # 800ms 最短保持（ESP32 侧也做了）

# 状态优先级（越大越紧急），用于快速切换时的取舍
PRIORITY = {
    '2': 3,  # 慌乱(ERROR)
    '3': 3,  # 骄傲(DONE) — 紧急但正面
    'v': 2,  # 执行(EXECUTING)
    'y': 2,  # 编码(CODING)
    't': 1,  # 编辑(EDITING)
    'c': 1,  # 书写(WRITING)
    'x': 1,  # 分析(ANALYZING)
    'i': 1,  # 思考(THINKING)
    '1': 1,  # 得意(SMUG)
    'w': 0,  # 正常(IDLE)
    '4': 0,  # 极光(FABLE5)
    'b': 0,  # 环视(LOOKAROUND)
}

def connect_serial():
    """连接串口，带重试"""
    for attempt in range(5):
        try:
            ser = serial.Serial(COM_PORT, BAUD, timeout=0.5)
            time.sleep(2)  # ESP32 启动后需要稳定
            print(f"[OK] COM3 连接成功")
            return ser
        except serial.SerialException:
            print(f"[RETRY] COM3 连接失败，第{attempt+1}次重试...")
            time.sleep(2)
    print("[FATAL] 无法连接 COM3")
    sys.exit(1)

def read_state():
    """从文件读取当前状态字符"""
    try:
        with open(STATE_FILE, 'r') as f:
            state = f.read().strip()
            if len(state) >= 1:
                return state[0]  # 只取第一个字符
    except FileNotFoundError:
        pass
    return 'w'  # 默认正常眼

def send_state(ser, state_char):
    """发送状态切换命令"""
    cmd = f"S:{state_char}\n"
    try:
        ser.write(cmd.encode())
        ser.flush()
        # 等待 ACK
        for _ in range(3):
            ack = ser.readline().decode().strip()
            if ack.startswith("OK:"):
                return True
            time.sleep(0.05)
    except Exception as e:
        print(f"[ERR] 串口写入失败: {e}")
    return False

def send_heartbeat(ser):
    """发送心跳"""
    try:
        ser.write(b"H:1\n")
        ser.flush()
    except:
        pass

def main():
    print("  Clawd Mochi State Broadcaster")
    print("  监控 state.txt → COM3")
    print("=" * 40)

    ser = connect_serial()
    last_state = 'w'
    last_send_time = 0
    last_heartbeat = time.time()

    print(f"[READY] 当前状态: {last_state}")
    send_state(ser, 'w')  # 初始同步

    while True:
        try:
            current_state = read_state()

            # 状态变化且超过最短保持时间 → 发送
            if current_state != last_state:
                elapsed = time.time() - last_send_time
                if elapsed >= MIN_STATE_CHANGE:
                    print(f"[STATE] {last_state} → {current_state}")
                    send_state(ser, current_state)
                    last_state = current_state
                    last_send_time = time.time()
                else:
                    # 检查优先级：如果新状态更紧急，立即覆盖
                    new_prio = PRIORITY.get(current_state, 0)
                    old_prio = PRIORITY.get(last_state, 0)
                    if new_prio > old_prio:
                        print(f"[PRIO] {last_state} → {current_state} (紧急覆盖)")
                        send_state(ser, current_state)
                        last_state = current_state
                        last_send_time = time.time()

            # 定时心跳
            if time.time() - last_heartbeat >= HEARTBEAT_INTERVAL:
                send_heartbeat(ser)
                last_heartbeat = time.time()

            time.sleep(0.1)

        except KeyboardInterrupt:
            print("\n[BYE] 停止广播")
            ser.close()
            break
        except Exception as e:
            print(f"[ERR] {e}")
            time.sleep(1)
            try:
                ser.close()
                ser = connect_serial()
            except:
                pass

if __name__ == "__main__":
    main()
