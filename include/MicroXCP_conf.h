/**
 * @file    MicroXcp_Conf.h
 * @author  https://github.com/xfp23
 * @brief   MicroXCP Configuration File
 * @version 0.1.0
 * @date    2026-03-20
 *
 * @note    This file is used to configure MicroXCP features and resources.
 *          Modify according to project requirements.
 *
 * @copyright
 * Copyright (c) 2026
 */

#ifndef MICROXCP_CONF_H
#define MICROXCP_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================
 * VERSION
 *==============================================================*/
#define MICROXCP_VERSION_STR "0.0.1"

/*==============================================================
 * BASIC CONFIG
 *==============================================================*/

/**
 * @brief MicroXCP scheduling period (unit: ms)
 */
#define MICROXCP_MAIN_PERIOD_MS   (1U)

/**
 * @brief Byte order configuration
 * 
 * 0 : Intel (Little Endian)
 * 1 : Motorola (Big Endian)
 */
#define MICROXCP_CTO_BYTE_ORDER   (0U)

/*==============================================================
 * FEATURE SWITCH
 *==============================================================*/

/**
 * @brief Calibration (memory read/write) support
 */
#define MICROXCP_FEATURE_CALIBRATION   (1U)

/**
 * @brief DAQ data acquisition support
 */
#define MICROXCP_FEATURE_DAQ           (1U)

/**
 * @brief Calibration protection (prevent unauthorized write)
 */
#define MICROXCP_FEATURE_CAL_PROTECT   (0U)

/*==============================================================
 * DAQ CONFIG
 *==============================================================*/

#if (MICROXCP_FEATURE_DAQ == 1U)

/**
 * @brief Number of DAQ lists
 */
#define MICROXCP_DAQ_LIST_COUNT        (8U)

/**
 * @brief Number of ODTs per DAQ
 */
#define MICROXCP_DAQ_ODT_COUNT         (8U)

/**
 * @brief Data size (bytes) per ODT
 * 
 * @note Typically = CTO size - PID - reserved fields
 */
#define MICROXCP_DAQ_ODT_DATA_SIZE     (7U)

/**
 * @brief DAQ trigger period (based on main cycle multiplier)
 * 
 * Actual period = MICROXCP_MAIN_PERIOD_MS * MICROXCP_DAQ_PERIOD
 */
#define MICROXCP_DAQ_PERIOD            (1U)

#endif /* MICROXCP_FEATURE_DAQ */

/*==============================================================
 * COMPILE CHECK
 *==============================================================*/

#if (MICROXCP_CTO_BYTE_ORDER != 0U) && (MICROXCP_CTO_BYTE_ORDER != 1U)
#error "MICROXCP_CTO_BYTE_ORDER must be 0 (Intel) or 1 (Motorola)"
#endif

#if (MICROXCP_MAIN_PERIOD_MS == 0U)
#error "MICROXCP_MAIN_PERIOD_MS must be > 0"
#endif

/*==============================================================
 * END
 *==============================================================*/

#ifdef __cplusplus
}
#endif

#endif /* MICROXCP_CONF_H */