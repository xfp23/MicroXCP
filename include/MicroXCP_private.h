/**
 * @file MicroXcp_private.h
 * @author https://github.com/xfp23
 * @brief 
 * @version 0.1
 * @date 2026-03-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef MICROXCP_PRIVATE_H
#define MICROXCP_PRIVATE_H


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum 
{
    /* --- 协议层通用错误 --- */
    XCP_ERR_CMD_SYNCH          = 0x00, /* 同步错误：命令未预期（例如：未连接就发命令） */
    XCP_ERR_CMD_UNKNOWN        = 0x20, /* 未知命令：从机未实现该 PID */
    XCP_ERR_CMD_SYNTAX         = 0x21, /* 语法错误：命令长度不对或参数格式错误 */
    XCP_ERR_OUT_OF_RANGE       = 0x22, /* 范围溢出：请求的参数（如地址）超出合法范围 */
    XCP_ERR_WRITE_PROTECTED    = 0x23, /* 写保护：尝试写入只读区域 */
    XCP_ERR_ACCESS_DENIED      = 0x24, /* 访问拒绝：未解锁（Seed & Key） */
    XCP_ERR_ACCESS_LOCKED      = 0x25, /* 访问锁定：资源当前不可用 */
    XCP_ERR_PAGE_NOT_VALID     = 0x26, /* 页面无效：标定页设置错误 */
    XCP_ERR_MODE_NOT_VALID     = 0x27, /* 模式无效：当前状态不支持该模式 */
    XCP_ERR_SEGMENT_NOT_VALID  = 0x28, /* 段无效：内存段索引错误 */
    XCP_ERR_SEQUENCE           = 0x29, /* 序列错误：Block传输中的序列号不匹配 */
    XCP_ERR_DAQ_CONFIG         = 0x2A, /* DAQ配置错误：设置的DAQ参数不合法 */

    /* --- 内存/编程相关错误 --- */
    XCP_ERR_MEMORY_OVERFLOW    = 0x30, /* 内存溢出 */
    XCP_ERR_GENERIC            = 0x31, /* 通用错误：未定义的内部错误 */
    XCP_ERR_VERIFY             = 0x32  /* 校验错误：程序写入后校验失败 */
} MicroXcp_ErrorCode_t;

typedef enum 
{
    CONNECT = 0xFF, 
    /*
    ================== CONNECT ==================
    功能：建立XCP连接（主机第一次握手）

    ------------------ 请求 ------------------
    [0] 0xFF           // 命令码
    [1] mode           // 通信模式（一般填0，不用管）

    ------------------ 响应 ------------------
    [0] 0xFF           // RES（表示成功）

    [1] resource       // 功能支持位（重点！！）
        bit0 = 1 → 支持 CAL（Calibration，标定 = 读写内存）
        bit1 = 1 → 支持 DAQ（数据采集）
        bit2 = 1 → 支持 STIM（主机往ECU实时写数据，一般不用）
        bit3 = 1 → 支持 PGM（Flash编程）
        例：
            0x01 → 只支持标定
            0x05 → 支持标定 + DAQ（最常见）

    [2] comm_mode      // 通信模式（重点！！）
        bit7 = 1 → 大端模式（Motorola，高字节在前）
        bit6 = 1 → 支持地址扩展（一般不用）
        bit5 = 1 → 支持分块传输（block mode，大数据用）
        bit0 = 1 → 支持从机block模式（可以连续发数据）

        常见值：
        0x81 = 1000 0001
             ↑       ↑
             |       └─ 支持block模式
             └─ 大端

    [3] max_cto_high
    [4] max_cto_low
        → 最大CTO长度（单位：字节）
        CAN下通常 = 8

    [5] max_dto
        → 最大DTO长度（DAQ数据长度）
        CAN下通常 = 8

    [6] proto_ver
        → 协议层版本（一般填0x01）

    [7] trans_ver
        → 传输层版本（CAN一般填0x01）
    */

    DISCONNECT = 0xFE, 
    /*
    ================== DISCONNECT ==================
    功能：断开XCP连接

    请求：
    [0] 0xFE

    响应：
    [0] 0xFF
    */

    GET_STATUS = 0xFD, 
    /*
    ================== GET_STATUS ==================
    功能：获取当前ECU状态

    请求：
    [0] 0xFD

    响应：
    [0] 0xFF

    [1] session_status（会话状态）
        bit0 = 1 → 已连接
        bit5 = 1 → DAQ正在运行（正在发数据）
        例：
            1 → DAQ运行中
            0 → DAQ不运行

    [2] protection（保护状态）
        bit0 = 1 → CAL受保护（不能写）
        bit1 = 1 → DAQ受保护
        bit2 = 1 → PGM受保护
        一般：
            0x00 → 没有保护（开发阶段）

    [3~5] reserved
        → 保留字段，一般填0

    [6] proto_ver
    [7] trans_ver
    */

    SYNCH = 0xFC, 
    /*
    ================== SYNCH ==================
    功能：通信同步（当通信异常时恢复）

    请求：
    [0] 0xFC

    响应：
    [0] 0xFF
    */
}MicroXcp_StandCmd_t;


