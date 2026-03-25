/**
 * @file MicroXcp_private.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-03-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "MicroXCP.h"
#include "MicroXCP_private.h"
#include "string.h"

// /**
//  * @brief 同步
//  * 
//  */
// static inline void MicroXcp_SynchErr()
// {
//     uint8_t err_res[8] = {0xFE,0x01};

//     MicroXcp_Transmit(err_res,8);
// }

/* cto*/

void MicroXcp_ConnectResFunc()
{
    MicroXcp_ConnectRes_t con_res = {
        .byte.res_succ = 0xFF,
        .byte.Calib_bit = MICROXCP_SUPPORT_CALIBRATION,
        .byte.Daq_bit = MICROXCP_SUPPORT_DAQ,
        .byte.Stim_bit = 0,
        .byte.Pgm_bit = 0,
        .byte.block_bit = 0, // 保留意见
        .byte.blockmode_bit = 0,
        .byte.max_cto_lsb = 8,
        .byte.max_cto_msb = 0,
        .byte.max_dto = 8,
        .byte.proto_ver = 0x01,
        .byte.trans_ver = 0x01,
        .byte.byte_order = MICROXCP_CTO_BYTEORDER,
        .byte.addrex_bit = 0,
    };
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

/**
 * @brief 断开连接pid响应
 *
 */
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

/** MTO */

void MicroXcp_SetMatResFunc()
{
    this->mem.ext = this->frame.byte.payload[0];
    this->mem.address = (uint32_t)((uint32_t)this->frame.byte.payload[1] << 24 | (uint32_t)this->frame.byte.payload[2] << 16 | (uint16_t)this->frame.byte.payload[3] << 8 | \
    this->frame.byte.payload[4]);

    uint8_t res[8] = {0xFF};
    MicroXcp_Transmit(res, 8);
}

/**
 * @brief 向主机上传数据
 * 
 */
void MicroXcp_UploadResFunc()
{
    this->mem.r_len = this->frame.byte.payload[0];
    if (this->mem.r_len > 7)
    {
        MicroXcp_ReportError(XCP_ERR_CMD_SYNTAX);
        return;
    }
    uint8_t data[8] = {0xFF, 0x00};
    memcpy((void *)this->mem.Cache_Byte, (void *)this->mem.address, this->mem.r_len); // 拷贝数据
    memcpy((void *)&data[1], (void *)&this->mem.Cache_Byte, this->mem.r_len);
    MicroXcp_Transmit(data, 8);
    this->mem.address += this->mem.r_len;
}

// /**
//  * @brief 处理 UPLOAD (0xF5) 命令：向主机上传内存数据
//  */
// void MicroXcp_UploadResFunc()
// {
//     // 1. 获取要读取的字节数
//     uint8_t len = this->frame.byte.payload[0];

//     // 2. 边界检查：传统 CAN 除去 PID(0xFF) 最多传 7 字节
//     if (len > 7) 
//     {
//         MicroXcp_SendErr(0x22); // ERR_OUT_OF_RANGE (超出范围)
//         return;
//     }

//     // // 3. 安全检查：检查 MTA 地址是否在合法的 RAM/Flash 范围内！
//     // if (!MicroXcp_IsAddressValid(this->mem.address, len))
//     // {
//     //     MicroXcp_SendErr(0x24); // ERR_ACCESS_DENIED (地址非法)
//     //     return;
//     // }

//     // 4. 构造响应报文（自动清零防止脏数据）
//     uint8_t data[8] = {0};
//     data[0] = 0xFF; // RES 肯定应答

//     // 5. 安全拷贝：不需要中间变量 Cache_Byte，直接从 MTA 拷贝到发送 buffer
//     // 使用 void* 强转读取
//     memcpy((void*)&data[1], (const void*)this->mem.address, len);

//     // 6. 发送数据
//     MicroXcp_Transmit(data, 8);

//     // 7. 协议灵魂：MTA 自动累加，为下一次读取做准备！
//     this->mem.address += len; 
// }

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
    uint32_t addr = (uint32_t)((uint32_t)this->frame.data[3] << 24 | (uint32_t)this->frame.data[4] << 16 | (uint16_t)this->frame.data[5] << 8 | this->frame.data[6]);
    uint8_t res[8] = {0};
    res[0] = 0xFF;

    memcpy((void*)&res[1],(void*)addr,size);
    MicroXcp_Transmit(res,8);
}

void MicroXcp_DownloadResFunc()
{
    uint8_t size = this->frame.byte.payload[0]; 

    if (size > 6) 
    {
        MicroXcp_SendError(XCP_ERR_CMD_SYNTAX);
        return;
    }

    memcpy((void*)this->mem.address, (void*)&this->frame.byte.payload[1], size);

    // 5. MTA 自动递增
    this->mem.address += size;

    // 6. 回复 RES (0xFF)
    uint8_t res[8] = {0xFF, 0x00};
    MicroXcp_Transmit(res,8);
}