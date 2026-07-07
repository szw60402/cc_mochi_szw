# USB 串口控制方案

## 背景

ESP32-C3 桌宠通过 USB CDC 串口接收 PC 端实时指令，实现 Claude Code 工作状态 → 表情联动。

## 核心难题与解决

Windows 下 ESP32 的 USB CDC 存在三重障碍：

| # | 问题 | 解决方案 |
|---|------|------|
| 1 | Windows 将 COM 口标记为独占 | 使用 usbipd 将设备从 Windows 驱动栈剥离 |
| 2 | 打开串口时 DTR 跳变触发 ESP32 硬复位 | Linux 下 `pyserial` 设置 `dtr=False, rts=False` |
| 3 | WSL 内 `/dev/ttyACM0` 默认 root 权限 | `chmod 666` 或 udev 规则 |

## 最终架构

```
ESP32-C3 (固件: cc_mochi_szw.ino)
    │  USB 线
    ▼
Windows USB CDC 驱动 → usbipd 接管
    │  usbipd bind + attach
    ▼
WSL2 Ubuntu Linux → /dev/ttyACM0
    │  pyserial (dtr=False)
    ▼
state_broadcaster.py ← state.txt ← Claude Code
```

## 使用方法

### 初始化

```powershell
usbipd list                           # 找到 ESP32 的 BUSID (VID:PID=303A:1001)
usbipd bind --busid 2-1 --force
usbipd attach --wsl --busid 2-1
```

### 发送指令

```bash
python3 tools/clawd_wsl_send.py j     # 开心
python3 tools/clawd_wsl_send.py 4     # 极光
python3 tools/clawd_wsl_send.py y     # 编码
```

### 持久广播模式

```bash
python3 tools/state_broadcaster.py    # 持续监控 state.txt → 实时发送
```

## 注意事项

1. 拔插 USB 后必须重新 `bind + attach`
2. 需要管理员权限运行 usbipd
3. 烧录固件前先 `usbipd detach`
4. WSL 重启后重新 `chmod 666 /dev/ttyACM0`
