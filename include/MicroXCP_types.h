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

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum 
{
    MICROXCP_OK,
    MICROXCP_ERR,
}MicroXcp_Status_t;

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
    
}MicroXcp_RegisterTable_t;

typedef struct MicroXcp_FindPid
{
    uint8_t pid;
    void (*func)(void);

    struct MicroXcp_FindPid *next;
}MicroXcp_FindPid_t;

typedef struct 
{
MicroXcp_FindPid_t list[4]; // 暂定链表大小
}MicroXcp_Cto_t;

typedef struct
{
    bool en;
    size_t len;
} MicroXcp_Ready_t;

typedef struct
{
    MicroXcp_Ready_t ready;
    MicroXcp_Frame_t frame;

    MicroXcp_Cto_t cto;

} MicroXcp_Obj_t; // xcp对象

#ifdef __cplusplus
}
#endif

#endif