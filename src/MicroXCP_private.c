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

void MicroXcp_GetDaqSizeResFunc()
{
    uint8_t res[8] = {0};

    res[0] = 0xFF; // 响应pid
    res[1] =  (uint8_t)(MICROXCP_FEATURE_DAQ | 0x00 << 1 | 0x00 << 2 | 0x00); // bit0: DAQ支持 bit1: STIM支持 bit2:DAQ类型：(0 : 静态 1:动态)
    res[2] = MICROXCP_DAQ_LIST_COUNT;
    res[3] = 2; // 最大事件通道数量
    res[4] = 0;

}

void MicroXcp_SetDaqPtrResFunc()
{
    uint8_t daq_count = this->frame.data[1];
    uint8_t odt_count = this->frame.data[2];
    uint8_t entry_count = this->frame.data[3];

    if(daq_count >= MICROXCP_DAQ_LIST_COUNT || odt_count >= MICROXCP_DAQ_ODT_COUNT || entry_count >= MICROXCP_DAQ_ODT_DATA_SIZE )
    {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }
    
    this->daq.ptr_daq = daq_count;
    this->daq.ptr_odt = odt_count;
    this->daq.ptr_entry = entry_count;

    uint8_t res[8] = {0};
    res[0] = 0xFF;
    MicroXcp_Transmit(res,8);
}

void MicroXcp_WriteDaqResFunc()
{
    uint8_t daq = this->daq.ptr_daq;
    uint8_t odt = this->daq.ptr_odt;
    uint8_t entry = this->daq.ptr_entry;

    if(daq >= MICROXCP_DAQ_LIST_COUNT || odt >= MICROXCP_DAQ_ODT_COUNT || entry >= MICROXCP_DAQ_ODT_DATA_SIZE) {
        MicroXcp_ReportError(XCP_ERR_WRITE_PROTECTED); 
        return;
    }

    //Byte 2 是 Size，Byte 4~7 是 Address
    uint8_t size = this->frame.data[2];
    uint32_t addr = (uint32_t)((uint32_t)this->frame.data[7] << 24 | (uint32_t)this->frame.data[6] << 16 | (uint16_t)this->frame.data[5] << 8 | this->frame.data[4]);

    MicroXcp_Entry_t* pEntry = &this->daq.daq_list[daq].odts[odt].entries[entry];
    MicroXcp_Odt_t* pOdt = &this->daq.daq_list[daq].odts[odt];

    pEntry->addr = addr;
    pEntry->size = size;

    // 更新当前 ODT 实际塞入的元素个数
    if (entry >= pOdt->entry_count) {
        pOdt->entry_count = entry + 1; 
    }

    //写完一个之后，指针必须自动自增
    this->daq.ptr_entry++;
    if (this->daq.ptr_entry >= MICROXCP_DAQ_ODT_DATA_SIZE) {
        this->daq.ptr_entry = 0;
        this->daq.ptr_odt++;
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;

    MicroXcp_Transmit(res,8);
}

void MicroXcp_SetDaqModeResFunc()
{
    /* * [标准协议解析]
     * Byte 1: Mode (bit 0 是使能位)
     * Byte 2-3: DAQ List Number (此处按照小端处理，即 data[2] 是低字节)
     * Byte 4-5: Event Channel
     */
    
    // 1. 获取目标 DAQ List 索引 (Byte 2-3)
    uint16_t daq_list_idx = (uint16_t)(this->frame.data[3] | ((uint16_t)this->frame.data[2] << 8));

    // 2. 边界检查
    if (daq_list_idx >= MICROXCP_DAQ_LIST_COUNT) {
        MicroXcp_ReportError(XCP_ERR_OUT_OF_RANGE);
        return;
    }

    MicroXcp_DaqObj_t* pDaq = &this->daq.daq_list[daq_list_idx];

    // 3. 设置 Mode (Byte 1)
    uint8_t mode = this->frame.data[1];
    pDaq->en = (mode & 0x01); // Bit 0: 1=Selected for session, 0=Not selected

    // 4. 设置 Event Channel
    pDaq->event_channel = (uint16_t)(this->frame.data[5] <<8 | ((uint16_t)this->frame.data[4]));

    // 5. 构造正响应
    uint8_t res[8] = {0};
    res[0] = 0xFF; // Packet ID: RES (Success)

    MicroXcp_Transmit(res, 8);
}


void MicroXcp_StartDaqListResFunc()
{
    uint8_t daq_index = this->frame.data[2];
    uint8_t daq_mode = this->frame.data[1];
    
    if (daq_index < MICROXCP_DAQ_LIST_COUNT) {
        this->daq.daq_list[daq_index].is_running = (daq_mode == 0x02) ? 1 : 0; // 标准 Start=2, Stop=0
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;

    MicroXcp_Transmit(res,8);
}

void MicroXcp_StartSyncResFunc()
{
    for(int i = 0; i < MICROXCP_DAQ_LIST_COUNT; i++)
    {
        if(this->daq.daq_list[i].en == 1) 
        {
            this->daq.daq_list[i].is_running = 1;
        }
    }

    uint8_t res[8] = {0};
    res[0] = 0xFF;

    MicroXcp_Transmit(res,8);
}

void MicroXcp_DaqResolutionInfoResFunc()
{
    uint8_t res[8] = {0};

    res[0] = 0xFF;
    res[1] = 0x01; // 1 : 按字节访问 2 : 按16bit访问 3 : 按32bit访问
    res[2] = 0x07; // 单个 entry 最大支持多少字节
    res[3] = 0x00; // TIMESTAMP_MODE 是否支持时间戳
    res[4] = 0x00; // 时间分辨率（单位）
    res[5] = 0x00; // 时间戳占几个字节

    MicroXcp_Transmit(res,8);
}