typedef enum 
{
    SET_MTA = 0xF6, 
    /*
    ================== SET_MTA ==================
    功能：设置内存指针（MTA = 当前操作地址）

    请求：
    [0] 0xF6

    [1] addr_ext       // 地址扩展（多段内存用，一般填0） payload 0
    [2] addr[31:24]    // 高字节 payload 1
    [3] addr[23:16] payload 2
    [4] addr[15:8] payload 3
    [5] addr[7:0]      // 低字节 payload 4

    响应：
    [0] 0xFF

    说明：
    后续UPLOAD/DOWNLOAD都从这个地址开始
    */

    UPLOAD = 0xF5, 
    /*
    ================== UPLOAD ==================
    功能：从ECU读取内存

    请求：
    [0] 0xF5
    [1] size           // 读取多少字节（最大7）

    响应：
    [0] 0xFF
    [1~] data          // 读出的数据

    说明：
    每读一次，MTA自动增加 size
    */

    SHORT_UPLOAD = 0xF4, 
    /*
    ================== SHORT_UPLOAD ==================
    功能：直接读指定地址（不依赖MTA）

    请求：
    [0] 0xF4
    [1] size
    [2] addr_ext
    [3~6] address

    响应：
    [0] 0xFF
    [1~] data
    */

    DOWNLOAD = 0xF0,
    /*
    ================== DOWNLOAD ==================
    功能：写内存（标定核心）

    请求：
    [0] 0xF0
    [1~] data          // 写入的数据

    响应：
    [0] 0xFF

    说明：
    写完后MTA自动递增
    */
   SHORT_DOWNLOAD = 0xED,
   /**
    * @brief 直接写内存，单次写，不依赖MTA
    * 
    *  
    *   Byte0: 0xED       // PID
        Byte1: size       // 写入字节数（1~6）
        Byte2: addr_ext   // 地址扩展（一般0）
        Byte3: addr[31:24]
        Byte4: addr[23:16]
        Byte5: addr[15:8]
        Byte6: addr[7:0]
        Byte7~ : data
    */

    DOWNLOAD_NEXT = 0xEF,
    /*
    ================== DOWNLOAD_NEXT ==================
    功能：连续写数据（大数据分包）

    请求：
    [0] 0xEF
    [1~] data

    响应：
    [0] 0xFF
    */

    DOWNLOAD_MAX = 0xEE,
    /*
    ================== DOWNLOAD_MAX ==================
    功能：一次写满最大长度

    请求：
    [0] 0xEE
    [1~] data

    响应：
    [0] 0xFF
    */
}MicroXcp_MemoryCmd_t;

typedef enum 
{
    GET_DAQ_RESOLUTION_INFO = 0xD9,
    // GET_DAQ_PROCESSOR_INFO  = 0xD7,
    GET_DAQ_PROCESSOR_INFO  = 0xDA,
    /*
    ================== GET_DAQ_PROCESSOR_INFO  ==================
    功能：查询DAQ能力（支持多少采集）

    请求：
    [0] 0xD7

    响应：
    [0] 0xFF
    [1] daq_count      // 支持多少DAQ列表
    [2] odt_count      // 每个DAQ有多少ODT（数据包）
    [3] entry_size     // 每个ODT最多多少字节
    */

    SET_DAQ_PTR = 0xE2,
    /*
    ================== SET_DAQ_PTR ==================
    功能：设置DAQ配置位置（类似写指针）

    请求：
    [0] 0xE2
    [1] daq_list   // 第几个DAQ
    [2] odt        // 第几个数据包
    [3] entry      // 第几个变量

    响应：
    [0] 0xFF
    */

    WRITE_DAQ = 0xE1,
    /*
    ================== WRITE_DAQ ==================
    功能：往DAQ里添加一个采集变量

    请求：
    [0] 0xE1
    [1] addr_ext
    [2~5] address  // 变量地址
    [6] size       // 变量大小

    响应：
    [0] 0xFF

    说明：
    相当于告诉ECU：“我要采集这个变量”
    */

    SET_DAQ_LIST_MODE = 0xE0,
    /*
    ================== SET_DAQ_LIST_MODE ==================
    功能：设置DAQ触发方式

    请求：
    [0] 0xE0
    [1] mode
        bit0 = 1 → 启用DAQ
        bit1 = 1 → 按事件触发（比如1ms任务）
    [2] event_channel
        → 事件号（你自己定义，比如1ms任务=0）

    响应：
    [0] 0xFF
    */

    START_STOP_DAQ_LIST = 0xDE,
    /*
    ================== START_STOP_DAQ_LIST ==================
    功能：启动/停止DAQ

    请求：
    [0] 0xDE
    [1] mode
        0 = 停止
        1 = 启动
    [2] daq_list

    响应：
    [0] 0xFF

    说明：
    启动后ECU会开始发DTO数据（实时数据流）
    */

    START_STOP_SYNCH = 0xDD,
    /*
    ================== START_STOP_SYNCH ==================
    功能：同步启动/停止所有已选择的 DAQ 列表

    请求：
    [0] 0xDD
    [1] mode
        0 = 停止所有 (Stop All)
        1 = 开启所有已选择的 (Start Selected)
        2 = 停止所有已选择的 (Stop Selected)
        3 = 准备同步启动 (Prepare for Synchronous Start)

    响应：
    [0] 0xFF

    说明：
    1. 这个指令是“群发”开关。
    2. 它只影响那些之前通过 0xE0 (SET_DAQ_LIST_MODE) 把 bit0 设为 1 (Selected) 的列表。
    3. 如果你的 MicroXcp 追求简单，上位机发 1 时，你遍历所有列表把 is_running 改成 1 即可。
    */
}MicroXcp_DaqCmd_t;


// typedef union 
// {
//     struct __attribute__((packed)) 
//     {
//         // [0]
//         uint8_t res_succ; // 响应成功 固定值： 0xFF

//         // [1]
//         uint8_t Calib_bit :1; // 标定功能支持位 （给予支持）
//         uint8_t Daq_bit :1; // DAQ功能支持位 （给予支持)
//         uint8_t Stim_bit :1; // 主机实时写数据功能支持位 (本库不予支持)
//         uint8_t Pgm_bit :1; // PGM flash编程支持位 (本库不予支持)
//         uint8_t :4;

//         // [2]
//         uint8_t block_bit :1; // 从机block模式 (连续发数据) （不予支持）
//         uint8_t :4;
//         uint8_t blockmode_bit :1; // 分块传输 （不予支持）
//         uint8_t addrex_bit :1; // 地址扩展 （不予支持）
//         uint8_t byte_order :1; // 字节序 1 : motorola | 0 : intel

