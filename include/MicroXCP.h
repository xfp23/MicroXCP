/**
 * @file MicroXcp.h
 * @author https://github.com/xfp23
 * @brief 
 * @version 0.1
 * @date 2026-03-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef MICROXCP_H
#define MICROXCP_H

#include "MicroXcp_types.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define MICROXCP_WEAK __attribute__((weak))

extern void MicroXcp_Init(void);

MicroXcp_Status_t MicroXcp_TimerHandler(void);

extern int MICROXCP_WEAK MicroXcp_Transmit(uint8_t *data, size_t size);

extern void MicroXcp_ReceiveCallback(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif