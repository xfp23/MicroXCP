/**
 * @file MicroXcp_conf.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-03-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef MICROXCP_CONF_H
#define MICROXCP_CONF_H


#ifdef __cplusplus
extern "C"
{
#endif

#define MICROXCP_VERSION "0.0.1"

/* CTO配置区域 */

/**
 * @brief 字节序配置 
 * 
 * @note 1 : motolora
 * @note 0 : intel (本库是intel)
 * 
 */
#define MICROXCP_CTO_BYTEORDER 0

/**
 * @brief 标定 读写内存功能支持 (本库支持，用户可以选择使能与禁用)
 * 
 *  1 : 支持
 *  0 : 不支持
 * 
 */
#define MICROXCP_SUPPORT_CALIBRATION 1

/**
 * @brief DAQ数据采集功能 (本库支持，用户可以选择使能与禁用)
 * 
 *  1 : 支持
 *  0 : 不支持
 * 
 */
#define MICROXCP_SUPPORT_DAQ 1

/**
 * @brief 标定保护
 * 
 */
#define MICROXCP_PROTECT_CAL 0



#ifdef __cplusplus
}
#endif


#endif