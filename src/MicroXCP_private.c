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

/**
 * @brief 同步
 * 
 */
static inline void MicroXcp_SynchErr()
{
    uint8_t err_res[8] = {0xFE,0x01};

    MicroXcp_Transmit(err_res,8);
}

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

void MicroXcp_UploadResFunc()
{
    this->mem.r_len = this->frame.byte.payload[0];
    if(this->mem.r_len > 7)
    {
        MicroXcp_SynchErr(); // 报个错先
        return;
    }
}