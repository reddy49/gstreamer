/* aboxIpc.h - Samsung Abox IPC header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Samsung Abox IPC specific definitions.
*/

#ifndef __INCaboxIpch
#define __INCaboxIpch

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef BIT
#define BIT(x)              (0x1u << (x))

/* system IPC */

typedef enum aboxSystemMsg
    {
    ABOX_SUSPEND = 1,
    ABOX_RESUME,
    ABOX_BOOT_DONE,
    ABOX_CHANGE_GEAR,
    ABOX_START_L2C_CONTROL,
    ABOX_END_L2C_CONTROL,
    ABOX_REQUEST_SYSCLK,
    ABOX_REQUEST_L2C,
    ABOX_CHANGED_GEAR,
    ABOX_SET_GIC_DIST,
    ABOX_THREAD_COUNT,
    ABOX_CYCLES_PER_THREAD,
    ABOX_REPORT_LOG = 0x10,
    ABOX_FLUSH_LOG,

    ABOX_REPORT_DUMP = 0x20,
    ABOX_REQUEST_DUMP,
    ABOX_FLUSH_DUMP,

    ABOX_VLINK_CONNECTED = 0x30,
    ABOX_VLINK_DISCONNECTED,
    ABOX_SYS_REGMAP_READ,
    ABOX_SYS_REGMAP_WRITE,
    ABOX_DIRECT_READ,
    ABOX_DIRECT_WRITE,
    ABOX_SET_GICD_ITARGET,
    ABOX_RELOAD_ADSP,

    ABOX_REPORT_SRAM = 0x40,
    ABOX_START_CLAIM_SRAM,
    ABOX_END_CLAIM_SRAM,
    ABOX_START_RECLAIM_SRAM,
    ABOX_END_RECLAIM_SRAM,
    ABOX_REPORT_DRAM,

    ABOX_SET_MODE = 0x50,

    ABOX_SET_TYPE = 0x60,

    ABOX_START_VSS = 0xA0,

    /*ABOX PCM DEBUG */

    ABOX_REQUEST_PCM_DUMP_FW = 0xB0,
    ABOX_REQUEST_PCM_DUMP_FW_SRC,
    ABOX_FLUSH_PCM_DUMP_FW,
    ABOX_FLUSH_PCM_DUMP_FW_SRC,
    ABOX_SET_AVB,

    ABOX_REPORT_COMPONENT = 0xC0,
    ABOX_UPDATE_COMPONENT_CONTROL,
    ABOX_REQUEST_COMPONENT_CONTROL,
    ABOX_REPORT_COMPONENT_CONTROL,

    ABOX_REQUEST_DEBUG = 0xDE,

    ABOX_REPORT_FAULT = 0xFA,
    ABOX_SET_HIFI_LOG_LEVEL,

    /* Expand IPC was deleted = 0xFC */
    /* ABOX INTER DSP TEST */

    ABOX_START_INTERDSP_TEST = 0xFD,
    ABOX_BROADCAST_INTERDSP_TEST,
    ABOX_END_INTERDSP_TEST,
    ABOX_INTERDSP_TEST_SUCCESSFUL,

    ABOX_CLEAR_CHECK_TIME_BUF,
    ABOX_MEMTEST_TCM,
    ABOX_MEMTEST_ARAM,
    ABOX_MEMTEST_DDR,
    }ABOX_SYSTEM_MSG;

typedef struct ipcSystemMsg
    {
    ABOX_SYSTEM_MSG msgtype;
    int32_t   param1;
    int32_t   param2;
    int32_t   param3;
    int32_t   param4;
    uint32_t  param5;
    union
        {
        int32_t   param_s32[2];
        INT8    param_bundle[12];
        UINT64  param_u64;
        } bundle;
    } IPC_SYSTEM_MSG;

/* PCM task IPC */

