/* vxbFdtAboxCtr.h - Samsung Abox controller driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Samsung Abox mailbox driver specific definitions.
*/

#ifndef __INCvxbFdtAboxCtrh
#define __INCvxbFdtAboxCtrh

#include <vxWorks.h>
#include <subsys/clk/vxbClkLib.h>
#include <subsys/reg/vxbRegMap.h>
#include <soc.h>
#include <aboxIpc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DTS value defines */

#define ABOX_V920           920
#define ABOX_V920_EVT2_SFR  92020

/* DTS value defines end */

#define ABOX_CTR_GLB_DATA   aboxCtrGetData() //ABOX_DATA, for other modules

#define ABOX_CTR_NAME       "aboxCtr"

#define SYS_DOMAIN          DOMAIN2
#define IS_SYS_DOMAIN(id)   ((id) ==  SYS_DOMAIN ? TRUE : FALSE)

extern uint32_t aboxTarget;
extern uint32_t aboxSfrset;

#define IS_V920             (aboxTarget == ABOX_V920 ? TRUE : FALSE)
#define IS_V920_EVT2_SFR    (aboxSfrset == ABOX_V920_EVT2_SFR ? TRUE : FALSE)

#define ADSP_STATUS_TIMEOUT_MS      (30000)   //in sdk is 26000, we set it to 30000 in case timeout

#define ADSP_MASTER_CORE_ID         (0)

#define ADSP_CORE_STATUS            (0x4)

#define AUD_CPU_STATUS_POWER_ON     (0x1)

#define AUD_CPU_STATUS_MASK         (0x00000001)

#define IPC_RETRY                   (10)
#define ABOX_IPC_QUEUE_SIZE         (64)

#define ABOX_ATTACH_TASK_NAME       "aboxCtrlAttachTask"
#define ABOX_ATTACH_TASK_PRI        (110)
#define ABOX_ATTACH_TASK_STACK      (128 * 1024)

#define ABOX_IPC_TX_TASK_NAME       "aboxIpcQueTxTask"
#define ABOX_IPC_TX_TASK_PRI        (80)
#define ABOX_IPC_TX_TASK_STACK      (32 * 1024)

#define ADSP_LOG_BUFFER_OFFSET      0x2000000
#define ADSP_LOG_BUFFER_MAX_SIZE    0x100000


#define ABOX_MAJOR_VERSION_OFFSET   (20)
#define ABOX_MINOR_VERSION_OFFSET   (10)
#define ABOX_VERSION_VALID_MASK     (0x3FF)

#define GET_ADSP_MAJOR_VER(x)   \
    (((x) >> ABOX_MAJOR_VERSION_OFFSET) & ABOX_VERSION_VALID_MASK)
#define GET_ADSP_MINOR_VER(x)   \
    (((x) >> ABOX_MINOR_VERSION_OFFSET) & ABOX_VERSION_VALID_MASK)
#define GET_ADSP_PATCH_VER(x)   \
    ((x) & ABOX_VERSION_VALID_MASK)

#define ABOX_KERNEL_MAJOR_VER       (4)
#define ABOX_KERNEL_MINOR_VER       (0)
#define ABOX_KERNEL_PATCH_VER       (11)

#define AUD_PLL_RATE_HZ_FOR_48000_V920  (737280000)
#define AUD_PLL_RATE_HZ_FOR_48000       (AUD_PLL_RATE_HZ_FOR_48000_V920)

#define AUDIF_RATE_HZ               (AUD_PLL_RATE_HZ_FOR_48000 / 10)

#define ABOX_SAMPLING_RATES         (SNDRV_PCM_RATE_KNOT)
#define ABOX_SAMPLE_FORMATS         (VX_SND_FMTBIT_S16_LE       \
                                     | VX_SND_FMTBIT_S24_LE     \
                                     | VX_SND_FMTBIT_S32_LE)

#define ENUM_CTRL_STR_SIZE          (32)

#define DEFAULT_BUFFER_BYTES_MAX    (36864) // 36KB
#define PERIOD_BYTES_MIN            128//(SZ_128)//(1152) //1152B

#define TIMER_FLUSH                 (1 << 1)
#define TIMER_CLEAR                 (1 << 2)

/* registers */

#define ADSP_CORE0_V920_PMU_OFFSET  (0x3500)
#define ADSP_CORE1_V920_PMU_OFFSET  (0x3580)
#define ADSP_CORE2_V920_PMU_OFFSET  (0x3600)

