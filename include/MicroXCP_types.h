/**
 * @file MicroXcp_types.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-03-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef MICROXCP_TYPES_H
#define MICROXCP_TYPES_H

#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include "MicroXCP_conf.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MICROXCP_OK,
    MICROXCP_ERR,
} MicroXcp_Status_t;

typedef union
{
    struct __attribute__((packed))
    {
        uint8_t pid;
        uint8_t payload[7];
    } byte;
    uint8_t data[8];
} MicroXcp_Frame_t;

typedef struct
{
    uint8_t pid;
    void (*func)(void);

} MicroXcp_RegisterTable_t;

typedef struct MicroXcp_FindPid
{
    uint8_t pid;
    void (*func)(void);

    struct MicroXcp_FindPid *next;
} MicroXcp_FindPid_t;

typedef struct
{
    MicroXcp_FindPid_t list[4]; // 暂定链表大小
} MicroXcp_Cto_t;

typedef struct
{
    MicroXcp_FindPid_t list[5];
    uint32_t address;      // 当前地址
    uint8_t ext;           // 内存扩展
    uint8_t r_len;         // 读取内存字节数
    uint8_t Cache_Byte[8]; // 缓存字节
} MicroXcp_Mem_t;          // 内存操作

typedef struct
{
    uint8_t con_sta;     // 连接状态
    uint8_t daq_run;     // 0x01 运行中
    uint8_t cal_protect; // 0x01 cal受保护(不能写)
    uint8_t daq_protect; // 0x01 daq受保护
    uint8_t pgm_protect; // 0x01 pgm受保护
                            // 一般：
                            // 0x00 → 没有保护（开发阶段）
} MicroXcp_STA_t;

typedef struct
{
    volatile bool en;
    size_t len;
} MicroXcp_Ready_t;

typedef struct
{
    uint32_t addr;
    uint8_t size;
} MicroXcp_Entry_t;

typedef struct
{
    MicroXcp_Entry_t entries[MICROXCP_DAQ_ODT_DATA_SIZE];
    uint8_t entry_count; // 这个 ODT 实际存了几个变量
    uint8_t pid;         // odt的pid
} MicroXcp_Odt_t;

typedef struct
{
    MicroXcp_Odt_t odts[MICROXCP_DAQ_ODT_COUNT]; // 一个周期内可能要发 8 帧 CAN 报文
    uint8_t odt_count;                           // 实际激活了几帧
    uint8_t event_channel;                       // 周期
    uint8_t is_running;                          // 状态开关
    uint8_t en;                                  // 使能开关
} MicroXcp_DaqObj_t;

typedef struct
{
    MicroXcp_DaqObj_t daq_list[MICROXCP_DAQ_LIST_COUNT];
    MicroXcp_FindPid_t pid_list[7];

    uint32_t tick;

    uint8_t ptr_daq;
    uint8_t ptr_odt;
    uint8_t ptr_entry;
} MicroXcp_Context_t;

typedef struct
{
    MicroXcp_Ready_t ready;
    MicroXcp_Frame_t frame;

    MicroXcp_Cto_t cto;
    MicroXcp_STA_t sta;
    MicroXcp_Mem_t mem;

    MicroXcp_Context_t daq; // daq是独立的模块

} MicroXcp_Obj_t; // xcp对象

#ifdef __cplusplus
}
#endif

#endif
