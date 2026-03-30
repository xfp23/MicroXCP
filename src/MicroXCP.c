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

MicroXcp_RegisterTable_t DaqTable[] = {
    {.pid = GET_DAQ_PROCESSOR_INFO, .func = MicroXcp_GetDaqSizeResFunc},
    {.pid = SET_DAQ_PTR, .func = MicroXcp_SetDaqPtrResFunc},
    {.pid = WRITE_DAQ, .func = MicroXcp_WriteDaqResFunc},
    {.pid = SET_DAQ_LIST_MODE, .func = MicroXcp_SetDaqModeResFunc},
    {.pid = START_STOP_DAQ_LIST, .func = MicroXcp_StartDaqListResFunc},
    {.pid = START_STOP_SYNCH, .func = MicroXcp_StartSyncResFunc},
    {.pid = GET_DAQ_RESOLUTION_INFO, .func = MicroXcp_DaqResolutionInfoResFunc},

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
        this->daq.pid_list[i].pid = DaqTable[i].pid;
    }

    for (int i = 0; i < size - 1; i++)
    {
        this->daq.pid_list[i].next = &this->daq.pid_list[i + 1];
    }

    this->daq.pid_list[size - 1].next = NULL;

    uint8_t global_pid = 0;

    for (int i = 0; i < MICROXCP_DAQ_LIST_COUNT; i++)
    {
        for (int j = 0; j < MICROXCP_DAQ_ODT_COUNT; j++)
        {

            this->daq.daq_list[i].odts[j].pid = global_pid++;

            this->daq.daq_list[i].odts[j].entry_count = 0;

            for (int k = 0; k < MICROXCP_DAQ_ODT_DATA_SIZE; k++)
            {
                this->daq.daq_list[i].odts[j].entries[k].addr = 0;
                this->daq.daq_list[i].odts[j].entries[k].size = 0;
            }
        }
    }
}

static void MicroXcp_DaqHandler(uint32_t current_tick_ms)
{
    // 1. 遍历所有 DAQ List
    for (uint8_t d = 0; d < MICROXCP_DAQ_LIST_COUNT; d++)
    {
        MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[d];

        // 没运行，直接跳过
        if (!pDaq->is_running)
            continue;

        // 2. 检查周期 (这里建议直接根据 event_channel 映射，比如 0=1ms, 1=10ms)
        uint32_t period = (pDaq->event_channel == 0) ? 1 : (pDaq->event_channel * 10);

        if (current_tick_ms % period == 0)
        {
            // 3. 遍历这个 List 下的所有激活的 ODT (每一帧报文)
            for (uint8_t o = 0; o < MICROXCP_DAQ_ODT_COUNT; o++)
            {
                MicroXcp_Odt_t *pOdt = &pDaq->odts[o];
                uint8_t buf[8] = {0};
                uint8_t pos = 1; // 从 Byte 1 开始存数据，Byte 0 留给 PID

                buf[0] = pOdt->pid;

                // 4. 遍历 ODT 里的变量 Entry
                for (uint8_t e = 0; e < pOdt->entry_count; e++)
                {
                    MicroXcp_Entry_t *pEntry = &pOdt->entries[e];

                    // 安全检查：防止 entry 里的数据把 buf 撑爆
                    if (pos + pEntry->size <= 8)
                    {
                        // ?  entry->addr 强制转为指针取出里面的值
                        memcpy(&buf[pos], (void *)pEntry->addr, pEntry->size);
                        pos += pEntry->size;
                    }
                }

                // 5. 组包完成，立刻吐出去
                MicroXcp_Transmit(buf, 8);
            }
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
    else if (this->frame.byte.pid >= 0xD7 && this->frame.byte.pid <= 0xDD)
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
