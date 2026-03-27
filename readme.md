
[中文](./readme.zh.md)

# MicroXCP Quick Start

## 1. Overview

MicroXCP is a lightweight XCP (Universal Measurement and Calibration Protocol) slave stack designed for embedded systems.

It provides:

* Memory calibration (read/write)
* DAQ (data acquisition)
* Platform-independent design

---

## 2. Configuration

Edit `MicroXcp_Conf.h` according to your project:

```c
#define MICROXCP_MAIN_PERIOD_MS   (1U)
#define MICROXCP_FEATURE_DAQ      (1U)
#define MICROXCP_FEATURE_CALIBRATION (1U)
```

---

## 3. Integration

### Step 1: Initialize

Call initialization once at system startup:

```c
MicroXcp_Init();
```

---

### Step 2: Periodic Handler

Call the handler periodically (e.g. 1ms task):

```c
void SysTask_1ms(void)
{
    MicroXcp_TimerHandler();
}
```

---

### Step 3: Implement Transmit Function

Provide transport layer implementation:

```c
int MicroXcp_Transmit(uint8_t *data, size_t size)
{
    // Example: CAN / UART send
    return YourDriver_Send(data, size);
}
```

---

### Step 4: Feed Received Data

Call this function when a complete XCP frame is received:

```c
void OnFrameReceived(uint8_t *data, uint16_t len)
{
    MicroXcp_ReceiveCallback(data, len);
}
```

---

## 5. Notes

* `MicroXcp_TimerHandler()` must be called periodically.
* `MicroXcp_Transmit()` should be non-blocking or bounded-time.
* The stack does not include transport layer (CAN/UART must be provided by user).
* All buffers must remain valid during processing.

---

## 6. Minimal Example

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

## 7. Supported Features

* Calibration (optional)
* DAQ (optional)
* Little Endian / Big Endian support

---

## 8. License

[license](./LICENSE)
