#!/usr/bin/env python3
"""WSL串口指令发送 — 用于Hermes控制Clawd-Mochi桌宠
用法: python3 clawd_wsl_send.py j  (发送"开心")
字符映射: w正常 s眯眼 e生气 f悲伤 g疲惫 h睡觉 i思考 j开心 k烦躁 l亲亲
          m眨眼 n无聊 o疑问 p晕 u死掉 b左顾右盼 c书写 t编辑 v执行
          x分析 y编码 z震惊 1得意 2慌乱 3骄傲 4极光
"""
import serial
import time
import sys
import os

PORT = '/dev/ttyACM0'

# 等待设备出现
for _ in range(20):
    if os.path.exists(PORT):
        break
    time.sleep(0.5)
else:
    print(f'error: {PORT} not found', file=sys.stderr)
    sys.exit(1)

# 设置权限(如果还没设)
try:
    os.chmod(PORT, 0o666)
except:
    pass

s = serial.Serial(PORT, 115200, timeout=1, write_timeout=2)
s.dtr = False
s.rts = False
time.sleep(1.5)  # 等ESP32从可能的热复位中恢复

if len(sys.argv) > 1:
    cmd = sys.argv[1]
    s.write(cmd.encode())
    s.flush()
    time.sleep(0.2)
    print(f'ok: {cmd}')
else:
    print('usage: python3 clawd_wsl_send.py <char>')

s.close()
