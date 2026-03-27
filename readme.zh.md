
---

[En](./readme.md)

# MicroXCP 快速开始

## 1. 概述

MicroXCP 是一个面向嵌入式系统的轻量级 XCP（通用测量与标定协议）从机协议栈。

主要特性：

* 支持内存标定（读/写）
* 支持 DAQ 数据采集
* 与底层传输层解耦（平台无关）

---

## 2. 配置

根据项目需求修改 `MicroXcp_Conf.h`：

```c
#define MICROXCP_MAIN_PERIOD_MS   (1U)
#define MICROXCP_FEATURE_DAQ      (1U)
#define MICROXCP_FEATURE_CALIBRATION (1U)
```

---

## 3. 集成方法

### 步骤 1：初始化

在系统启动时调用一次：

```c
MicroXcp_Init();
```

---

### 步骤 2：周期调度

按固定周期调用（例如 1ms 任务）：

```c
void SysTask_1ms(void)
{
    MicroXcp_TimerHandler();
}
```

---

### 步骤 3：实现发送接口

用户需要提供底层发送函数（如 CAN / UART）：

```c
int MicroXcp_Transmit(uint8_t *data, size_t size)
{
    // 示例：调用底层驱动发送
    return YourDriver_Send(data, size);
}
```

---

### 步骤 4：接收数据输入

当接收到完整的 XCP 报文后调用：

```c
void OnFrameReceived(uint8_t *data, uint16_t len)
{
    MicroXcp_ReceiveCallback(data, len);
}
```

---

## 4. 注意事项

* `MicroXcp_TimerHandler()` 必须按固定周期调用
* `MicroXcp_Transmit()` 应为非阻塞或有界执行时间
* 本协议栈不包含传输层（需用户自行实现 CAN / UART 等）
* 数据缓冲区在处理期间必须保持有效

---

## 5. 最小示例

```c
int main(void)
{
    System_Init();

    MicroXcp_Init();

    while (1)
    {
        MicroXcp_TimerHandler();
    }
}
```

---

## 6. 功能支持

* 标定（可选）
* DAQ 数据采集（可选）
* 支持大小端配置

---

## 7. 许可证

详见：[license](./LICENSE)