//         // [3] [4]
//         uint8_t max_cto_msb :8;
//         uint8_t max_cto_lsb :8; // 高低字节 最大cto长度 can下通常8字节

//         // [5]
//         uint8_t max_dto :8; // 最大Dto长度 Can下通常8字节

//         // [6]
//         uint8_t proto_ver :8; // 协议层版本 固定 0x01

//         // [7]
//         uint8_t trans_ver :8; // 传输层版本 固定 0x01
//     }byte;

//     uint8_t data[8]; // 一帧

// }MicroXcp_ConnectRes_t;// xcp连接响应 

typedef union 
{
    struct __attribute__((packed)) 
    {
        // [0] PID: 总是 0xFF (Res Success)
        uint8_t res_succ; 

        // [1] Resource
        uint8_t Calib_bit :1; // Bit 0
        uint8_t Daq_bit   :1; // Bit 1
        uint8_t Stim_bit  :1; 
        uint8_t Pgm_bit   :1; 
        uint8_t reserved1 :4;

        // [2] Comm Mode Basic
        uint8_t byte_order    :1; // Bit 0: 0=Intel, 1=Motorola
        uint8_t address_ext   :1; // Bit 1
        uint8_t block_mode    :1; // Bit 2
        uint8_t reserved2     :3;
        uint8_t optional_data :1; // Bit 6
        uint8_t slave_block   :1; // Bit 7

        // [3] MAX_CTO: 控制帧最大长度
        uint8_t max_cto; 

        // [4] MAX_DTO_LSB: 数据帧最大长度低字节
        uint8_t max_dto_lsb; 

        // [5] MAX_DTO_MSB: 数据帧最大长度高字节
        uint8_t max_dto_msb; 

        // [6] Protocol Layer Version
        uint8_t proto_ver; 

        // [7] Transport Layer Version
        uint8_t trans_ver; 
    } byte;

    uint8_t data[8];
} MicroXcp_ConnectRes_t;


typedef union {
    struct __attribute__((packed))
    {
        uint8_t res :8; // 响应字节，固定 0xFF
        uint8_t con_sta :1; // 连接状态 1 : 已连接 0 : 未连接
        uint8_t:4;
        uint8_t daq_sta :1; // 1 : DAQ运行中
        uint8_t:2;
        uint8_t protect_cal :1; // 标定保护
        uint8_t protect_daq :1; // daq保护
        uint8_t protect_pgm :1; // pgm受保护
        uint32_t :29;
        uint8_t proto_ver:8;
        uint8_t trans_ver:8; 
    }byte;

    uint8_t data[8];
}MicroXcp_GetStaRes_t;

extern MicroXcp_Obj_t * const this;

/**
 * @brief 连接pid响应
 * 
 */
extern void MicroXcp_ConnectResFunc(void);

extern void MicroXcp_DisConnectResFunc(void);

extern void MicroXcp_GetStatusResFunc(void); 

extern void MicroXcp_SynchResFunc(void);

extern void MicroXcp_SetMatResFunc(void);

extern void MicroXcp_UploadResFunc(void); 

extern void MicroXcp_ReportError(MicroXcp_ErrorCode_t err); 

extern void MicroXcp_ShortUploadResFunc(void);

extern void MicroXcp_DownloadResFunc(void);

extern void MicroXcp_GetDaqSizeResFunc(void);

extern void MicroXcp_SetDaqPtrResFunc(void); 

extern void MicroXcp_WriteDaqResFunc(void);

extern void MicroXcp_SetDaqModeResFunc(void);

extern void MicroXcp_StartDaqListResFunc(void);

extern void MicroXcp_StartSyncResFunc(void);

extern void MicroXcp_DaqResolutionInfoResFunc(void);

extern void MicroXcp_ShortDownLoadResFunc(void);

#ifdef __cplusplus
}
#endif


#endif