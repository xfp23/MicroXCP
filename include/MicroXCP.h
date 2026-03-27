/**
 * @file    MicroXcp.h
 * @author  https://github.com/xfp23
 * @brief   Public API for MicroXCP stack
 * @version 0.1.0
 * @date    2026-03-20
 *
 * @details
 * This file provides the public interface for the MicroXCP protocol stack.
 * Users should include this header to interact with the stack.
 *
 * The implementation is platform-independent. User must provide transport
 * layer integration (e.g. CAN, UART, Ethernet).
 *
 * @note
 * - This is a non-thread-safe module unless explicitly protected by user.
 * - All APIs are designed for embedded real-time systems.
 *
 * @copyright
 * Copyright (c) 2026
 */

#ifndef MICROXCP_H
#define MICROXCP_H

#include "MicroXcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================
 * MACROS
 *==============================================================*/

/**
 * @brief Weak symbol definition
 * 
 * Used to allow user override of default implementation.
 */
#define MICROXCP_WEAK __attribute__((weak))

/*==============================================================
 * API FUNCTIONS
 *==============================================================*/

/**
 * @brief Initialize the MicroXCP module
 *
 * This function initializes internal states, DAQ lists,
 * and protocol context.
 *
 * @note Must be called once before any other API.
 */
extern void MicroXcp_Init(void);

/**
 * @brief Periodic handler of MicroXCP
 *
 * This function must be called periodically based on
 * configured cycle (e.g. 1ms).
 *
 * It handles:
 * - Protocol timeout
 * - DAQ scheduling
 * - Internal state machine
 *
 * @return MicroXcp_Status_t
 *         Status of the protocol execution
 */
void MicroXcp_TimerHandler(void);

/**
 * @brief Transmit function (to be implemented by user)
 *
 * This function is responsible for sending XCP frames
 * via the underlying transport layer (e.g. CAN, UART).
 *
 * @param data Pointer to transmit buffer
 * @param size Length of data in bytes
 *
 * @return int
 *         0  : success
 *         !=0 : error
 *
 * @note
 * - This function can be overridden by user.
 * - Must be non-blocking or bounded-time execution.
 */
extern int MICROXCP_WEAK MicroXcp_Transmit(uint8_t *data, size_t size);

/**
 * @brief Receive callback (to be called by user)
 *
 * This function shall be called by the user when a valid
 * XCP frame is received from transport layer.
 *
 * @param data Pointer to received data buffer
 * @param len  Length of received data
 *
 * @note
 * - This function runs in interrupt or task context.
 * - Keep execution short and efficient.
 */
extern void MicroXcp_ReceiveCallback(uint8_t *data, size_t len);

/*==============================================================
 * END
 *==============================================================*/

#ifdef __cplusplus
}
#endif

#endif /* MICROXCP_H */