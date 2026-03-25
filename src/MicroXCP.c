#include "MicroXcp.h"
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
    {.pid = SHORT_UPLOAD, .func = NULL},
    {.pid = DOWNLOAD, .func = NULL},
    {.pid = DOWNLOAD_NEXT, .func = NULL},
};

/**
 * @brief 错误响应，pid不支持
 *
 */
static inline void MicroXcp_UnKnownPidErr()
{
    uint8_t res[8] = {0xFE,0x01};
    MicroXcp_Transmit(res,8);
}

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

void MicroXcp_Init()
{
    MicroXcp_CtoInit();
    MicroXcp_MemInit();

    this->sta.cal_protect = MICROXCP_PROTECT_CAL;
    this->sta.daq_protect = MICROXCP_PROTECT_CAL;
    this->sta.pgm_protect = MICROXCP_PROTECT_CAL;
}

int MICROXCP_WEAK MicroXcp_Transmit(uint8_t *data, size_t size)
{
    (void)data;
    (void)size;

    return 0;
}

MicroXcp_Status_t MicroXcp_TimerHandler()
{

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
            // 没找到回复不支持pid
            MicroXcp_UnKnownPidErr();
        }
    }
    else if (this->frame.byte.pid >= 0xF6 && this->frame.byte.pid <= 0xEE)
    { // 查找Mem

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
            // 没找到回复不支持pid
            MicroXcp_UnKnownPidErr();
        }
    }
    else if (this->frame.byte.pid >= 0xD7 && this->frame.byte.pid <= 0xDE)
    {
        // 查找DTO
    }else {
        MicroXcp_UnKnownPidErr();
    }

    this->ready.en = false;
    memset(this->frame.data, 0, sizeof(MicroXcp_Frame_t));

    return MICROXCP_OK;
}

void MicroXcp_ReceiveCallback(uint8_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > sizeof(this->frame))
    {
        return;
    }

    memcpy(this->frame.data, data, len);
    this->ready.en = true;
    this->ready.len = len;
}
