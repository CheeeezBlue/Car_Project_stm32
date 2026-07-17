# OLED 菜单系统说明

## 硬件

| 组件 | 引脚 | 说明 |
|------|------|------|
| OLED | PB8(SCL) / PB9(SDA) | 软件 I2C，地址 0x78，4行×16列，列号从 **1** 开始 |
| KEY1 | PB3 | 低电平有效，内部上拉 |
| KEY2 | PB4 | 低电平有效，内部上拉 |
| KEY3 | PB5 | 低电平有效，内部上拉 |
| KEY4 | PC13 | 低电平有效，内部上拉 |

---

## 通用规则

- 上电显示 `Car  Ready!` 停留 1.5 秒后进入主菜单
- 每个子页面停留期间，OLED 每 10ms 刷新一次（仅在内容变化时重绘）
- 物理按键 **松开瞬间** 触发短按（下降沿检测），长按 >0.5s 后以每 30ms 连发
- 串口（UART1: CH340 USB / UART3: 蓝牙 HC05）发送 `KEY1`~`KEY4` 可直接操作菜单，效果等同于物理短按
- 每层 MENU 的按键定义见下方各表

---

## 一、主菜单 `[L0] MAIN`

### 显示

```
===  MAIN  ===
1.Mode  2.Data
3.PID   4.Run
```

### 按键

| 按键 | 动作 | 底层调用 |
|------|------|----------|
| KEY1 | 进入模式切换 | `menu.level = MENU_MODE_SELECT` |
| KEY2 | 进入数据查看 | `menu.level = MENU_DATA_CAT` |
| KEY3 | 进入 PID 调参 | `menu.level = MENU_PID_CAT` |
| KEY4 | 进入启停确认 | `menu.level = MENU_RUN_CONFIRM` |

---

## 二、模式切换 `[L1] MODE SELECT`

### 显示

```
  MODE SELECT
>1.IDLE
 2.MANUAL
 3.LINE
```

`>` 标记当前正在运行的模式。

### 模式槽位对应的 RunMode_t 枚举值

| 槽位 | 按键 | 模式 | 含义 |
|------|------|------|------|
| 1 | KEY1 | IDLE (0) | 停车等待，屏蔽所有速度/转向指令 |
| 2 | KEY2 | MANUAL (1) | 手动模式，蓝牙串口直控速度 |
| 3 | KEY3 | LINE (3) | 灰度循迹模式，LineControl PID 接管差速 |

### 按键

| 按键 | 动作 | 底层调用 |
|------|------|----------|
| KEY1/2/3 | 切换到对应模式 **并自动返回主菜单** | `RunMode_Set(mode)` → 关闭旧环 → `Car_Stop()` → 开启新环 |
| KEY4 | 返回主菜单（不切换模式） | `menu.level = MENU_MAIN` |

### 底层详解：`RunMode_Set(MODE_LINE)` 的执行流程

1. `Car_SetYawDiff(0.0f)` — 差速清零
2. `LineControl_Disable()` — 关闭旧 LINE 环（如果之前在 LINE 模式）
3. `Car_Stop()` — 停止两轮（目标速度清零，编码器脉冲目标清零）
4. `LineControl_Enable()` — 开启 LINE 环（设置 `enabled=1`）
5. `current_mode = MODE_LINE`

---

## 三、数据查看

### 3-1 类别选择 `[L1] DATA CATEGORY`

#### 显示

```
 DATA  SELECT
> SPEED LOOP
   KEY1/2:switch
KEY3:enter 4:back
```

#### 可选类别

| 类别 | 页数 | 说明 |
|------|------|------|
| SPEED LOOP | 4 页 (0~3) | 速度环实时数据 |
| GRAYSCALE | 1 页 | 灰度传感器（当前未启用） |
| YAW ANGLE | 1 页 | MPU6050 偏航角速度（当前未启用） |
| SYSTEM | 1 页 | 系统状态摘要 |

#### 按键

| 按键 | 动作 |
|------|------|
| KEY1 | 上翻类别（循环） |
| KEY2 | 下翻类别（循环） |
| KEY3 | 进入当前类别的数据翻页 |
| KEY4 | 返回主菜单 |

---

### 3-2-1 数据翻页：SPEED LOOP（第 0 页 / 总览）

#### 显示

```
SPEED  LOOP
LT:  50 RT:  50
LS:  48 RS:  49
LP: 420 RP: 400
```