#define ADSP_CORE0_PMU_OFFSET       (ADSP_CORE0_V920_PMU_OFFSET)
#define ADSP_CORE1_PMU_OFFSET       (ADSP_CORE1_V920_PMU_OFFSET)
#define ADSP_CORE2_PMU_OFFSET       (ADSP_CORE2_V920_PMU_OFFSET)

#define ABOX_SYSPOWER_CTRL          (0x0010)
#define ABOX_SYSPOWER_CTRL_MASK     (0x1)

#define ABOX_QCHANNEL_DISABLE       (0x0038)

#define ABOX_TIMER_CTRL0(n)         (0x600 + ((n) * 0x20))

//adsp_state
typedef enum adspState
    {
    ADSP_DISABLED,
    ADSP_DISABLING,
    ADSP_VERSION_MISMATCH,
    ADSP_ENABLING,
    ADSP_ENABLED,
    ADSP_STATE_COUNT,
    } ADSP_STATE;

//abox_domains
typedef enum aboxDomains
    {
    DOMAIN0 = 0,
    DOMAIN1,
    DOMAIN2,
    DOMAIN3,
    DOMAIN4,
    DOMAIN5,
    MAX_NUM_OF_DOMAIN,
    } ABOX_DOMAINS;

//abox_loopback_id
typedef enum aboxLoopbackId
    {
    ABOX_LOOPBACK0 = 0,
    ABOX_LOOPBACK1,
    NUM_OF_LOOPBACK
    } ABOX_LOOPBACK_ID;

//abox_clk
typedef enum aboxClks
    {
    CLK_DIV_AUD_CPU = 0,
    CLK_PLL_OUT_AUD,
    CLK_DIV_AUD_AUDIF,
    CLK_DIV_AUD_NOC,
    CLK_DIV_AUD_CNT,
    ABOX_CLK_CNT
} ABOX_CLKS;

//qchannel
typedef enum qchannel
    {
    ABOX_CCLK,
    ABOX_ACLK,
    ABOX_BCLK_UAIF0,
    ABOX_BCLK_UAIF1,
    ABOX_BCLK_UAIF2,
    ABOX_BCLK_UAIF3,
    ABOX_BCLK_UAIF4,
    ABOX_BCLK_UAIF5,
    ABOX_BCLK_UAIF6,
    ABOX_BCLK_UAIF7,
    ABOX_BCLK_UAIF8,
    ABOX_BCLK_UAIF9,
    Q_CHANNEL_MAX
    } Q_CHANNEL;

//reloading_adsp_status
typedef enum reloadingAdspStatus
    {
    RQ_RELOADING_ADSP,
    RQ_RELEASE_RESOURCE,
    ACK_RELEASE,
    RESTART_ADSP,
    ACK_RESTART,
    RELOADING_MAX
    } RELOADING_ADSP_STATUS;

//abox_ipc
typedef struct aboxIpc
    {
    uint32_t       ipcId; //hw_irq
    ABOX_IPC_MSG msg;
    } ABOX_IPC;

//abox_dram_request
typedef struct aboxDramRequest
    {
    void *  id;
    BOOL    on;
    } ABOX_DRAM_REQUEST;

//adsp_data
typedef struct adspData
    {
    VIRT_ADDR   virtAddr;       //virt_addr
    PHYS_ADDR   physAddr;       //phys_addr
    ULONG       size;           //size
    uint32_t      pmuregOfs;      //pmureg_offset
    } ADSP_DATA;

//performance_data
typedef struct performanceData
    {
    BOOL    checked;
    UINT64  period_t;
    } PERFORMANCE_DATA;

