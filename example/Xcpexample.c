/**
 * @file example.c
 * @brief Minimal example for MicroXCP integration
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "MicroXcp.h"

/*==============================================================
 * Mock Driver Layer (Replace with your platform)
 *==============================================================*/

/* Example transmit driver (e.g. CAN/UART) */
int YourDriver_Send(uint8_t *data, size_t size)
{
    /* TODO: Replace with real driver */
    (void)data;
    (void)size;

    /* return 0 for success */
    return 0;
}

/*==============================================================
 * MicroXCP Porting Layer
 *==============================================================*/

/**
 * @brief User implementation of transmit function
 */
int MicroXcp_Transmit(uint8_t *data, size_t size)
{
    return YourDriver_Send(data, size);
}

/*==============================================================
 * Example Receive Path
 *==============================================================*/

/**
 * @brief Simulated receive function
 * 
 * In real project:
 * - Call this from CAN RX interrupt or task
 * - Ensure a complete XCP frame is passed in
 */
void Example_RxIndication(uint8_t *data, uint16_t len)
{
    MicroXcp_ReceiveCallback(data, len);
}

/*==============================================================
 * System Layer (Mock)
 *==============================================================*/

void System_Init(void)
{
    /* Hardware init here */
}

/* Simulated 1ms task */
void SysTask_1ms(void)
{
    MicroXcp_TimerHandler();
}

/*==============================================================
 * Main Entry
 *==============================================================*/

int main(void)
{
    System_Init();

    /* Initialize MicroXCP */
    MicroXcp_Init();

    while (1)
    {
        /* Periodic processing */
        SysTask_1ms();

        /* ======================================================
         * Example: simulate receiving an XCP command
         * (CONNECT command: 0xFF)
         * ======================================================*/
        uint8_t exampleFrame[] = {0xFF};

        Example_RxIndication(exampleFrame, sizeof(exampleFrame));

        /* In real system:
         * - Remove this simulation
         * - Data comes from CAN/UART ISR
         */
    }
}