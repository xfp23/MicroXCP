/**
 * @file MicroXcp_private.c
 */

#include "MicroXCP.h"
#include "MicroXCP_private.h"
#include "string.h"
#include "MicroXCP_types.h"

/* CTO */

void MicroXcp_ConnectResFunc()
{
    MicroXcp_ConnectRes_t con_res = {0};

    con_res.byte.res_succ = 0xFF;
    con_res.byte.Calib_bit = 1;
    con_res.byte.Daq_bit = 1;

    con_res.byte.byte_order = MICROXCP_CTO_BYTE_ORDER;
    con_res.byte.max_cto = 8;     // Byte 3
    con_res.byte.max_dto_lsb = 8; // Byte 4
    con_res.byte.max_dto_msb = 0; // Byte 5

    con_res.byte.proto_ver = 0x01;
    con_res.byte.trans_ver = 0x01;
    int ret = MicroXcp_Transmit(con_res.data, sizeof(con_res.data));

    if (ret == 0)
    {
        this->sta.con_sta = 1;
        this->sta.daq_run = 0;
    }
    else
    {
        this->sta.con_sta = 0;
        this->sta.daq_run = 0;
    }
}

void MicroXcp_DisConnectResFunc()
{
    uint8_t res[8] = {0xFF};
    this->sta.con_sta = 0;
    this->sta.daq_run = 0;

    MicroXcp_Transmit(res, 8);
}

void MicroXcp_GetStatusResFunc()
{
    MicroXcp_GetStaRes_t sta_res = {
        .byte.con_sta = this->sta.con_sta,
        .byte.daq_sta = this->sta.daq_run,
        .byte.proto_ver = 0x01,
        .byte.trans_ver = 0x01,
        .byte.protect_cal = this->sta.cal_protect,
        .byte.protect_daq = this->sta.daq_protect,
        .byte.protect_pgm = this->sta.pgm_protect,
        .byte.res = 0xFF,
    };

    MicroXcp_Transmit(sta_res.data, sizeof(MicroXcp_GetStaRes_t));
}

void MicroXcp_SynchResFunc()
{
    uint8_t res[8] = {0xFF};
    MicroXcp_Transmit(res, 8);
}

/* MTO */

void MicroXcp_SetMatResFunc()
{
    this->mem.ext = this->frame.data[3];
    this->mem.address = (uint32_t)((uint32_t)this->frame.data[7] << 24 | (uint32_t)this->frame.data[6] << 16 | (uint16_t)this->frame.data[5] << 8 | this->frame.data[4]);

    uint8_t res[8] = {0xFF};
    MicroXcp_Transmit(res, 8);
}

void MicroXcp_UploadResFunc()
{
    this->mem.r_len = this->frame.byte.payload[0];
    if (this->mem.r_len > 7)
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX);
        return;
    }
    uint8_t data[8] = {0xFF, 0x00};
    memcpy((void *)this->mem.Cache_Byte, (void *)this->mem.address, this->mem.r_len); 
    memcpy((void *)&data[1], (void *)&this->mem.Cache_Byte, this->mem.r_len);
    MicroXcp_Transmit(data, 8);
    this->mem.address += this->mem.r_len;
}

void MicroXcp_ReportError(MicroXcp_ErrorCode_t err)
{
    uint8_t data[8] = {0};
    data[0] = 0xFE;
    data[1] = (uint8_t)err;

    MicroXcp_Transmit(data,8);
}

void MicroXcp_ShortUploadResFunc()
{
    uint8_t size = this->frame.data[1];
    if(size > 7)
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX);
        return;
    }
    // 【重构】修复偏移量，Byte 4~7 是地址
    uint32_t addr = (uint32_t)((uint32_t)this->frame.data[7] << 24 | (uint32_t)this->frame.data[6] << 16 | (uint16_t)this->frame.data[5] << 8 | this->frame.data[4]);
    uint8_t res[8] = {0};
    res[0] = 0xFF;

    memcpy((void*)&res[1],(void*)addr,size);
    MicroXcp_Transmit(res,8);

    // 【重构】XCP 协议要求 ShortUpload 后自动更新 MTA
    this->mem.address = addr + size;
}

void MicroXcp_DownloadResFunc()
{
    uint8_t size = this->frame.byte.payload[0]; 

    if (size > 6) 
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX); 
        return;
    }

    memcpy((void*)this->mem.address, (void*)&this->frame.byte.payload[1], size);

    this->mem.address += size;

    uint8_t res[8] = {0xFF, 0x00};
    MicroXcp_Transmit(res,8);
}