//abox_data
typedef struct aboxCtrData
    {
    VXB_DEV_ID                  pDev;   //pdev
    VX_SND_SOC_COMPONENT *      cmpnt;  //cmpnt

    spinlockIsr_t               regmapSpinlockIsr;
    VXB_REG_MAP *               pPmuRegMap;
    void *                      regHandle;
    VIRT_ADDR                   sfrBase;   //sfr_base
    VIRT_ADDR                   sysregSfrBase;    //sysreg_sfr_base
    VIRT_ADDR                   hifiSfrBase;  //hifi_sfr_base
    VIRT_ADDR                   tcm0Base;  //tcm0_base
    VIRT_ADDR                   timerBase; //timer_base

    ADSP_DATA *                 adspData;  //adsp_data
    uint32_t                    adspVersion;    //adsp_version
    ADSP_STATE                  adspState; //adsp_state

    uint32_t                    numOfUaif;  //num_of_uaif
    uint32_t                    numOfPcmPlayback;   //num_of_pcm_playback
    uint32_t                    numOfPcmCapture;    //num_of_pcm_capture
    uint32_t                    numOfRdma;  //num_of_rdma
    uint32_t                    numOfWdma;  //num_of_wdma
    uint32_t                    numOfAdsp;  //num_of_adsp
    VXB_DEV_ID                  mboxRxDev;  //dev_mailbox_rx
    VXB_DEV_ID                  mboxTxDev;  //dev_mailbox_tx
    VXB_DEV_ID                  gicDev;     //dev_gic
    VXB_DEV_ID *                uaifDev;    //pdev_uaif
    VXB_DEV_ID *                pcmPlaybackDev; //pdev_pcm_playback
    VXB_DEV_ID *                pcmCaptureDev;  //pdev_pcm_capture
    VXB_DEV_ID *                rdmaDev;    //pdev_rdma
    VXB_DEV_ID *                wdmaDev;    //pdev_wdma

    TASK_ID                     ipcTxTask;
    SEM_ID                      ipcTxSem;
    ABOX_IPC                    ipcQue[ABOX_IPC_QUEUE_SIZE]; //ipc_queue
    uint32_t                    ipcQueStart;    //ipc_queue_start
    uint32_t                    ipcQueEnd;      //ipc_queue_end
    spinlockIsr_t               ipcQueSpinlockIsr; //ipc_queue_lock
    spinlockIsr_t               ipcProcSpinlockIsr;   //ipc_raw_lock
    SEM_ID                      ipcDspStaWaitSem;

    VXB_CLK_ID *                aboxClks;   //abox_clk

    ABOX_DRAM_REQUEST           dramRequests[16];  //dram_requests
    BOOL                        enabled;
    uint32_t                    myDomainId;    //vabox_my_id
    PERFORMANCE_DATA            perfData;  //perf_data
    uint32_t                    avbStatus;  //avb_status

    UINT32                      adspReloadingStatus;  //adsp_reloading_status
    }ABOX_CTR_DATA;

extern ABOX_CTR_DATA * aboxCtrGetData (void);

extern int32_t abox_request_ipc (VXB_DEV_ID ctrDev, uint32_t ipcId,
                                 const ABOX_IPC_MSG * pMsg, size_t size,
                                 uint32_t atomic, uint32_t sync);
extern void abox_request_dram_on (VXB_DEV_ID ctrDev, void * id, BOOL on);
extern int32_t abox_enable (VXB_DEV_ID ctrDev);
extern int32_t abox_disable (VXB_DEV_ID ctrDev);
extern int32_t abox_register_pcm_capture (VXB_DEV_ID ctrDev, VXB_DEV_ID pcmCaptureDev,
                                          uint32_t id);
extern int32_t abox_register_pcm_playback (VXB_DEV_ID ctrDev,VXB_DEV_ID pcmPlaybackDev,
                                           uint32_t id);
extern int32_t abox_register_rdma (VXB_DEV_ID ctrDev, VXB_DEV_ID rdmaDev,
                                   uint32_t id);
extern int32_t abox_register_wdma (VXB_DEV_ID ctrDev, VXB_DEV_ID wdmaDev,
                                   uint32_t id);
extern STATUS abox_register_uaif (VXB_DEV_ID ctrDev, VXB_DEV_ID uaifDev,
                                  uint32_t id);
extern STATUS abox_hw_params_fixup_helper (VX_SND_SOC_PCM_RUNTIME * rtd,
                                           VX_SND_PCM_HW_PARAMS * params);
extern STATUS abox_disable_qchannel (ABOX_CTR_DATA * pCtrData,
                                     Q_CHANNEL clk, int32_t disable);
extern STATUS abox_check_bclk_from_cmu_clock (ABOX_CTR_DATA * pCtrData,
                                              uint32_t rate, uint32_t channels,
                                              uint32_t width);
extern STATUS aboxClkRateSet (VXB_CLK_ID pClk, uint64_t rate);
#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtAboxCtrh */

