# 表情字符映射表

## Claude Code 自动联动规则

ESP32 通过 USB 串口接收 PC 端指令，Claude Code 根据当前工作状态自动写入对应字符：

| Claude Code 状态 | 字符 | 对应表情 | 视觉效果 |
|:--|:--:|:--|:--|
| 读取文件 | `y` | 编码 | 数码雨 |
| 写入代码 | `c` | 书写 | 纸面逐行写字 |
| 编译中   | `v` | 执行 | 脉冲条纹扫描 |
| 编译错误 | `2` | 慌乱 | 反白画面 + 冷汗滴 |
| 编译成功 | `3` | 骄傲 | ^ ^ 眼 + 星星爆发 |
| Fable5  | `4` | 极光 | 像素波动色彩 |

## 完整字符映射

| 字符 | 表情 | 字符 | 表情 | 字符 | 表情 |
|:--:|------|:--:|------|:--:|------|
| `w` | 正常 Normal | `s` | 眯眼 Squint | `e` | 生气 Angry |
| `f` | 悲伤 Sad | `g` | 疲惫 Tired | `h` | 睡觉 Sleep |
| `i` | 思考 Think | `j` | 开心 Happy | `k` | 烦躁 Annoyed |
| `l` | 亲亲 Kiss | `m` | 眨眼 Wink | `n` | 无聊 Bored |
| `o` | 疑问 Confused | `p` | 晕 Dizzy | `u` | 死掉 Dead |
| `b` | 左顾右盼 Look | `c` | 书写 Writing | `t` | 编辑 Editing |
| `v` | 执行 Executing | `x` | 分析 Analyzing | `y` | 编码 Coding |
| `z` | 震惊 Surprise | `1` | 得意 Smug | `2` | 慌乱 Panic |
| `3` | 骄傲 Proud | `4` | 极光 Fable5 | | |

## 空闲自动轮播

无外部指令时按概率自动循环：

```
正常 Normal (45%) → 疲惫 Tired (30%) → 睡觉 Sleep (25%)
```

时间规则：
- **23:00–08:00** 强制睡觉
- **12:00–13:00** 强制疲惫（午休）