typedef enum pcmMsg
    {
    PCM_OPEN                = 1,
    PCM_CLOSE               = 2,
    PCM_CPUDAI_TRIGGER      = 3,
    PCM_CPUDAI_HW_PARAMS    = 4,
    PCM_CPUDAI_SET_FMT      = 5,
    PCM_CPUDAI_SET_CLKDIV   = 6,
    PCM_CPUDAI_SET_SYSCLK   = 7,
    PCM_CPUDAI_STARTUP      = 8,
    PCM_CPUDAI_SHUTDOWN     = 9,
    PCM_CPUDAI_DELAY        = 10,
    PCM_PLTDAI_OPEN         = 11,
    PCM_PLTDAI_CLOSE        = 12,
    PCM_PLTDAI_IOCTL        = 13,
    PCM_PLTDAI_HW_PARAMS    = 14,
    PCM_PLTDAI_HW_FREE      = 15,
    PCM_PLTDAI_PREPARE      = 16,
    PCM_PLTDAI_TRIGGER      = 17,
    PCM_PLTDAI_POINTER      = 18,
    PCM_PLTDAI_MMAP         = 19,
    PCM_SET_BUFFER          = 20,
    PCM_SYNCHRONIZE         = 21,
    PCM_PLTDAI_ACK          = 22,
    PCM_PP_ATTACH           = 30,
    PCM_PP_DETACH           = 31,
    PCM_PP_OPEN             = 32,
    PCM_PP_PREPARE          = 33,
    PCM_PP_HW_PARAM         = 34,
    PCM_PP_TRIGGER          = 35,
    PCM_PP_POINTER          = 36,
    PCM_PP_HW_FREE          = 37,
    PCM_PP_CLOSE            = 38,
    PCM_PLTDAI_REGISTER     = 50,
    }PCMMSG;

typedef struct pcmtaskHwParams
    {
    int32_t sample_rate;
    int32_t bit_depth;
    int32_t channels;
    } PCMTASK_HW_PARAMS;

typedef struct pcmtaskSetBuffer
    {
    int32_t phyaddr;
    int32_t size;
    int32_t count;
    } PCMTASK_SET_BUFFER;

typedef struct pcmOemExtparam
    {
    int32_t vendorid;
    int32_t category;
    int32_t paramid;
    int32_t size;

    /* params points to the first element of the raw data. */
    /* every raw data SHOULD BE controlled carefully with */
    /* its size. */

    uint32_t * values;
    } PCM_OEM_EXTPARAM;

typedef struct pcmPostParam
    {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
    uint32_t param5;
    } PCM_POST_PARAM;

typedef struct pcmtaskHardware
    {
    INT8    name[32];           /* name */
    uint32_t  addr;               /* buffer address */
    uint32_t  width_min;          /* min width */
    uint32_t  width_max;          /* max width */
    uint32_t  rate_min;           /* min rate */
    uint32_t  rate_max;           /* max rate */
    uint32_t  channels_min;       /* min channels */
    uint32_t  channels_max;       /* max channels */
    uint32_t  buffer_bytes_max;   /* max buffer size */
    uint32_t  period_bytes_min;   /* min period size */
    uint32_t  period_bytes_max;   /* max period size */
    uint32_t  periods_min;        /* min # of periods */
    uint32_t  periods_max;        /* max # of periods */
    } PCMTASK_HARDWARE;

typedef struct pcmtaskDmaTrigger
    {
    int32_t   trigger;
    int32_t   rbuf_offset;
    int32_t   rbuf_cnt;
    BOOL    is_real_dma;
    } PCMTASK_DMA_TRIGGER;

typedef enum ppMsg
    {
    PP_SRC,
    PP_MUX,
    PP_EXT_SOL,
    PP_SFI,
    PP_MAX
    } PP_MSG;

typedef struct adspSrcMsg
    {
    BOOL    enable;
    BOOL    is_real_dma;
    int32_t   cmd;
    int32_t   type;
    int32_t   pcm_device_id;
    int32_t   src_samplerate;
    int32_t   dst_samplerate;
    int32_t   src_format;
    int32_t   dst_format;
    } ADSP_SRC_MSG;

typedef struct adspMuxMsg {
    BOOL    enable;
    BOOL    is_real_dma;
    int32_t   pcm_device_id;
    int32_t   cmd;
    int32_t   type;
    int32_t   sample_rate;
    int32_t   format;
    int32_t   channel;
    int32_t   dma_id;
    int32_t   uaif_id;
    int32_t   slot_start;
    int32_t   slot_end;
} ADSP_MUX_MSG;

typedef struct externalSolMsg {
    BOOL    enable;
    BOOL    is_real_dma;
    int32_t   cmd;
    int32_t   type;
    int32_t   pcm_device_id;
    int32_t   src_samplerate;
    int32_t   dst_samplerate;
    int32_t   src_format;
    int32_t   dst_format;
    int32_t   src_channel;
    int32_t   dst_channel;
} EXTERNAL_SOL_MSG;