| 行 | 标签 | 含义 | 数据来源 |
|----|------|------|----------|
| 1 | SPEED LOOP | 标题 | — |
| 2 | LT / RT | 左/右轮目标速度（单位：编码器脉冲/10ms） | `Car_GetTarget(0)` / `Car_GetTarget(1)` |
| 3 | LS / RS | 左/右轮实际速度（滤波后，单位：脉冲/10ms） | `Car_GetFiltSpeed(0)` / `Car_GetFiltSpeed(1)` |
| 4 | LP / RP | 左/右电机 PWM 占空比（0~999，TIM1 自动重载值） | `Motor_GetLastPWM(MOTOR_LEFT)` / `Motor_GetLastPWM(MOTOR_RIGHT)` |

#### 按键

| 按键 | 动作 |
|------|------|
| KEY1 | 上一页 |
| KEY2 | 下一页 |
| KEY4 | 返回类别选择 |

---

### 3-2-2 数据翻页：SPEED LOOP（第 1 页 / PID 监控）

#### 显示

```
PID  MONITOR
Kp:0.500 Ki:0.500
Kd:0.070
T:  50 T:  50
```

| 行 | 标签 | 含义 |
|----|------|------|
| 1 | PID MONITOR | 标题 |
| 2 | Kp / Ki | 速度环当前 Kp、Ki |
| 3 | Kd | 速度环当前 Kd |
| 4 | T / T | 与总览页 LT/RT 相同 |

#### 按键

| 按键 | 动作 |
|------|------|
| KEY1 | 上一页 |
| KEY2 | 下一页 |
| KEY4 | 返回类别选择 |

---

### 3-2-3 数据翻页：SPEED LOOP（第 2 页 / 左电机详情）

#### 显示

```
MOTOR LEFT
SPD:  50 ENC:  48
PWM: 420
FLT: 48.5
```

| 行 | 标签 | 含义 |
|----|------|------|
| 1 | MOTOR LEFT | 标题 |
| 2 | SPD / ENC | 左轮目标速度 / 左轮原始编码器脉冲 |
| 3 | PWM | 左电机 PWM 输出 |
| 4 | FLT | 左轮滤波后速度（浮点数，1位小数） |

### 3-2-4 数据翻页：SPEED LOOP（第 3 页 / 右电机详情）

同理，所有数据为右电机。

---

### 3-3 数据翻页：SYSTEM

#### 显示

```
   SYSTEM
MODE:LINE
UART1/3  OK
    4:BACK
```

| 行 | 内容 | 数据来源 |
|----|------|----------|
| 1 | SYSTEM | 标题 |
| 2 | MODE:LINE | 当前运行模式名 | `RunMode_GetName(RunMode_Get())` |
| 3 | UART1/3 OK | 串口状态（固定显示，实际未检测） | — |

#### 按键

| 按键 | 动作 |
|------|------|
| KEY4 | 返回类别选择 |

此类别只有 1 页，KEY1/KEY2 无效（不会翻到其他页）。

---

## 四、PID 调参

### 4-1 PID 类别选择 `[L1] PID SELECT`

#### 显示

```
 PID  SELECT
1.SPEED PID
2.LINE  PID
   4.BACK
```

#### 按键

| 按键 | 动作 |
|------|------|
| KEY1 | 进入速度环 PID 调参 |
| KEY2 | 进入灰度 PID 调参 |
| KEY4 | 返回主菜单 |

---

### 4-2 PID 调参界面 `[L2] PID TUNE`

#### 显示（速度环 PID 示例）

```
 SPEED PID
*Kp:0.500
 Ki:0.500
 Kd:0.070
```

- `*` 表示当前选中的参数（KEY3 循环切换 Kp → Ki → Kd）
- 进入时默认选中 Kp

#### 按键

| 按键 | 动作 | 底层调用 |
|------|------|----------|
| KEY1 | **增大** 当前选中参数 | 短按 +0.001，长按 >0.5s 后每 30ms +0.010 |
| KEY2 | **减小** 当前选中参数 | 短按 -0.001，长按 >0.5s 后每 30ms -0.010 |
| KEY3 | 切换选中参数（Kp→Ki→Kd 循环） | `menu.pid_param = (pid_param + 1) % 3` |
| KEY4 | 返回 PID 类别选择 | `menu.level = MENU_PID_CAT` |

#### 底层详解

- **速度环 PID**：修改 `Car_SetKp/Ki/Kd()`，直接影响 `CarControl_Update()` 中的 PID 计算
  - 计算公式：`PWM = FFL * target + PID(target, encoder) + diff`
- **灰度 PID**：修改 `LineControl_SetKp/Ki/Kd()`，影响 `LineControl_Update()` 中的 PID 计算
  - 计算公式：`diff = Kp * error + Ki * integral(error) + Kd * derivative(error)`
  - diff 输出限幅：[-40, 40]

---

## 五、启停确认 `[L1] RUN CONFIRM`

### 显示

