/**
 * @file MicroXCP.c
 * @author https://github.com/xfp23
 * @brief xcp源码实现
 * @version 0.1
 * @date 2026-03-26
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "MicroXcp.h"
#include "MicroXCP_types.h"
#include "MicroXcp_private.h"
#include "string.h"


#define XCP_SIZEOF(x) (sizeof(x) / sizeof(x[0]))

static MicroXcp_Obj_t Xcp_obj = {0};
MicroXcp_Obj_t *const this = &Xcp_obj;

MicroXcp_RegisterTable_t CtoTable[] = {
    {.pid = CONNECT, .func = MicroXcp_ConnectResFunc},
    {.pid = DISCONNECT, .func = MicroXcp_DisConnectResFunc},
    {.pid = GET_STATUS, .func = MicroXcp_GetStatusResFunc},
    {.pid = SYNCH, .func = MicroXcp_SynchResFunc},
};

MicroXcp_RegisterTable_t MemTable[] = {
    {.pid = SET_MTA, .func = MicroXcp_SetMatResFunc},
    {.pid = UPLOAD, .func = MicroXcp_UploadResFunc},
    {.pid = SHORT_UPLOAD, .func = MicroXcp_ShortUploadResFunc},
    {.pid = DOWNLOAD, .func = MicroXcp_DownloadResFunc},
    {.pid = SHORT_DOWNLOAD, .func = MicroXcp_ShortDownLoadResFunc},
};

 /* 静态事件通道定义，与 event_channel 编号一一对应 */
const MicroXcp_EventChannel_t s_EventChannelTable[2] =
{
    /* ch0: 1ms  */ { .time_cycle = 1,  .time_unit = 2, .priority = 0, .max_daq_list = MICROXCP_DAQ_LIST_COUNT, .name = "1ms"  },
    /* ch1: 10ms */ { .time_cycle = 10, .time_unit = 2, .priority = 1, .max_daq_list = MICROXCP_DAQ_LIST_COUNT, .name = "10ms" },
};

MicroXcp_RegisterTable_t DaqTable[] = {
    {.pid = GET_DAQ_PROCESSOR_INFO,  .func = MicroXcp_GetDaqSizeResFunc},
    {.pid = GET_DAQ_RESOLUTION_INFO, .func = MicroXcp_DaqResolutionInfoResFunc},
    {.pid = GET_DAQ_LIST_INFO,       .func = MicroXcp_GetDaqListInfoResFunc},
    {.pid = GET_DAQ_EVENT_INFO,      .func = MicroXcp_GetDaqEventInfoResFunc},
    {.pid = FREE_DAQ,                .func = MicroXcp_FreeDaqResFunc},
    {.pid = ALLOC_DAQ,               .func = MicroXcp_AllocDaqResFunc},
    {.pid = ALLOC_ODT,               .func = MicroXcp_AllocOdtResFunc},       
    {.pid = ALLOC_ODT_ENTRY,         .func = MicroXcp_AllocOdtEntryResFunc},   
    {.pid = SET_DAQ_PTR,             .func = MicroXcp_SetDaqPtrResFunc},
    {.pid = WRITE_DAQ,               .func = MicroXcp_WriteDaqResFunc},
    {.pid = SET_DAQ_LIST_MODE,       .func = MicroXcp_SetDaqModeResFunc},
    {.pid = START_STOP_DAQ_LIST,     .func = MicroXcp_StartDaqListResFunc},
    {.pid = START_STOP_SYNCH,        .func = MicroXcp_StartSyncResFunc},
};

static void MicroXcp_CtoInit()
{
    size_t size = XCP_SIZEOF(CtoTable);

    for (int i = 0; i < size; i++)
    {
        this->cto.list[i].func = CtoTable[i].func;
        this->cto.list[i].pid = CtoTable[i].pid;
    }

    // 对象内部绝对有足够的数组元素空间
    for (int i = 0; i < size - 1; i++) // 把链表接起来
    {
        this->cto.list[i].next = &this->cto.list[i + 1];
    }

    this->cto.list[size - 1].next = NULL; // 不循环，查到NULL就退出
}

static void MicroXcp_MemInit()
{
    size_t size = XCP_SIZEOF(MemTable);

    for (int i = 0; i < size; i++)
    {
        this->mem.list[i].func = MemTable[i].func;
        this->mem.list[i].pid = MemTable[i].pid;
    }

    for (int i = 0; i < size - 1; i++)
    {
        this->mem.list[i].next = &this->mem.list[i + 1];
    }

    this->mem.list[size - 1].next = NULL; // 不循环，查到NULL就退出
}

static void MicroXcp_DaqInit()
{
    size_t size = XCP_SIZEOF(DaqTable);

    for (int i = 0; i < size; i++)
    {
        this->daq.pid_list[i].func = DaqTable[i].func;
        this->daq.pid_list[i].pid  = DaqTable[i].pid;
    }

    for (int i = 0; i < size - 1; i++)
    {
        this->daq.pid_list[i].next = &this->daq.pid_list[i + 1];
    }

    this->daq.pid_list[size - 1].next = NULL;

    MicroXcp_DaqReset();
}


void MicroXcp_DaqHandler(uint32_t current_tick_ms)
{
    for (uint8_t d = 0; d < MICROXCP_DAQ_LIST_COUNT; d++)
    {
        MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[d];

        if (!pDaq->is_running)
            continue;

        /* 通道号越界：跳过，防止访问表越界 */
        if (pDaq->event_channel >= MICROXCP_EVENT_CHANNEL_COUNT)
            continue;

        /* 直接查表取周期，单位已经在表里定义好是 ms */
        uint32_t period = s_EventChannelTable[pDaq->event_channel].time_cycle;
        if (period == 0) period = 1; /* 防止除零 */

        if ((current_tick_ms % period) != 0)
            continue;

        for (uint8_t o = 0; o < MICROXCP_DAQ_ODT_COUNT; o++)
        {
            MicroXcp_Odt_t *pOdt = &pDaq->odts[o];

            if (pOdt->entry_count == 0)
                continue;

            uint8_t buf[8] = {0};
            uint8_t pos    = 1;
            buf[0] = pOdt->pid;

            for (uint8_t e = 0; e < pOdt->entry_count; e++)
            {
                MicroXcp_Entry_t *pEntry = &pOdt->entries[e];

                if (pEntry->addr == 0 || pEntry->size == 0)
                    continue;

                if ((pos + pEntry->size) > 8)
                    break;

                memcpy(&buf[pos], (const void *)(uintptr_t)pEntry->addr, pEntry->size);
                pos += pEntry->size;
            }

            MicroXcp_Transmit(buf, 8);
        }
    }
}

void MicroXcp_Init()
{
    MicroXcp_CtoInit();
    MicroXcp_MemInit();
    MicroXcp_DaqInit();
    this->sta.cal_protect = MICROXCP_FEATURE_CAL_PROTECT;
    this->sta.daq_protect = MICROXCP_FEATURE_CAL_PROTECT;
    this->sta.pgm_protect = MICROXCP_FEATURE_CAL_PROTECT;
}

// 弱函数，可重写
MICROXCP_WEAK int MicroXcp_Transmit(uint8_t *data, size_t size)
{
    (void)data;
    (void)size;

    return 0;
}

void MicroXcp_TimerHandler()
{
    if (!this->ready.en)
        return;

    if (this->sta.con_sta == 0 && this->frame.byte.pid != CONNECT && this->frame.byte.pid != GET_STATUS)
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNCH); // 同步错误
        goto CLEANUP;
    }

    if (this->frame.byte.pid >= 0xFC && this->frame.byte.pid <= 0xFF) // 查找Cto
    {
        MicroXcp_FindPid_t *node = this->cto.list;
        while (node)
        {
            if (node->pid == this->frame.byte.pid)
            {
                node->func();
                break;
            }
            node = node->next;
        }

        if (node == NULL)
        {
            MicroXcp_ReportError(XCP_ERR_CMD_UNKNOWN);
        }
    }
    else if (this->frame.byte.pid >= 0xEE && this->frame.byte.pid <= 0xF6)
    {
        MicroXcp_FindPid_t *node = this->mem.list;
        while (node)
        {
            if (node->pid == this->frame.byte.pid)
            {
                node->func();
                break;
            }
            node = node->next;
        }

        if (node == NULL)
        {
            MicroXcp_ReportError(XCP_ERR_CMD_UNKNOWN);
        }
    }
    else if (((this->frame.byte.pid >= 0xD3 && this->frame.byte.pid <= 0xDD) || (this->frame.byte.pid >= 0xE0 && this->frame.byte.pid <= 0xE2)))
    {
        MicroXcp_FindPid_t *node = this->daq.pid_list;
        while (node)
        {
            if (node->pid == this->frame.byte.pid)
            {
                node->func();
                break;
            }
            node = node->next;
        }

        if (node == NULL)
        {
            MicroXcp_ReportError(XCP_ERR_CMD_UNKNOWN);
        }
    }
    else
    {
        MicroXcp_ReportError(XCP_ERR_CMD_UNKNOWN);
    }

CLEANUP:
    if (this->sta.daq_run)
    {
        MicroXcp_DaqHandler(this->daq.tick++);
    }

    this->ready.en = false;
    memset(this->frame.data, 0, sizeof(MicroXcp_Frame_t));

    // return MICROXCP_OK;
}

void MicroXcp_ReceiveCallback(uint8_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > sizeof(this->frame))
    {
        MicroXcp_ReportError(XCP_ERR_GENERIC);
        return;
    }

    memcpy(this->frame.data, data, len);
    this->ready.en = true;
    this->ready.len = len;
}