typedef struct pcmtaskPpMsg
    {
    PP_MSG msgtype;
    union
        {
        ADSP_SRC_MSG        src;
        ADSP_MUX_MSG        mux;
        EXTERNAL_SOL_MSG    ext;
        } pp_solution;
    } PCMTASK_PP_MSG;

typedef struct ipcPcmtaskMsg
    {
    PCMMSG  msgtype;
    int32_t   pcm_device_id;
    int32_t   hw_dma_id;
    int32_t   irq_id;
    int32_t   pcm_alsa_id;
    int32_t   domain_id;
    ULONG   start_threshold;
    union
        {
        PCMTASK_HW_PARAMS   hw_params;
        PCMTASK_SET_BUFFER  setbuff;
        PCM_OEM_EXTPARAM    ext_param;
        PCM_POST_PARAM      post_param;
        PCMTASK_HARDWARE    hardware;
        PCMTASK_DMA_TRIGGER dma_trigger;
        PCMTASK_PP_MSG      pp_config;
        uint32_t              pointer;
        int32_t               trigger;
        int32_t               synchronize;
        } param;
    } IPC_PCMTASK_MSG;

/* offload IPC */

typedef enum offloadMsg
    {
    OFFLOAD_OPEN = 1,
    OFFLOAD_CLOSE,
    OFFLOAD_SETPARAM,
    OFFLOAD_START,
    OFFLOAD_WRITE,
    OFFLOAD_PAUSE,
    OFFLOAD_STOP,
    } OFFLOADMSG;

typedef struct offloadSetParam
    {
    int32_t sample_rate;
    int32_t chunk_size;
    } OFFLOAD_SET_PARAM;

typedef struct offloadStart
    {
    int32_t id;
    } OFFLOAD_START_PARAM;

typedef struct offloadWrite
    {
    int32_t id;
    int32_t buff;
    int32_t size;
    } OFFLOAD_WRITE_PARAM;

typedef struct ipcOffloadtaskMsg
    {
    OFFLOADMSG  msgtype;
    int32_t       channel_id;
    union
        {
        OFFLOAD_SET_PARAM   setparam;
        OFFLOAD_START_PARAM start;
        OFFLOAD_WRITE_PARAM write;
        } param;
    } IPC_OFFLOADTASK_MSG;

/* config IPC */

typedef enum aboxConfigmsg
    {
    ASRC_HW_INFO = 0x1,
    } ABOX_CONFIGMSG;

typedef struct asrcMsgHwInfo
    {
    unsigned int dma_id;
    unsigned int dma_type;
    int asrc_id;
    BOOL enable;
    unsigned int backend_format;
    unsigned int backend_rate;
    unsigned int backend_channels;
    } ASRC_MSG_HW_INFO;

typedef struct ipcAboxCfgMsg
    {
    ABOX_CONFIGMSG msgtype;
    union
        {
        ASRC_MSG_HW_INFO asrc_hw_info;
        } param;
    }IPC_ABOX_CONFIG_MSG;

/* IPC msg */

typedef enum ipcId
    {
    IPC_RECEIVED            = BIT(0),
    IPC_SYSTEM              = BIT(1),
    IPC_PCMPLAYBACK         = BIT(2),
    IPC_PCMCAPTURE          = BIT(3),
    IPC_OFFLOAD             = BIT(4),
    IPC_DMA_INTR            = BIT(5),
    NOT_USED6               = BIT(6),
    NOT_USED7               = BIT(7),
    NOT_USED8               = BIT(8),
    NOT_USED9               = BIT(9),
    IPC_ABOX_CONFIG         = BIT(10),
    IPC_ABOX_STOP_LOOPBACK  = BIT(11),
    IPC_POSTPROCESSING      = BIT(12),
    IPC_PERFORMANCE         = BIT(13),
    NOT_USED14              = BIT(14),
    NOT_USED15              = BIT(15),
    IPC_ID_COUNT            = BIT(16),
    }IPC_ID;

typedef struct aboxIpcMsg
    {
    IPC_ID                  ipcid;
    int32_t                 task_id;
    union IPC_MSG
        {
        IPC_SYSTEM_MSG system;
        IPC_PCMTASK_MSG pcmtask;
        IPC_OFFLOADTASK_MSG offload;
        IPC_ABOX_CONFIG_MSG config;
        } msg;
    } ABOX_IPC_MSG;

typedef STATUS (*ABOX_IPC_HANDLE_FUNC)(uint32_t, ABOX_IPC_MSG *, VXB_DEV_ID);

#ifdef __cplusplus
    }
#endif

#endif /* __INCaboxIpch */