```
 RUN  MODE ?
                
    LINE
4=RUN 1/2/3=STOP
```

第 3 行显示当前运行模式名。

### 按键

| 按键 | 动作 | 底层调用 |
|------|------|----------|
| KEY4 | **RUN** — 启动当前模式 | `RunMode_Run()` → 根据当前模式调用 `LineControl_Enable()` 或 `YawControl_Enable()` |
| KEY1/2/3 | **STOP** — 停车 | `RunMode_Stop()` → `Car_Stop()` + `LineControl_Disable()` + `YawControl_Disable()` |
| 超时 2 秒 | 自动取消，回主菜单 | — |

> **注意**：`Car_Stop()` 会将左右目标速度清零（`target_L = 0, target_R = 0`），编码器脉冲目标清零，PWM 归零，但不修改 PID 参数。

---

## 六、串口命令快速参考

串口（UART1 USB / UART3 蓝牙）发送以下命令可远程操作菜单：

| 命令 | 效果 | 等效按键 |
|------|------|----------|
| `KEY1` | 触发 KEY1 短按 | 物理 KEY1 |
| `KEY2` | 触发 KEY2 短按 | 物理 KEY2 |
| `KEY3` | 触发 KEY3 短按 | 物理 KEY3 |
| `KEY4` | 触发 KEY4 短按 | 物理 KEY4 |

这些命令在**任何菜单层级**都有效，直接调用 `Menu_Dispatch(key, 1)`，不经过物理按键扫描。

### 常用操作流（串口）

```
KEY2          → 进入数据查看
KEY3          → 进入 SPEED 数据
KEY2 × N      → 翻页查看
KEY4          → 返回类别选择
KEY4          → 返回主菜单
```

---

## 七、菜单状态机总览

```
MENU_MAIN (L0)
 ├─ KEY1 → MENU_MODE_SELECT (L1)
 │          ├─ KEY1/2/3 → 选模式 → 自动回 MAIN
 │          └─ KEY4 → 回 MAIN
 │
 ├─ KEY2 → MENU_DATA_CAT (L1)
 │          ├─ KEY1/2 → 切类别
 │          ├─ KEY3 → MENU_DATA_PAGE (L2)
 │          │          ├─ KEY1/2 → 翻页
 │          │          └─ KEY4 → 回 DATA_CAT
 │          └─ KEY4 → 回 MAIN
 │
 ├─ KEY3 → MENU_PID_CAT (L1)
 │          ├─ KEY1 → MENU_PID_TUNE (L2) [速度环]
 │          │          ├─ KEY1/2 → ±step
 │          │          ├─ KEY3 → 切 Kp/Ki/Kd
 │          │          └─ KEY4 → 回 PID_CAT
 │          ├─ KEY2 → MENU_PID_TUNE (L2) [灰度]
 │          └─ KEY4 → 回 MAIN
 │
 └─ KEY4 → MENU_RUN_CONFIRM (L1)
            ├─ KEY4 → RunMode_Run() → 回 MAIN
            ├─ KEY1/2/3 → RunMode_Stop() → 回 MAIN
            └─ 超时2s → 回 MAIN
```

---

## 八、配置常量

| 常量 | 值 | 说明 |
|------|----|------|
| `PID_STEP_FINE` | 0.001 | PID 短按步长 |
| `PID_STEP_COARSE` | 0.010 | PID 长按连发步长 |
| `LONG_PRESS_TICKS` | 50 | 长按判定阈值 (0.5s @ 10ms) |
| `REPEAT_INTERVAL` | 3 | 长按连发间隔 (30ms) |
| `RUN_CONFIRM_TIMEOUT` | 200 | 启停确认超时 (2s @ 10ms) |

---

## 九、状态变量（Menu_t 结构体）

| 字段 | 类型 | 说明 |
|------|------|------|
| `level` | MenuLevel_t | 当前层级（0~6） |
| `cursor` | u8 | 当前选中项（预留，暂未使用） |
| `data_cat` | u8 | 数据类别：0=SPEED, 1=GRAY, 2=YAW, 3=SYS |
| `data_page` | u8 | 当前数据页号（SPEED: 0~3，其他: 0） |
| `pid_cat` | u8 | PID 类别：0=速度, 1=灰度 |
| `pid_param` | u8 | 当前选中 PID 参数：0=Kp, 1=Ki, 2=Kd |
| `key_held` | u8 | 当前正被按住的键号，0=无 |
| `hold_ticks` | u16 | 按住持续 tick 数（每 10ms +1） |
| `confirm_tick` | u16 | 启停确认倒计时 tick 数 |
| `dirty` | u8 | 脏标记：1=下次 Update 时重绘 |