void MicroXcp_ShortDownLoadResFunc()
{
    uint8_t res[8] = {0};

    uint8_t size = this->frame.data[1];

    if(size >= 6)
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX); 
        return;
    }

    uint32_t addr = this->frame.data[6] << 24 | this->frame.data[5] << 16 | this->frame.data[4] << 8 | this->frame.data[3];

    memcpy((void*)addr,&this->frame.data[7],size);
}
/* DAQ */

/* ----------------------------------------------------------
 * GET_DAQ_PROCESSOR_INFO  0xDA
 * 响应: [0xFF][GEN_PROP][MAX_DAQ_L][MAX_DAQ_H]
 *       [MAX_EVENT_L][MAX_EVENT_H][DAQ_KEY_BYTE]
 * ---------------------------------------------------------- */
void MicroXcp_GetDaqSizeResFunc()
{
    uint8_t res[8] = {0};

    res[0] = 0xFF;
    /* GEN_PROP:
     *   bit0 = DAQ 方向支持
     *   bit1 = STIM 支持（本实现不支持，填 0）
     *   bit2 = 动态 DAQ（本实现静态，填 0）
     *   bit5 = 时间戳支持（不支持，填 0）
     */
    res[1] = (uint8_t)(MICROXCP_FEATURE_DAQ);   /* 仅 bit0 置位 */
    res[2] = (uint8_t)(MICROXCP_DAQ_LIST_COUNT & 0xFF);   /* MAX_DAQ 低字节 */
    res[3] = (uint8_t)(MICROXCP_DAQ_LIST_COUNT >> 8);     /* MAX_DAQ 高字节 */
    res[4] = 2;    /* MAX_EVENT 低字节 */
    res[5] = 0;    /* MAX_EVENT 高字节 */
    res[6] = 0x01; /* DAQ_KEY_BYTE: 地址粒度 1 byte */

    MicroXcp_Transmit(res, 8);

}

/* ----------------------------------------------------------
 * SET_DAQ_PTR  0xE2
 * 主机帧: [0xE2][0x00][DAQ_L][DAQ_H][ODT_NUM][ENTRY_NUM]
 * ---------------------------------------------------------- */
void MicroXcp_SetDaqPtrResFunc(void)
{
    uint16_t daq_idx   = (uint16_t)(this->frame.data[2]
                       | ((uint16_t)this->frame.data[3] << 8));
    uint8_t  odt_idx   = this->frame.data[4];
    uint8_t  entry_idx = this->frame.data[5];

    if (daq_idx >= this->daq.alloc_daq_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* ODT 边界用该 DAQ 实际分配的 odt_count */
    if (odt_idx >= this->daq.daq_list[daq_idx].odt_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* Entry 边界用该 ODT 实际分配的 entry_count */
    if (entry_idx >= this->daq.daq_list[daq_idx].odts[odt_idx].entry_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    this->daq.ptr_daq   = (uint8_t)daq_idx;
    this->daq.ptr_odt   = odt_idx;
    this->daq.ptr_entry = entry_idx;

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}


/* ----------------------------------------------------------
 * WRITE_DAQ  0xE1
 * 主机帧: [0xE1][BIT_OFFSET][SIZE][ADDR_EXT]
 *               [ADDR_0][ADDR_1][ADDR_2][ADDR_3]
 * ---------------------------------------------------------- */
void MicroXcp_WriteDaqResFunc(void)
{
    uint8_t daq   = this->daq.ptr_daq;
    uint8_t odt   = this->daq.ptr_odt;
    uint8_t entry = this->daq.ptr_entry;

    /* 同样用动态分配的边界 */
    if (daq   >= this->daq.alloc_daq_count                       ||
        odt   >= this->daq.daq_list[daq].odt_count               ||
        entry >= this->daq.daq_list[daq].odts[odt].entry_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 其余不变 */
    uint8_t  size = this->frame.data[2];
    uint32_t addr = (uint32_t)this->frame.data[4]
                  | ((uint32_t)this->frame.data[5] << 8)
                  | ((uint32_t)this->frame.data[6] << 16)
                  | ((uint32_t)this->frame.data[7] << 24);

    if (size == 0 || size > MICROXCP_DAQ_ODT_DATA_SIZE)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    MicroXcp_Entry_t *pEntry = &this->daq.daq_list[daq].odts[odt].entries[entry];
    pEntry->addr = addr;
    pEntry->size = size;

    this->daq.ptr_entry++;
    if (this->daq.ptr_entry >= this->daq.daq_list[daq].odts[odt].entry_count)
    {
        this->daq.ptr_entry = 0;
        this->daq.ptr_odt++;
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * SET_DAQ_LIST_MODE  0xE0
 * 主机帧: [0xE0][MODE][DAQ_L][DAQ_H][EVT_L][EVT_H][PRESCALER][PRIORITY]
 * ---------------------------------------------------------- */
void MicroXcp_SetDaqModeResFunc()
{
    /* 协议小端：data[2]=低字节，data[3]=高字节 */
    uint16_t daq_list_idx   = (uint16_t)(this->frame.data[2]  | ((uint16_t)this->frame.data[3] << 8));
    uint16_t event_channel  = (uint16_t)(this->frame.data[4] | ((uint16_t)this->frame.data[5] << 8));
    uint8_t  mode           = this->frame.data[1];

    if (daq_list_idx >= MICROXCP_DAQ_LIST_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[daq_list_idx];

    /* bit0: 1=已选中（配合 START_STOP_SYNCH 用），0=未选中 */
    pDaq->en            = (mode & 0x01);
    pDaq->event_channel = (uint8_t)event_channel; /* 保持 uint8_t 存储，若通道 > 255 需扩展 */

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * START_STOP_DAQ_LIST  0xDE
 * 主机帧: [0xDE][MODE][DAQ_L][DAQ_H]
 *   MODE: 0x00=停止, 0x01=启动, 0x02=仅预选(配合SYNCH)
 * 响应:   [0xFF][FIRST_PID]
 * ---------------------------------------------------------- */
void MicroXcp_StartDaqListResFunc()
{
    uint8_t  mode     = this->frame.data[1];
    uint16_t daq_idx  = (uint16_t)(this->frame.data[2] | ((uint16_t)this->frame.data[3] << 8));

    if (daq_idx >= MICROXCP_DAQ_LIST_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[daq_idx];

    switch (mode)
    {
        case 0x00:  /* 停止 */
            pDaq->is_running = 0;
            pDaq->en         = 0;
            break;
        case 0x01:  /* 立即启动 */
            pDaq->is_running = 1;
            pDaq->en         = 1;
            break;
        case 0x02:  /* 预选，等待 START_STOP_SYNCH 统一启动 */
            pDaq->is_running = 0;
            pDaq->en         = 1;
            break;
        default:
            MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX);
            return;
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    /* FIRST_PID：该 DAQ List 第 0 个 ODT 的 PID */
    res[1] = pDaq->odts[0].pid;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * START_STOP_SYNCH  0xDD
 * 主机帧: [0xDD][MODE]
 *   MODE: 0x00=停止所有, 0x01=启动所有已选中(en==1)
 * ---------------------------------------------------------- */
void MicroXcp_StartSyncResFunc(void)
{
    uint8_t mode = this->frame.data[1];

    switch (mode)
    {
        case 0x00:  /* 停止所有正在运行的 */
            for (int i = 0; i < MICROXCP_DAQ_LIST_COUNT; i++)
            {
                this->daq.daq_list[i].is_running = 0;
            }
            break;

        case 0x01:  /* 启动所有已预选（en==1）的 */
            for (int i = 0; i < MICROXCP_DAQ_LIST_COUNT; i++)
            {
                if (this->daq.daq_list[i].en)
                {
                    this->daq.daq_list[i].is_running = 1;
                }
            }
            break;

        default:
            MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX);
            return;
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}


/* ----------------------------------------------------------
 * GET_DAQ_RESOLUTION_INFO  0xD9
 * 响应: [0xFF]
 *       [GRANULARITY_ODT_ENTRY_SIZE_DAQ]   Byte1
 *       [MAX_ODT_ENTRY_SIZE_DAQ]           Byte2
 *       [GRANULARITY_ODT_ENTRY_SIZE_STIM]  Byte3
 *       [MAX_ODT_ENTRY_SIZE_STIM]          Byte4
 *       [TIMESTAMP_MODE]                   Byte5
 *       [TIMESTAMP_TICKS_L]                Byte6
 *       [TIMESTAMP_TICKS_H]                Byte7
 * ---------------------------------------------------------- */
void MicroXcp_DaqResolutionInfoResFunc()
{
    uint8_t res[8] = {0};

    res[0] = 0xFF;
    res[1] = 0x01; /* GRANULARITY_DAQ : 按 1 字节对齐 */
    res[2] = 0x07; /* MAX_ODT_ENTRY_SIZE_DAQ : 单 entry 最多 7 字节 */
    res[3] = 0x01; /* GRANULARITY_STIM（不支持 STIM，给 1 是合法的最小值） */
    res[4] = 0x07; /* MAX_ODT_ENTRY_SIZE_STIM */
    res[5] = 0x00; /* TIMESTAMP_MODE : 不支持时间戳 */
    res[6] = 0x00; /* TIMESTAMP_TICKS 低字节 */
    res[7] = 0x00; /* TIMESTAMP_TICKS 高字节 */

    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * GET_DAQ_LIST_INFO  0xD8
 * 主机帧: [0xD8][0x00][DAQ_LIST_NUMBER_L][DAQ_LIST_NUMBER_H]
 * 响应帧: [0xFF][DAQ_LIST_PROP][MAX_ODT][FIRST_PID]
 *               [EVENT_CHANNEL_L][EVENT_CHANNEL_H][0x00][0x00]
 * ---------------------------------------------------------- */
void MicroXcp_GetDaqListInfoResFunc(void)
{
    uint16_t daq_idx = (uint16_t)(this->frame.data[2] 
                     | ((uint16_t)this->frame.data[3] << 8));

    if (daq_idx >= MICROXCP_DAQ_LIST_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[daq_idx];

    uint8_t prop = 0;
    prop |= (0      & 0x01);        /* bit0: 方向，固定 DAQ=0 */
    prop |= (0      & 0x01) << 2;   /* bit2: 事件固定=0，可配置 */
    prop |= (pDaq->is_running & 0x01) << 3; /* bit3: 运行中 */

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    res[1] = prop;
    res[2] = MICROXCP_DAQ_ODT_COUNT;       /* MAX_ODT */
    res[3] = pDaq->odts[0].pid;            /* FIRST_PID */
    res[4] = (uint8_t)(pDaq->event_channel);       /* EVENT_CHANNEL 低字节 */
    res[5] = 0x00;                         /* EVENT_CHANNEL 高字节（uint8_t 存储） */
    res[6] = 0x00;
    res[7] = 0x00;

    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * FREE_DAQ  0xD6
 * 主机帧: [0xD6][0x00]*
 * 响应帧: [0xFF][0x00]*
 * ---------------------------------------------------------- */
void MicroXcp_FreeDaqResFunc(void)
{
    MicroXcp_DaqReset();

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * GET_DAQ_EVENT_INFO  0xD7
 * 主机帧: [0xD7][0x00][EVENT_CHANNEL_L][EVENT_CHANNEL_H]
 * 响应帧: [0xFF][DAQ_EVENT_PROP][MAX_DAQ_LIST]
 *               [EVENT_CHANNEL_TIME_CYCLE]
 *               [EVENT_CHANNEL_TIME_UNIT]
 *               [EVENT_CHANNEL_PRIORITY]
 *               [NAME_LENGTH]
 *               (名称字节，若有)
 * ---------------------------------------------------------- */
void MicroXcp_GetDaqEventInfoResFunc(void)
{
    uint16_t ch = (uint16_t)(this->frame.data[2]
               | ((uint16_t)this->frame.data[3] << 8));

    if (ch >= MICROXCP_EVENT_CHANNEL_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    const MicroXcp_EventChannel_t *pEvt = &s_EventChannelTable[ch];

    uint8_t name_len = 0;
    if (pEvt->name != NULL)
    {
        /* 截断到帧剩余空间：8字节帧，前7字节已用，最多1字节名称
         * 若要支持长名称需要换成多帧，这里按 CAN 单帧限制截断 */
        size_t full_len = strlen(pEvt->name);
        name_len = (full_len > 1) ? 1 : (uint8_t)full_len;
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    res[1] = 0x00;          /* DAQ_EVENT_PROP: bit0=DAQ支持,bit1=STIM支持，静态实现固定 DAQ=1 */
    res[1] |= 0x01;         /* bit0: DAQ 方向支持 */
    res[2] = pEvt->max_daq_list;
    res[3] = pEvt->time_cycle;
    res[4] = pEvt->time_unit;
    res[5] = pEvt->priority;
    res[6] = name_len;
    if (name_len > 0)
    {
        res[7] = (uint8_t)pEvt->name[0];
    }

    MicroXcp_Transmit(res, 8);
}

void MicroXcp_DaqReset(void)
{
    uint8_t global_pid = 0;

    this->daq.alloc_daq_count = 0; 
    this->daq.alloc_entry_count = 0;
    this->daq.alloc_odt_count = 0;

    this->daq.ptr_daq         = 0;
    this->daq.ptr_odt         = 0;
    this->daq.ptr_entry       = 0;

    for (int i = 0; i < MICROXCP_DAQ_LIST_COUNT; i++)
    {
        MicroXcp_DaqObj_t *pDaq = &this->daq.daq_list[i];

        pDaq->event_channel = 0;
        pDaq->is_running    = 0;
        pDaq->en            = 0;
        pDaq->odt_count     = 0;

        for (int j = 0; j < MICROXCP_DAQ_ODT_COUNT; j++)
        {
            MicroXcp_Odt_t *pOdt = &pDaq->odts[j];

            pOdt->pid         = global_pid++;
            pOdt->entry_count = 0;

            for (int k = 0; k < MICROXCP_DAQ_ODT_DATA_SIZE; k++)
            {
                pOdt->entries[k].addr = 0;
                pOdt->entries[k].size = 0;
            }
        }
    }
}

/* ----------------------------------------------------------
 * ALLOC_DAQ  0xD5
 * 主机帧: [0xD5][0x00][DAQ_COUNT_L][DAQ_COUNT_H]
 * 响应帧: [0xFF][0x00]*
 * 前提:   必须在 FREE_DAQ 之后调用
 * ---------------------------------------------------------- */
void MicroXcp_AllocDaqResFunc(void)
{
    uint16_t req_count = (uint16_t)(this->frame.data[2]
                       | ((uint16_t)this->frame.data[3] << 8));

    if (req_count == 0 || req_count > MICROXCP_DAQ_LIST_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    this->daq.alloc_daq_count = (uint8_t)req_count;

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * ALLOC_ODT  0xD4
 * 主机帧: [0xD4][0x00][DAQ_LIST_NUMBER_L][DAQ_LIST_NUMBER_H][ODT_COUNT]
 * 响应帧: [0xFF][0x00]*
 * 前提:   必须在 ALLOC_DAQ 之后调用
 * ---------------------------------------------------------- */
void MicroXcp_AllocOdtResFunc(void)
{
    uint16_t daq_idx   = (uint16_t)(this->frame.data[2]
                       | ((uint16_t)this->frame.data[3] << 8));
    uint8_t  odt_count = this->frame.data[4];

    /* 检查 DAQ List 编号是否在本次 ALLOC_DAQ 申请的范围内 */
    if (daq_idx >= this->daq.alloc_daq_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 检查 ODT 数量是否超过静态上限 */
    if (odt_count == 0 || odt_count > MICROXCP_DAQ_ODT_COUNT)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 静态架构：记录数量，实际空间已经静态存在 */
    this->daq.daq_list[daq_idx].odt_count  = odt_count;
    this->daq.alloc_odt_count              = odt_count;

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}

/* ----------------------------------------------------------
 * ALLOC_ODT_ENTRY  0xD3
 * 主机帧: [0xD3][0x00][DAQ_LIST_NUMBER_L][DAQ_LIST_NUMBER_H]
 *               [ODT_NUMBER][ODT_ENTRIES_COUNT]
 * 响应帧: [0xFF][0x00]*
 * 前提:   必须在 ALLOC_ODT 之后调用
 * ---------------------------------------------------------- */
void MicroXcp_AllocOdtEntryResFunc(void)
{
    uint16_t daq_idx     = (uint16_t)(this->frame.data[2]
                         | ((uint16_t)this->frame.data[3] << 8));
    uint8_t  odt_idx     = this->frame.data[4];
    uint8_t  entry_count = this->frame.data[5];

    /* 检查 DAQ List 编号 */
    if (daq_idx >= this->daq.alloc_daq_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 检查 ODT 编号是否在本次 ALLOC_ODT 申请的范围内 */
    if (odt_idx >= this->daq.daq_list[daq_idx].odt_count)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 检查 Entry 数量是否超过静态上限 */
    if (entry_count == 0 || entry_count > MICROXCP_DAQ_ODT_DATA_SIZE)
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    /* 静态架构：记录数量，实际空间已经静态存在 */
    this->daq.daq_list[daq_idx].odts[odt_idx].entry_count = entry_count;
    this->daq.alloc_entry_count                           = entry_count;

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res, 8);
}
