/* vxbFdtAboxCtr.c - Samsung Abox controller driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#include <vxWorks.h>
#include <pmapLib.h>
#include <subsys/int/vxbIntLib.h>
#include <subsys/pinmux/vxbPinMuxLib.h>
#include <vxbFdtAboxGic.h>
#include <vxbFdtAboxMailbox.h>
#include <vxbFdtAboxUaif.h>
#include <vxbFdtAboxDma.h>
#include <vxbAboxPcm.h>
#include <vxbFdtAboxCtr.h>

#undef ABOX_CTR_DEBUG
#ifdef ABOX_CTR_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t aboxCtrDbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((aboxCtrDbgMask & (mask)) || ((mask) == DBG_ALL))           \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("[%s(%d)] ", __func__, __LINE__);     \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* ABOX_CTR_DEBUG */

/* defines */

#define ABOX_SOC_SINGLE_EXT_MULTI(xname, xinfo, xreg, xshift, xmax, xinvert, \
     xhandler_get, xhandler_put) \
{   .id.iface = SNDRV_CTL_TYPE_MIXER, .id.name = xname, \
    .info = xinfo, \
    .get = xhandler_get, .put = xhandler_put, \
    .privateValue = SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert, 0) }

#define MSEC_PER_SEC                (1000)
#define ADSP_ENABLE_TIMEOUT_MS      (1000)

#define MAX_BOOT_RETRY_COUNT        (3)

#define AUD_OUT_V920_EVT2           (0x28a0)
#define AUD_OUT                     (AUD_OUT_V920_EVT2)
#define AUD_GPIO_PAD_RTO_MASK       (11)

#define TOP_OUT_V920                (0x3C20)
#define TOP_OUT                     (TOP_OUT_V920)
#define RTO_V920                    (0x6000000)
#define RETENTION_DISABLE           (RTO_V920)

#define ADSP_CORE0_SW_RESET         (0x14)
#define MISC_BIT                    (16)

//abox_array_ref
typedef enum aboxArrayRef
    {
    ABOX_ARRAY_REF0,
    ABOX_ARRAY_REF1,
    ABOX_ARRAY_REF2,
    ABOX_ARRAY_REF3,
    ABOX_ARRAY_REF_CNT
    } ABOX_ARRAY_REF;

/* controllor read and write interface */

#define REG_READ_4(pData, regBase, regOfs)             \
                vxbRead32((pData)->regHandle, (uint32_t *)((regBase) + (regOfs)))

#define REG_WRITE_4(pData, regBase, regOfs, data)      \
                vxbWrite32((pData)->regHandle,              \
                           (uint32_t *)((regBase) + (regOfs)), (data))

/* forward declarations */

IMPORT void vxbUsDelay (int delayTime);
IMPORT TASK_ID aboxMachineAttachTaskId;

LOCAL STATUS aboxCtrProbe (VXB_DEV_ID pDev);
LOCAL STATUS aboxCtrAttach (VXB_DEV_ID pDev);
LOCAL int32_t aboxCmpntProbe (VX_SND_SOC_COMPONENT * component);
LOCAL void aboxCmpntRemove (VX_SND_SOC_COMPONENT * component);
LOCAL uint32_t aboxCmpntRead (VX_SND_SOC_COMPONENT * cmpnt,uint32_t regOfs);
LOCAL int32_t aboxCmpntWrite (VX_SND_SOC_COMPONENT * cmpnt, uint32_t regOfs,
                              uint32_t val);
LOCAL STATUS aboxLoopbackDaiSetFmt (VX_SND_SOC_DAI * dai, uint32_t fmt);
LOCAL STATUS aboxLoopbackDaiHwParams (SND_PCM_SUBSTREAM * substream,
                                      VX_SND_PCM_HW_PARAMS * params,
                                      VX_SND_SOC_DAI * dai);
LOCAL STATUS aboxLoopbackDaiHwFree (SND_PCM_SUBSTREAM * substream,
                                    VX_SND_SOC_DAI * dai);
LOCAL STATUS primaryCaptureDevGet (VX_SND_CONTROL * kcontrol,
                                   VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS primaryCaptureDevPut (VX_SND_CONTROL * kcontrol,
                                   VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS primaryPlaybackDevGet (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS primaryPlaybackDevPut (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS aboxArrayRefInfo (VX_SND_CONTROL * kcontrol,
                               VX_SND_CTRL_INFO * uinfo);
LOCAL STATUS aboxArrayRefGet (VX_SND_CONTROL * kcontrol,
                              VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS aboxArrayRefPut (VX_SND_CONTROL * kcontrol,
                              VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS aboxGetAvb (VX_SND_CONTROL * kcontrol,
                         VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS aboxSetAvb (VX_SND_CONTROL * kcontrol,
                         VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL VXB_RESOURCE * aboxCtrResAlloc (VXB_DEV_ID pDev, VXB_DEV_ID pChild,
                                      UINT32 id);
LOCAL STATUS aboxCtrResFree (VXB_DEV_ID pDev, VXB_DEV_ID pChild,
                             VXB_RESOURCE * pRes);
LOCAL VXB_RESOURCE_LIST * aboxCtrResListGet (VXB_DEV_ID pDev,
                                             VXB_DEV_ID pChild);
LOCAL VXB_FDT_DEV * aboxCtrDevGet (VXB_DEV_ID pDev, VXB_DEV_ID pChild);

/* locals */

LOCAL VXB_DRV_METHOD aboxCtrMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxCtrProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxCtrAttach},
    {VXB_DEVMETHOD_CALL (vxbResourceAlloc), (FUNCPTR) aboxCtrResAlloc},
    {VXB_DEVMETHOD_CALL (vxbResourceFree),  (FUNCPTR) aboxCtrResFree},
    {VXB_DEVMETHOD_CALL (vxbResourceListGet),(FUNCPTR) aboxCtrResListGet},
    {VXB_DEVMETHOD_CALL (vxbFdtDevGet),      (FUNCPTR) aboxCtrDevGet},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxCtrMatch[] =
    {
        {
        "samsung,abox",
        NULL
        },
        {}                              /* empty terminated list */
    };

//g_abox_data
LOCAL ABOX_CTR_DATA * pAboxCtrData;

/*
 * modules init sequence:
 * 0. Abox Ctr
 * 1. GIC
 * 2. Mailbox
 * 3. debug (if need)
 * 4. rdma
 * 5. sft rdma (if need)
 * 6. pcm playback
 * 7. uaif
 * 8. wdma
 * 9. pcm capture
 * 10. sft (if need)
 */

LOCAL char * gicMatchName[] =
    {
    "samsung,abox_gic"
    };
LOCAL char * mboxMatchName[] =
    {
    "samsung,abox_mailbox",
    "samsung,abox_mailbox_rx",
    "samsung,abox_mailbox_tx"
    };
LOCAL char * rdmaMatchName[] =
    {
    "samsung,abox-rdma"
    };
LOCAL char * pcmPlaybackMatchName[] =
    {
    "samsung,abox-pcm-playback"
    };
LOCAL char * uaifMatchName[] =
    {
    "samsung,abox-uaif"
    };
LOCAL char * wdmaMatchName[] =
    {
    "samsung,abox-wdma"
    };
LOCAL char * pcmCaptureMatchName[] =
    {
    "samsung,abox-pcm-capture"
    };

LOCAL spinlockIsr_t fwDlSpinlockIsr;

//playback_dev_num_enum
LOCAL const char * playbackDevNumEnum[] =
    {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "11", "12", "13", "14", "15", "NO_CONNECT",
    };
SOC_ENUM_SINGLE_EXT_DECL (primary_playback_enum, playbackDevNumEnum);

//capture_dev_num_enum
LOCAL const char *captureDevNumEnum[] =
    {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22",
    "23", "24", "25", "26", "27", "28", "29", "30", "NO_CONNECT",
    };
SOC_ENUM_SINGLE_EXT_DECL (primary_capture_enum, captureDevNumEnum);

//abox_array_ref_value
LOCAL uint32_t aboxArrayRefValue[ABOX_ARRAY_REF_CNT];

//abox_avb_enum_texts
LOCAL const char * aboxAvbEnumTexts[] =
    {"Off", "On", "Tx", "Rx", "Test", "Tx_TP", "Rx_TP"};

//abox_avb_enum
LOCAL const VX_SND_ENUM aboxAvbEnum[] =
    {
    SOC_ENUM_SINGLE_EXT (NELEMENTS (aboxAvbEnumTexts), aboxAvbEnumTexts),
    };

//abox_cmpnt_controls
LOCAL const VX_SND_CONTROL aboxCmpntControls[] =
    {
    SOC_ENUM_EXT ("PRIMARY CAPTURE DEVICE", primary_capture_enum,
                  primaryCaptureDevGet, primaryCaptureDevPut),
    SOC_ENUM_EXT ("PRIMARY PLAYBACK DEVICE", primary_playback_enum,
                  primaryPlaybackDevGet, primaryPlaybackDevPut),
    ABOX_SOC_SINGLE_EXT_MULTI ("ARRAY REF", aboxArrayRefInfo,
                               0, 0, 384000, 0, aboxArrayRefGet,
                               aboxArrayRefPut),
    SOC_ENUM_EXT ("AVB Enable", aboxAvbEnum, aboxGetAvb, aboxSetAvb),
    };

//abox_cmpnt
LOCAL const VX_SND_SOC_CMPT_DRV aboxCmpnt =
    {
    .probe              = aboxCmpntProbe,
    .remove             = aboxCmpntRemove,
    .controls           = aboxCmpntControls,
    .num_controls       = NELEMENTS (aboxCmpntControls),
    .probe_order        = SND_SOC_COMP_ORDER_LATE,
    .read               = aboxCmpntRead,
    .write              = aboxCmpntWrite,
//    .regmap             = NULL,
    };

//abox_loopback_dai_ops
LOCAL const VX_SND_SOC_DAI_OPS aboxLoopbackDaiOps =
    {
    .setFmt    = aboxLoopbackDaiSetFmt,
    .hwParams  = aboxLoopbackDaiHwParams,
    .hwFree    = aboxLoopbackDaiHwFree,
    };

//abox_dais
LOCAL VX_SND_SOC_DAI_DRIVER aboxDais[] =
    {
        { /* Virtual DAI */
        .name = "LOOPBACK0",
        .id = ABOX_LOOPBACK0,
        .playback =
            {
            .streamName = "LOOPBACK0 Playback",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        .capture =
            {
            .streamName = "LOOPBACK0 Capture",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        .ops = &aboxLoopbackDaiOps,
        },
        { /* Virtual DAI */
        .name = "LOOPBACK1",
        .id = ABOX_LOOPBACK1,
        .playback =
            {
            .streamName = "LOOPBACK1 Playback",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        .capture =
            {
            .streamName = "LOOPBACK1 Capture",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        .ops = &aboxLoopbackDaiOps,
        },
        { /* Virtual DAI */
        .name = "PCM Dummy Backend",
        .playback =
            {
            .streamName = "PCM Playback Dummy Backend",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        .capture =
            {
            .streamName = "PCM Capture Dummy Backend",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        },
    };

/* globals */

VXB_DRV aboxCtrDrv =
    {
    {NULL},
    ABOX_CTR_NAME,                          /* Name */
    "Abox Controller FDT driver",           /* Description */
    VXB_BUSID_FDT,                          /* Class */
    0,                                      /* Flags */
    0,                                      /* Reference count */
    aboxCtrMethodList                       /* Method table */
    };

VXB_DRV_DEF(aboxCtrDrv)

uint32_t aboxTarget;
uint32_t aboxSfrset;

LOCAL void aboxCtrRegmapRead
    (
    ABOX_CTR_DATA * pCtrData,
    VIRT_ADDR       regBase,
    uint64_t        regOfs,
    uint32_t *      pRegVal
    )
    {
    SPIN_LOCK_ISR_TAKE (&pCtrData->regmapSpinlockIsr);

    *pRegVal = REG_READ_4 (pCtrData, regBase, regOfs);

    SPIN_LOCK_ISR_GIVE (&pCtrData->regmapSpinlockIsr);
    }

LOCAL void aboxCtrRegmapWrite
    (
    ABOX_CTR_DATA * pCtrData,
    VIRT_ADDR       regBase,
    uint64_t        regOfs,
    uint32_t        regVal
    )
    {
    SPIN_LOCK_ISR_TAKE (&pCtrData->regmapSpinlockIsr);

    REG_WRITE_4 (pCtrData, regBase, regOfs, regVal);

    SPIN_LOCK_ISR_GIVE (&pCtrData->regmapSpinlockIsr);
    }

//__abox_ipc_queue_full
LOCAL BOOL _aboxIpcQueIsFull
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    if (((pCtrData->ipcQueEnd + 1) % ABOX_IPC_QUEUE_SIZE)
        == pCtrData->ipcQueStart)
        {
        return TRUE;
        }

    return FALSE;
    }

//__abox_ipc_queue_empty
LOCAL BOOL _aboxIpcQueIsEmpty
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    if (pCtrData->ipcQueEnd == pCtrData->ipcQueStart)
        {
        return TRUE;
        }

    return FALSE;
    }

LOCAL BOOL aboxIpcQueCheckEmpty
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    BOOL            ret = FALSE;

    SPIN_LOCK_ISR_TAKE (&pCtrData->ipcQueSpinlockIsr);

    ret = _aboxIpcQueIsEmpty (pCtrData);

    SPIN_LOCK_ISR_GIVE (&pCtrData->ipcQueSpinlockIsr);

    return ret;
    }

//abox_ipc_queue_put
LOCAL STATUS aboxIpcQuePut
    (
    ABOX_CTR_DATA * pCtrData,
    ABOX_IPC *      pSrcIpc
    )
    {
    STATUS          ret = OK;

    SPIN_LOCK_ISR_TAKE (&pCtrData->ipcQueSpinlockIsr);

    if (!_aboxIpcQueIsFull (pCtrData))
        {
        memcpy (&pCtrData->ipcQue[pCtrData->ipcQueEnd],
                pSrcIpc, sizeof(ABOX_IPC));

        pCtrData->ipcQueEnd = (pCtrData->ipcQueEnd + 1) % ABOX_IPC_QUEUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pCtrData->ipcQueSpinlockIsr);

    return ret;
    }

//abox_ipc_queue_get
LOCAL STATUS aboxIpcQueGet
    (
    ABOX_CTR_DATA * pCtrData,
    ABOX_IPC *      pIpc
    )
    {
    STATUS          ret = OK;

    SPIN_LOCK_ISR_TAKE (&pCtrData->ipcQueSpinlockIsr);

    if (!_aboxIpcQueIsEmpty (pCtrData))
        {
        memcpy (pIpc, &pCtrData->ipcQue[pCtrData->ipcQueStart],
                sizeof (ABOX_IPC));

        pCtrData->ipcQueStart =
            (pCtrData->ipcQueStart + 1) % ABOX_IPC_QUEUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pCtrData->ipcQueSpinlockIsr);

    return ret;
    }

//abox_schedule_ipc
LOCAL STATUS aboxScheduleIpc
    (
    ABOX_CTR_DATA *         pCtrData,
    uint32_t                ipcId,
    const ABOX_IPC_MSG *    pMsg,
    size_t                  size,
    BOOL                    isAtomic,
    BOOL                    isSync
    )
    {
    ABOX_IPC                ipc;
    uint32_t                retry;

    ipc.ipcId = ipcId;
    memcpy (&ipc.msg, pMsg, sizeof(ABOX_IPC_MSG));

    retry = 0;
    do {
        if (aboxIpcQuePut (pCtrData, &ipc) == OK)
            {

            /* start tx queue after inserting new msg */

            (void) semGive (pCtrData->ipcTxSem);

            DBG_MSG (DBG_INFO, "start abox ipc tx task\n");

            if (!isAtomic && isSync)
                {

                /* wait until all msgs of tx queue are processed */

                while (!aboxIpcQueCheckEmpty (pCtrData))
                    {
                    (void) taskDelay (1);
                    }
                }

            return OK;
            }
        else /* ipc tx queue is full */
            {
            (void) semGive (pCtrData->ipcTxSem);

            DBG_MSG (DBG_INFO, "start abox ipc tx task\n");

            if (isAtomic)
                {
                (void) taskDelay (1);
                }
            else
                {

                /* wait until all msgs of tx queue are processed */

                while (!aboxIpcQueCheckEmpty (pCtrData))
                    {
                    (void) taskDelay (1);
                    }
                }
            }
        } while (retry++ < IPC_RETRY);

    DBG_MSG (DBG_ERR, "abox tx ipc queue is full\n");

    return ERROR;
    }

//abox_request_ipc
int32_t abox_request_ipc
    (
    VXB_DEV_ID              ctrDev,
    uint32_t                ipcId,
    const ABOX_IPC_MSG *    pMsg,
    size_t                  size,
    uint32_t                atomic,
    uint32_t                sync
    )
    {
    ABOX_CTR_DATA *         pCtrData;
    BOOL                    isAtomic;
    BOOL                    isSync;
    int32_t                 ret = 0;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return -ENODATA;
        }

    isAtomic = (atomic) ? TRUE : FALSE;
    isSync = (sync) ? TRUE : FALSE;

    if (isAtomic && isSync)
        {
        SPIN_LOCK_ISR_TAKE (&pCtrData->ipcProcSpinlockIsr);

        DBG_MSG (DBG_INFO, "sync ipcId=%d to DSP\n", ipcId);

        if (aboxMailboxSendMsg (pCtrData->mboxTxDev,
                                ipcId,
                                pMsg,
                                sizeof (ABOX_IPC_MSG),
                                10000,
                                0) != OK)
            {
            DBG_MSG (DBG_ERR, "\n");
            ret = -EBUSY;
            }

        SPIN_LOCK_ISR_GIVE (&pCtrData->ipcProcSpinlockIsr);
        }
    else
        {
        DBG_MSG (DBG_INFO, "async ipcId=%d to DSP\n", ipcId);

        if (aboxScheduleIpc (pCtrData,
                             ipcId,
                             pMsg,
                             size,
                             isAtomic,
                             isSync) != OK)
            {
            DBG_MSG (DBG_ERR, "\n");
            ret = -EBUSY;
            }
        }

    return ret;
    }

//abox_process_ipc
LOCAL void aboxIpcQueTxTask
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    ABOX_IPC        ipc;

    while (1)
        {
        (void) semTake (pCtrData->ipcTxSem, WAIT_FOREVER);

        DBG_MSG (DBG_INFO, "abox ipc tx task run\n");

        while (aboxIpcQueGet (pCtrData, &ipc) == OK)
            {
            DBG_MSG (DBG_INFO, "get ipcId=%d to tx\n",  ipc.ipcId);
            if (aboxMailboxSendMsg (pCtrData->mboxTxDev,
                                    ipc.ipcId,
                                    &ipc.msg,
                                    sizeof (ABOX_IPC_MSG),
                                    10000,
                                    0) != OK)
                {
                DBG_MSG (DBG_ERR, "ipc msg mailbox send failed\n");
                }

            vxbUsDelay (100);
            }
        }
    }

//abox_boot_done
LOCAL void aboxBootDone
    (
    ABOX_CTR_DATA * pCtrData,
    ABOX_IPC_MSG *  pMsg
    )
    {
    uint32_t        ver;
    uint32_t        dd;
    uint32_t        mm;
    uint32_t        yy;
    uint32_t        fwMajor;
    uint32_t        fwMinor;
    uint32_t        fwPatch;

    yy = pMsg->msg.system.param1;
    mm = pMsg->msg.system.param2;
    dd = pMsg->msg.system.param3;
    ver = pMsg->msg.system.param4;
    pCtrData->adspVersion = ver;
    fwMajor = GET_ADSP_MAJOR_VER (ver);
    fwMinor = GET_ADSP_MINOR_VER (ver);
    fwPatch = GET_ADSP_PATCH_VER (ver);

    DBG_MSG (DBG_INFO, "Firmware Version Date: %x/%x/%x\n",
             yy, mm, dd);
    DBG_MSG (DBG_INFO, "Firmware Version: %d.%d.%d\n",
             fwMajor, fwMinor, fwPatch);

    if (fwMajor != ABOX_KERNEL_MAJOR_VER)
        {
        DBG_MSG (DBG_ERR,
                 "ABOX Kernel & Firmware Major Version is Not Matching\n");

        pCtrData->adspState = ADSP_VERSION_MISMATCH;
        }
    else
        {
        if (fwMinor != ABOX_KERNEL_MINOR_VER)
            {
            DBG_MSG (DBG_WARN,
                     "ABOX Kernel & Firmware Minor version is Not Matching\n");
            }

        if (fwMinor != ABOX_KERNEL_PATCH_VER)
            {
            DBG_MSG (DBG_INFO,
                     "ABOX Kernel & Firmware Patch version is Not Matching\n");
            }

        pCtrData->adspState = ADSP_ENABLED;
        }

    (void) semGive (pCtrData->ipcDspStaWaitSem);
    }

//print_abox_fw_logs
LOCAL void printAboxFwLogs
    (
    ABOX_CTR_DATA * pCtrData,
    int32_t         size,
    int32_t         buffer,
    int32_t         coreId
    )
    {
    int32_t         logSize;
    int32_t         tmpSize;
    int32_t         numOfCore;
    char *          logBuf;
    char            line[256];

    numOfCore = pCtrData->numOfAdsp;

    if (buffer < 0 || buffer >= ADSP_LOG_BUFFER_MAX_SIZE)
        {
        return;
        }

    if (coreId < 0 || coreId >= numOfCore)
        {
        return;
        }

    logBuf = (char *)(pCtrData->adspData[coreId].virtAddr +
                      ADSP_LOG_BUFFER_OFFSET + (uint32_t) buffer);
    logSize = size;

    printf ("IPC Received for ABOX Logs... size=%d, buffer=%d\n",
            logSize, buffer);

    while (logSize > 0)
        {
        tmpSize = 0;

        if (logSize > 255)
            {
            tmpSize = 255;
            }
        else
            {
            tmpSize = logSize;
            }

        line[tmpSize] = '\0';

        memcpy (line, logBuf, tmpSize);
        printf ("%s\n", line);
        logSize -= 255;
        }
    }

//abox_release_uaif_resource
LOCAL STATUS aboxReleaseUaifRsrc
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    VXB_DEV_ID                  uaifDev
    )
    {
    enum uaifBclkMuxMode
        {
        SLAVE,
        MASTER
        };
    ULONG                       uaifMux;
    uint32_t                    uaifCtrl0;
    uint32_t                    uaifMode;

    uaifCtrl0 = vxSndSocComponentRead (pUaifData->cmpnt, ABOX_UAIF_CTRL0);
    uaifMode = uaifCtrl0 & ABOX_UAIF_MODE_MASK;
    uaifMux = vxbClkRateGet (pUaifData->bclkMux);

    if (uaifMux == MUX_UAIF_SLAVE || uaifMode == SLAVE)
        {
        aboxClkRateSet (pUaifData->bclkMux, vxbClkRateGet (pUaifData->bclk));
        DBG_MSG (DBG_ERR, "UAIF%d bclk mux:%s\n",
                 pUaifData->id,
                 uaifMux == MUX_UAIF_SLAVE ? "Slave" : "Master");
        }

    if (vxbPinMuxEnableById (pUaifData->pDev, pUaifData->idlePinctrlId) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to set idlePinctrlId=%d\n",
                 pUaifData->idlePinctrlId);
        }

    return OK;
    }

//abox_release_dma_resource
LOCAL uint32_t aboxReleaseDmaRsrc
    (
    struct abox_dma_data *  pDmaData,
    uint32_t                id,
    uint32_t                ipcId
    )
    {
    uint32_t                status;
    uint32_t                progress;

    status = 0;

    if (ipcId == IPC_PCMPLAYBACK)
        {
        if (pDmaData->type != DMA_TO_SFT_UAIF)
            {
            progress = aboxRdmaStatusGet (pDmaData);

            status = aboxRdmaIsEnabled (pDmaData);
            status |= progress;
            if (status)
                {
                aboxRdmaDisable (pDmaData);
                if (pDmaData->cmpnt)
                    {
                    __abox_rdma_flush_sifs (pDmaData);
                    }
                }
            }
        else
            {
            //SFT is not supported now
            }
        }
    else
        {
        progress = aboxWdmaGetStatus (pDmaData);

        status = wdmaIsEnable (pDmaData);
        status |= progress;
        if (status)
            {
            aboxWdmaDisable (pDmaData);
            }
        }

    DBG_MSG (DBG_INFO, "%s%d Running status:%d\n",
             (ipcId == IPC_PCMPLAYBACK) ? "RDMA":"WDMA", id, status);

    return status;
    }

//abox_release_active_resource
LOCAL void aboxReleaseActiveResource
    (
    ABOX_CTR_DATA *             pCtrData
    )
    {
    ABOX_PCM_CTRL_DATA *        pPcmData;
    ABOX_DMA_CTRL *             pDmaCtrl;
    struct abox_dma_data *      pDmaData;
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    uint32_t                    index;
    uint32_t                    rdmaStatus;
    uint32_t                    wdmaStatus;

    DBG_MSG (DBG_INFO, "start\n");

    rdmaStatus = 0;
    wdmaStatus = 0;

    for (index = 0; index < pCtrData->numOfUaif; index++)
        {
        DBG_MSG (DBG_INFO, "Checking UAIF[%d]\n", index);

        if (pCtrData->uaifDev[index] == NULL)
            {
            continue;
            }

        pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *)
                    vxbDevSoftcGet (pCtrData->uaifDev[index]);
        if (pUaifData->cmpnt == NULL)
            {
            continue;
            }

        (void) aboxReleaseUaifRsrc (pUaifData, pCtrData->uaifDev[index]);
        (void) abox_disable_qchannel (pCtrData,
                               ABOX_BCLK_UAIF0 + pUaifData->id, FALSE);
        }

    for (index = 0; index < pCtrData->numOfPcmPlayback; index++)
        {
        DBG_MSG (DBG_INFO, "Checking PCM%d Playback\n", index);

        if (pCtrData->pcmPlaybackDev[index] == NULL)
            {
            continue;
            }

        pPcmData = (ABOX_PCM_CTRL_DATA *)
                    vxbDevSoftcGet (pCtrData->pcmPlaybackDev[index]);
        if (pPcmData->cmpnt == NULL)
            {
            continue;
            }

        if (pPcmData->substream && pPcmData->substream->runtime)
            {
            pPcmData->substream->runtime->status.state =
                SNDRV_PCM_SUBSTREAM_DISCONNECTED;
            }
        }

    for (index = 0; index < pCtrData->numOfRdma; index++)
        {
        DBG_MSG (DBG_INFO, "Checking RDMA[%d]\n", index);

        if (pCtrData->rdmaDev[index] == NULL)
            {
            continue;
            }

        pDmaCtrl = (ABOX_DMA_CTRL *)
                    vxbDevSoftcGet (pCtrData->rdmaDev[index]);
        pDmaData = pDmaCtrl->pDmaData;
        if (pDmaData->cmpnt == NULL)
            {
            continue;
            }

        rdmaStatus = rdmaStatus |
                     aboxReleaseDmaRsrc (pDmaData, index, IPC_PCMPLAYBACK);
        }

    for (index = 0; index < pCtrData->numOfPcmCapture; index++)
        {
        DBG_MSG (DBG_INFO, "Checking PCM%d Capture\n", index);

        if (pCtrData->pcmCaptureDev[index] == NULL)
            {
            continue;
            }

        pPcmData = (ABOX_PCM_CTRL_DATA *)
                    vxbDevSoftcGet (pCtrData->pcmCaptureDev[index]);
        if (pPcmData->cmpnt == NULL)
            {
            continue;
            }

        if (pPcmData->substream && pPcmData->substream->runtime)
            pPcmData->substream->runtime->status.state =
                SNDRV_PCM_SUBSTREAM_DISCONNECTED;
        }

    for (index = 0; index < pCtrData->numOfWdma; index++)
        {
        DBG_MSG (DBG_INFO, "Checking WDMA[%d]\n", index);

        if (!pCtrData->wdmaDev[index])
            {
            continue;
            }

        pDmaCtrl = (ABOX_DMA_CTRL *)
                    vxbDevSoftcGet (pCtrData->wdmaDev[index]);
        pDmaData = pDmaCtrl->pDmaData;
        if (pDmaData->cmpnt == NULL)
            {
            continue;
            }

        wdmaStatus = wdmaStatus |
                     aboxReleaseDmaRsrc (pDmaData, index, IPC_PCMCAPTURE);
        }

    if (rdmaStatus || wdmaStatus)
        {
        abox_disable (pCtrData->pDev);
        }

    /* SYSPOWER CTRL must to disable for v920 */

    aboxCtrRegmapWrite (pCtrData, pCtrData->sfrBase,
                        ABOX_SYSPOWER_CTRL, 0x0);

    DBG_MSG (DBG_INFO, "Done\n");
    }

//abox_timer_active_resource
LOCAL void aboxTimerActiveRsrc
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    uint32_t        index;
    const uint32_t  total_timer = 16;

    // This fn will be called in sys domain only

    if (IS_SYS_DOMAIN (pCtrData->myDomainId))
        {
        for (index = 0; index < total_timer; index++)
            {
            REG_WRITE_4 (pCtrData, pCtrData->timerBase,
                         ABOX_TIMER_CTRL0 (index), (TIMER_CLEAR | TIMER_FLUSH));
            }
        }
    }

//abox_restart_adsp
LOCAL void aboxRestartAdsp
    (
    ABOX_CTR_DATA * pCtrData,
    int32_t         status
    )
    {
    pCtrData->adspReloadingStatus = status;
    switch (status)
        {
        case RQ_RELEASE_RESOURCE:
            pCtrData->adspState = ADSP_DISABLED;
            aboxReleaseActiveResource (pCtrData);
            aboxTimerActiveRsrc (pCtrData);
            break;

        default:
            break;
        }
    }

//abox_system_ipc_handler
void aboxSystemIpcHandle
    (
    ABOX_CTR_DATA *     pCtrData,
    ABOX_IPC_MSG *      pMsg
    )
    {
    IPC_SYSTEM_MSG *    pSystemMsg;

    pSystemMsg = &pMsg->msg.system;

    DBG_MSG (DBG_INFO, "type=%d\n", pSystemMsg->msgtype);

    switch (pSystemMsg->msgtype)
        {
        case ABOX_BOOT_DONE:
            aboxBootDone (pCtrData, pMsg);
            break;

        case ABOX_REPORT_DUMP:

            /*
             * param1: log size
             * param2: buffer offset
             * param3: HiFi4 core ID
             */

            printAboxFwLogs (pCtrData,
                             pSystemMsg->param1,
                             pSystemMsg->param2,
                             pSystemMsg->param3);
            break;

        case ABOX_INTERDSP_TEST_SUCCESSFUL:
            DBG_MSG (DBG_INFO, "Inter DSP Test Successfully\n");
            break;

        case ABOX_RELOAD_ADSP:
            DBG_MSG (DBG_INFO, "ABOX_RELOAD_ADSP : %d\n",
                     pSystemMsg->param1);
            aboxRestartAdsp (pCtrData, pSystemMsg->param1);
            break;

        case ABOX_SET_GICD_ITARGET:
            {
            uint32_t domain = pSystemMsg->param1;
            uint32_t spiId = pSystemMsg->param2;

            if (domain >= MAX_NUM_OF_DOMAIN)
                {
                return;
                }

            if (spiId < ABOX_GIC_SPI_OFS || spiId > ABOX_GIC_IRQ_MAXNUM)
                {
                return;
                }

            aboxGicForwardSpi (spiId);
            }
            break;

        default:
            DBG_MSG (DBG_ERR, "invalid abox rx msg type\n");
            break;
        }
    }

//abox_can_adsp_ipc
LOCAL BOOL aboxAdspCanIpc
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    _Vx_ticks_t     semTimeoutTicks;
    BOOL            ret = TRUE;

    //DBG_MSG (DBG_INFO, "pCtrData->adspState=%d\n", pCtrData->adspState);

    switch (pCtrData->adspState)
        {
        case ADSP_DISABLING:
        case ADSP_ENABLED:
            break;

        case ADSP_ENABLING:
            if (pCtrData->adspState == ADSP_ENABLED)
                {
                break;
                }

            semTimeoutTicks = ((ADSP_ENABLE_TIMEOUT_MS * sysClkRateGet ()) /
                               MSEC_PER_SEC);
            if (semTimeoutTicks == 0)
                {
                semTimeoutTicks = 1;
                }
            (void) semBTake (pCtrData->ipcDspStaWaitSem, semTimeoutTicks);

            if (pCtrData->adspState == ADSP_ENABLED)
                {
                break;
                }

            ret = FALSE;
            break;

        case ADSP_DISABLED:
        case ADSP_VERSION_MISMATCH:
            ret = FALSE;
            break;

        default:
            ret = FALSE;
            break;
        }

    return ret;
    }

//__abox_irq_handler or abox_irq_handler
LOCAL STATUS aboxIpcRxIsr
    (
    uint32_t        ipcId,
    ABOX_IPC_MSG *  pMsg,
    VXB_DEV_ID      devId
    )
    {
    uint32_t        pcmIrqId;
    ABOX_CTR_DATA * pCtrData;
    ABOX_GIC_DATA * pGicData;
    STATUS          ret = OK;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (devId);
    if (pCtrData == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    pGicData = (ABOX_GIC_DATA *) vxbDevSoftcGet (pCtrData->gicDev);
    if (pGicData == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    switch (ipcId)
        {
        case IPC_SYSTEM:
            aboxSystemIpcHandle (pCtrData, pMsg);
            break;

        case IPC_DMA_INTR:
            pcmIrqId = pMsg->task_id;
            if (pcmIrqId > pGicData->pcmIrqNum)
                {
                DBG_MSG (DBG_ERR, "\n");
                return ERROR;
                }

            if (pGicData->pcmIrqHandler[pcmIrqId].irqFunc)
                {
                ret = pGicData->pcmIrqHandler[pcmIrqId].irqFunc
                          (pcmIrqId, pGicData->pcmIrqHandler[pcmIrqId].devId);
                }
            else
                {
                DBG_MSG (DBG_ERR, "\n");
                ret = ERROR;
                }
            break;

        default:
            DBG_MSG (DBG_ERR, "invalid ipcId\n");
            break;
        }

    return ret;
    }

//abox_cpu_get_pmu_offset
LOCAL uint32_t aboxGetPmuOffset
    (
    uint32_t coreId
    )
    {
    uint32_t pmureg_offset[] =
        {
        ADSP_CORE0_PMU_OFFSET,
        ADSP_CORE1_PMU_OFFSET,
        ADSP_CORE2_PMU_OFFSET,
        };

    return pmureg_offset[coreId];
    }

//is_abox_adsp_core_on
LOCAL BOOL aboxDspCoreIsOn
    (
    ABOX_CTR_DATA * pCtrData,
    uint32_t        coreId
    )
    {
    uint32_t        pmuCoreRegBase;
    uint32_t        status = 0;

    if (coreId >= pCtrData->numOfAdsp)
        {
        return FALSE;
        }

    pmuCoreRegBase = pCtrData->adspData[coreId].pmuregOfs;

    if (vxbRegMapRead32 (pCtrData->pPmuRegMap,
                         pmuCoreRegBase + ADSP_CORE_STATUS,
                         &status) != OK)
        {
        return FALSE;
        }

    return (((status & AUD_CPU_STATUS_MASK) == AUD_CPU_STATUS_POWER_ON)
            ? TRUE : FALSE);
    }

//wait_abox_adsp_core_status
LOCAL STATUS aboxWaitAdspCoreStatus
    (
    ABOX_CTR_DATA * pCtrData,
    uint32_t        coreId,
    BOOL            waitStatus
    )
    {
    uint64_t        elapsedMs;
    uint64_t        startTick;
    uint64_t        curTick;
    uint32_t        sysClkRate = sysClkRateGet ();

    startTick = tick64Get ();
    do  {
        if (aboxDspCoreIsOn (pCtrData, coreId) == waitStatus)
            {
            return OK;
            }

        curTick = tick64Get ();
        elapsedMs = ((curTick - startTick) * 1000) / sysClkRate;

        } while (elapsedMs < ADSP_STATUS_TIMEOUT_MS);

    return ERROR;
    }

//get_early_audio_state
LOCAL uint32_t getEarlyAudState
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    uint32_t        earlyAudState;
    uint32_t        regVal;

    regVal = REG_READ_4 (pCtrData, pCtrData->hifiSfrBase, ADSP_CORE0_SW_RESET);
    earlyAudState = (regVal >> MISC_BIT) & 0x1;
    return earlyAudState;
    }

//abox_pad_retention
LOCAL void aboxPadRetention
    (
    ABOX_CTR_DATA * pCtrData,
    BOOL            retention
    )
    {
    uint32_t        regVal = 0;

    (void) vxbRegMapRead32 (pCtrData->pPmuRegMap, AUD_OUT, &regVal);

    if (retention)
        {
        regVal &= ~(1 << AUD_GPIO_PAD_RTO_MASK);
        }
    else
        {
        regVal |= 1 << AUD_GPIO_PAD_RTO_MASK;
        }

    (void) vxbRegMapWrite32 (pCtrData->pPmuRegMap, AUD_OUT, regVal);
    }

//abox_sft_pad_retention
LOCAL void aboxSftPadRetention
    (
    ABOX_CTR_DATA * pCtrData,
    BOOL            retention
    )
    {
    uint32_t        regVal = 0;

    (void) vxbRegMapRead32 (pCtrData->pPmuRegMap, TOP_OUT, &regVal);

    if (retention)
        {
        regVal &= (~RETENTION_DISABLE);
        }
    else
        {
        regVal |= RETENTION_DISABLE;
        }

    (void) vxbRegMapWrite32 (pCtrData->pPmuRegMap, TOP_OUT, regVal);
    }

//abox_query_dsp_status
LOCAL STATUS aboxQueryDspStatus
    (
    VXB_DEV_ID      pDev,
    ABOX_CTR_DATA * pCtrData
    )
    {
    ABOX_IPC_MSG    ipcMsg = {0};
    int32_t         ret;
    uint32_t        retry_count = 10000;

    if (aboxWaitAdspCoreStatus (pCtrData, ADSP_MASTER_CORE_ID, TRUE) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    ipcMsg.ipcid = IPC_SYSTEM;
    ipcMsg.task_id = ABOX_CHANGE_GEAR;
    ipcMsg.msg.system.msgtype = ABOX_CHANGE_GEAR;

    ret = abox_request_ipc (pDev, ipcMsg.ipcid, &ipcMsg, sizeof(ipcMsg), 1, 1);
    if (ret < 0)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    /* wait 10s for timeout */

    do  {
        vxbUsDelay (1000);
        retry_count--;
        } while (!aboxAdspCanIpc (pCtrData) && retry_count);

    if (retry_count == 0 && !aboxAdspCanIpc (pCtrData))
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    return OK;
    }

//abox_request_init_gic
LOCAL STATUS aboxReqInitGic
    (
    ABOX_CTR_DATA * pCtrData
    )
    {
    ABOX_GIC_DATA * pGicData;
    uint32_t        i;

    pGicData = (ABOX_GIC_DATA *) vxbDevSoftcGet (pCtrData->gicDev);
    if (pGicData == NULL)
        {
        return ERROR;
        }

    if (pGicData->gicdSettingMode == GICD_SET_ALL_WITH_DOMAINS)
        {
        for (i = SYS_DOMAIN; i < MAX_NUM_OF_DOMAIN; i++)
            {
            if (aboxGicInit (pCtrData, i) != OK)
                {
                return ERROR;
                }
            }
        }
    else
        {
        if (aboxGicInit (pCtrData, pCtrData->myDomainId) != OK)
            {
            return ERROR;
            }
        }

    return OK;
    }

LOCAL STATUS aboxDevResInit
    (
    int32_t         fdtOfs,
    ABOX_CTR_DATA * pCtrData
    )
    {
    uint32_t *      pValue;
    int32_t         len;

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-rdma", &len);
    if (pValue != NULL)
        {
        pCtrData->numOfRdma = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-wdma", &len);
    if (pValue != NULL)
        {
        pCtrData->numOfWdma = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-uaif", &len);
    if (pValue != NULL)
        {
        pCtrData->numOfUaif = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-pcm_playback",
                                      &len);
    if (pValue != NULL)
        {
        pCtrData->numOfPcmPlayback = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-pcm_capture",
                                      &len);
    if (pValue != NULL)
        {
        pCtrData->numOfPcmCapture = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pCtrData->rdmaDev = (VXB_DEV_ID *) vxbMemAlloc (sizeof (VXB_DEV_ID) *
                                                    pCtrData->numOfRdma);
    if (pCtrData->rdmaDev == NULL)
        {
        return ERROR;
        }

    pCtrData->wdmaDev = (VXB_DEV_ID *) vxbMemAlloc (sizeof (VXB_DEV_ID) *
                                                    pCtrData->numOfWdma);
    if (pCtrData->wdmaDev == NULL)
        {
        goto errOut;
        }

    pCtrData->uaifDev = (VXB_DEV_ID *) vxbMemAlloc (sizeof (VXB_DEV_ID) *
                                                    pCtrData->numOfUaif);
    if (pCtrData->uaifDev == NULL)
        {
        goto errOut;
        }

    pCtrData->pcmPlaybackDev = (VXB_DEV_ID *) vxbMemAlloc
                                                (sizeof (VXB_DEV_ID) *
                                                 pCtrData->numOfPcmPlayback);
    if (pCtrData->pcmPlaybackDev == NULL)
        {
        goto errOut;
        }

    pCtrData->pcmCaptureDev = (VXB_DEV_ID *) vxbMemAlloc
                                                (sizeof (VXB_DEV_ID) *
                                                 pCtrData->numOfPcmCapture);
    if (pCtrData->pcmCaptureDev == NULL)
        {
        goto errOut;
        }

    return OK;

errOut:
    if (pCtrData->rdmaDev != NULL)
        {
        vxbMemFree (pCtrData->rdmaDev);
        }

    if (pCtrData->wdmaDev != NULL)
        {
        vxbMemFree (pCtrData->wdmaDev);
        }

    if (pCtrData->uaifDev != NULL)
        {
        vxbMemFree (pCtrData->uaifDev);
        }

    if (pCtrData->pcmPlaybackDev != NULL)
        {
        vxbMemFree (pCtrData->pcmPlaybackDev);
        }

    return ERROR;
    }

LOCAL STATUS aboxClkInit
    (
    VXB_DEV_ID      pDev,
    ABOX_CTR_DATA * pCtrData
    )
    {
    pCtrData->aboxClks = (VXB_CLK_ID *) vxbMemAlloc (sizeof (VXB_CLK_ID) *
                                                     ABOX_CLK_CNT);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    pCtrData->aboxClks[CLK_DIV_AUD_CPU] = vxbClkGet (pDev, "cpu");
    if (pCtrData->aboxClks[CLK_DIV_AUD_CPU] == NULL)
        {
        goto errOut;
        }

    pCtrData->aboxClks[CLK_PLL_OUT_AUD] = vxbClkGet (pDev, "pll");
    if (pCtrData->aboxClks[CLK_PLL_OUT_AUD] == NULL)
        {
        goto errOut;
        }

    pCtrData->aboxClks[CLK_DIV_AUD_AUDIF] = vxbClkGet (pDev, "audif");
    if (pCtrData->aboxClks[CLK_DIV_AUD_AUDIF] == NULL)
        {
        goto errOut;
        }

    pCtrData->aboxClks[CLK_DIV_AUD_NOC] = vxbClkGet (pDev, "bus");
    if (pCtrData->aboxClks[CLK_DIV_AUD_NOC] == NULL)
        {
        goto errOut;
        }

    pCtrData->aboxClks[CLK_DIV_AUD_CNT] = vxbClkGet (pDev, "audcnt");
    if (pCtrData->aboxClks[CLK_DIV_AUD_CNT] == NULL)
        {
        goto errOut;
        }

    return OK;

errOut:
    if (pCtrData->aboxClks != NULL)
        {
        vxbMemFree (pCtrData->aboxClks);
        }

    return ERROR;
    }

LOCAL STATUS aboxDspInit
    (
    VXB_DEV_ID          pDev,
    int32_t             fdtOfs,
    ABOX_CTR_DATA *     pCtrData
    )
    {
    uint32_t *          pValue;
    int32_t             len;
    uint32_t            i;
    uint32_t            phandle;
    int32_t             offset;
    FDT_RSV_MEM_INFO *  pFdtRsvMemInfo;
    uint32_t            mmuAttr;

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "samsung,num-of-adsp", &len);
    if (pValue != NULL)
        {
        pCtrData->numOfAdsp = vxFdt32ToCpu (*pValue);
        }
    else
        {
        return ERROR;
        }

    pCtrData->adspData = (ADSP_DATA *) vxbMemAlloc (sizeof (ADSP_DATA) *
                                                    pCtrData->numOfAdsp);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    pValue = (uint32_t *) vxFdtPropGet (fdtOfs, "memory-region", &len);
    if (pValue == NULL || len != (sizeof (uint32_t) * pCtrData->numOfAdsp))
        {
        goto errOut;
        }

    for (i = 0; i < pCtrData->numOfAdsp; i++, pValue++)
        {
        phandle = vxFdt32ToCpu (*pValue);
        offset = vxFdtNodeOffsetByPhandle (phandle);

        pFdtRsvMemInfo = vxFdtRsvDescGet (offset, 0);
        if (pFdtRsvMemInfo == NULL)
            {
            goto errOut;
            }

        pCtrData->adspData[i].physAddr = pFdtRsvMemInfo->physRsvMemAddr;
        pCtrData->adspData[i].size = pFdtRsvMemInfo->len;

        mmuAttr = MMU_ATTR_VALID | MMU_ATTR_SUP_RW | MMU_ATTR_CACHE_OFF;
        pCtrData->adspData[i].virtAddr = (VIRT_ADDR) pmapGlobalMap
                                                (
                                                pCtrData->adspData[i].physAddr,
                                                pCtrData->adspData[i].size,
                                                mmuAttr
                                                );

        pCtrData->adspData[i].pmuregOfs = aboxGetPmuOffset(i);

        DBG_MSG (DBG_INFO, "dsp[%d] physAddr=0x%llx virtAddr=0x%llx size=0x%llx"
                           " pmuregOfs=0x%x\n", i,
                           pCtrData->adspData[i].physAddr,
                           pCtrData->adspData[i].virtAddr,
                           pCtrData->adspData[i].size,
                           pCtrData->adspData[i].pmuregOfs);
        }

    return OK;

errOut:
    if (pCtrData->adspData != NULL)
        {
        vxbMemFree (pCtrData->adspData);
        }

    return ERROR;
    }

//abox_get_sfr
LOCAL STATUS aboxGetRegs
    (
    VXB_DEV_ID          pDev,
    int32_t             fdtOfs,
    ABOX_CTR_DATA *     pCtrData
    )
    {
    uint16_t            idx;
    VXB_RESOURCE *      pResSfr;
    VXB_RESOURCE *      pResSysregSfr;
    VXB_RESOURCE *      pResTcm0;
    VXB_RESOURCE *      pResHifiSfr;
    VXB_RESOURCE *      pResTimer;

    idx = (uint16_t) vxFdtPropStrIndexGet (fdtOfs, "reg-names", "sfr");
    pResSfr = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResSfr == NULL) || (pResSfr->pRes == NULL))
        {
        return ERROR;
        }
    pCtrData->sfrBase = ((VXB_RESOURCE_ADR *)(pResSfr->pRes))->virtAddr;
    pCtrData->regHandle = ((VXB_RESOURCE_ADR *)(pResSfr->pRes))->pHandle;

    DBG_MSG (DBG_INFO, "abox sfr reg vaddr=0x%llx paddr=0x%llx\n",
             pCtrData->sfrBase,
             ((VXB_RESOURCE_ADR *)(pResSfr->pRes))->start);

    idx = (uint16_t) vxFdtPropStrIndexGet (fdtOfs, "reg-names", "sysreg_aud_sfr");
    pResSysregSfr = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResSysregSfr == NULL) || (pResSysregSfr->pRes == NULL))
        {
        (void) vxbResourceFree (pDev, pResSfr);
        return ERROR;
        }
    pCtrData->sysregSfrBase = ((VXB_RESOURCE_ADR *)
                                  (pResSysregSfr->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "abox sysreg_aud_sfr reg vaddr=0x%llx paddr=0x%llx\n",
             pCtrData->sysregSfrBase,
             ((VXB_RESOURCE_ADR *)(pResSysregSfr->pRes))->start);

    idx = (uint16_t) vxFdtPropStrIndexGet (fdtOfs, "reg-names", "tcm0");
    pResTcm0 = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResTcm0 == NULL) || (pResTcm0->pRes == NULL))
        {
        (void) vxbResourceFree (pDev, pResSfr);
        (void) vxbResourceFree (pDev, pResSysregSfr);
        return ERROR;
        }
    pCtrData->tcm0Base = ((VXB_RESOURCE_ADR *)(pResTcm0->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "abox tcm0 reg vaddr=0x%llx paddr=0x%llx\n",
             pCtrData->tcm0Base,
             ((VXB_RESOURCE_ADR *)(pResTcm0->pRes))->start);

    idx = (uint16_t) vxFdtPropStrIndexGet (fdtOfs, "reg-names", "hifi_sfr");
    pResHifiSfr = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResHifiSfr == NULL) || (pResHifiSfr->pRes == NULL))
        {
        (void) vxbResourceFree (pDev, pResSfr);
        (void) vxbResourceFree (pDev, pResSysregSfr);
        (void) vxbResourceFree (pDev, pResTcm0);
        return ERROR;
        }
    pCtrData->hifiSfrBase = ((VXB_RESOURCE_ADR *)(pResHifiSfr->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "abox hifi_sfr reg vaddr=0x%llx paddr=0x%llx\n",
             pCtrData->hifiSfrBase,
             ((VXB_RESOURCE_ADR *)(pResHifiSfr->pRes))->start);

    idx = (uint16_t) vxFdtPropStrIndexGet (fdtOfs, "reg-names", "abox_timer");
    pResTimer = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResTimer == NULL) || (pResTimer->pRes == NULL))
        {
        (void) vxbResourceFree (pDev, pResSfr);
        (void) vxbResourceFree (pDev, pResSysregSfr);
        (void) vxbResourceFree (pDev, pResTcm0);
        (void) vxbResourceFree (pDev, pResHifiSfr);
        return ERROR;
        }
    pCtrData->timerBase = ((VXB_RESOURCE_ADR *)(pResTimer->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "abox abox_timer reg vaddr=0x%llx paddr=0x%llx\n",
             pCtrData->timerBase,
             ((VXB_RESOURCE_ADR *)(pResTimer->pRes))->start);

    SPIN_LOCK_ISR_INIT (&pCtrData->regmapSpinlockIsr, 0);

    return OK;
    }

LOCAL STATUS aboxCtrSubNodeAttach
    (
    VXB_DEV_ID              pDev,
    int32_t                 parentOfs,
    char *                  matchNames[],
    uint32_t                nameNum
    )
    {
    const char *            compatProp;
    int32_t                 offset;
    int32_t                 len;
    uint32_t                i;
    BOOL                    isMatch;
    VXB_DEV_ID              pCur;
    VXB_FDT_BUS_DEV_INFO *  pFdtBusDevInfo;
    VXB_FDT_DEV *           pFdtDev;
    char *                  pUnitAddr;
    char                    name[MAX_DEV_NAME_LEN] = {0};

    if (pDev == NULL || matchNames == NULL)
        {
        return ERROR;
        }

    for (offset = VX_FDT_CHILD (parentOfs); offset > 0;
         offset = VX_FDT_PEER (offset))
        {
        if (vxFdtIsEnabled (offset) == FALSE)
            {
            continue;
            }

        compatProp = vxFdtPropGet (offset, "compatible", &len);
        if (compatProp == NULL)
            {
            continue;
            }

        isMatch = FALSE;
        for (i = 0; i < nameNum; i++)
            {
            if ((strlen (compatProp) == strlen (matchNames[i])) &&
                (strncmp (compatProp, matchNames[i], strlen (compatProp)) == 0))
                {
                isMatch = TRUE;
                break;
                }
            }

        if (isMatch == FALSE)
            {
            continue;
            }

        DBG_MSG (DBG_INFO, "macth node compatProp=\"%s\"\n", compatProp);

        pCur = NULL;
        if (vxbDevCreate (&pCur) != OK)
            {
            continue;
            }

        pFdtBusDevInfo = (VXB_FDT_BUS_DEV_INFO *)
                             vxbMemAlloc (sizeof (VXB_FDT_BUS_DEV_INFO));
        if (pFdtBusDevInfo == NULL)
            {
            (void) vxbDevDestroy (pCur);
            continue;
            }

        pFdtDev = &pFdtBusDevInfo->vxbFdtDev;

        vxbFdtDevSetup (offset, pFdtDev);

        vxbDevNameSet (pCur, pFdtDev->name, FALSE);

        pUnitAddr = vxbFdtUnitAddrGet ((uint32_t)offset, name, MAX_DEV_NAME_LEN);
        if (pUnitAddr == NULL)
            {
            (void) snprintf (name, MAX_DEV_NAME_LEN, "0");
            }
        else
            {
            (void) snprintf (name, MAX_DEV_NAME_LEN, "%s", pUnitAddr);
            }

        vxbDevNameAddrSet (pCur, name, TRUE);

        if (vxbResourceInit (&pFdtBusDevInfo->vxbResList) != OK)
            {
            vxbMemFree (pFdtBusDevInfo);
            (void) vxbDevDestroy (pCur);
            continue;
            }

        if (vxbFdtRegGet (&pFdtBusDevInfo->vxbResList, pFdtDev) != OK)
            {
            vxbFdtResFree (&pFdtBusDevInfo->vxbResList);
            vxbMemFree (pFdtBusDevInfo);
            (void) vxbDevDestroy (pCur);
            continue;
            }

        if (vxbFdtIntGet (&pFdtBusDevInfo->vxbResList, pFdtDev) != OK)
            {
            vxbFdtResFree (&pFdtBusDevInfo->vxbResList);
            vxbMemFree (pFdtBusDevInfo);
            (void) vxbDevDestroy (pCur);
            continue;
            }

        vxbDevIvarsSet (pCur, (void *) pFdtBusDevInfo);
        vxbDevClassSet (pCur, VXB_BUSID_FDT);

        (void) vxbDevAdd (pDev, pCur);
        }

    return OK;
    }

/******************************************************************************
*
* aboxCtrProbe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS aboxCtrProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxCtrMatch, NULL);
    }

LOCAL STATUS _aboxCtrAttach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;
    ABOX_CTR_DATA * pCtrData;
    uint32_t *      pValue;
    int32_t         len;
    int32_t         offset;

    if (pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    pCtrData = (ABOX_CTR_DATA *) vxbMemAlloc (sizeof (ABOX_CTR_DATA));
    if (pCtrData == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    /* set pAboxCtrData first due to other modules may use it */

    pAboxCtrData = pCtrData;
    vxbDevSoftcSet (pDev, pCtrData);

    pCtrData->pDev = pDev;
    pCtrData->cmpnt = NULL;
    pCtrData->myDomainId = SYS_DOMAIN;

    pCtrData->perfData.checked = FALSE;
    pCtrData->perfData.period_t = 0;

    /* attach GIC and mailbox first, the sequence cannot be changed */

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              gicMatchName, NELEMENTS (gicMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              mboxMatchName, NELEMENTS (mboxMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* IPC init */

    SPIN_LOCK_ISR_INIT (&pCtrData->ipcQueSpinlockIsr, 0);
    SPIN_LOCK_ISR_INIT (&pCtrData->ipcProcSpinlockIsr, 0);
    pCtrData->ipcTxSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pCtrData->ipcTxSem == SEM_ID_NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pCtrData->ipcDspStaWaitSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pCtrData->ipcDspStaWaitSem == SEM_ID_NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pCtrData->ipcTxTask = taskSpawn (ABOX_IPC_TX_TASK_NAME,
                                     ABOX_IPC_TX_TASK_PRI,
                                     0,
                                     ABOX_IPC_TX_TASK_STACK,
                                     (FUNCPTR) aboxIpcQueTxTask,
                                     (_Vx_usr_arg_t) pCtrData,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (pCtrData->ipcTxTask == TASK_ID_ERROR)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    SPIN_LOCK_ISR_INIT (&fwDlSpinlockIsr, 0);

    /* DTS configuration */

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,abox_target",
                                      &len);
    if (pValue != NULL)
        {
        aboxTarget = vxFdt32ToCpu (*pValue);
        }
    else
        {
        aboxTarget = ABOX_V920; /* default V920 EVT2 */
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,abox_sfr_set",
                                      &len);
    if (pValue != NULL)
        {
        aboxSfrset = vxFdt32ToCpu (*pValue);
        }
    else
        {
        aboxSfrset = ABOX_V920_EVT2_SFR; /* default V920 EVT2 */
        }

    if (aboxGetRegs (pDev, pFdtDev->offset, pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxDspInit (pDev, pFdtDev->offset, pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /*
     * init PMU here for abox temporarily using,
     * PMU driver shoule be condsider later individually
     */

    offset = vxFdtNodeOffsetByCompatible (-1, "samsung,exynos-pmu");
    if (offset < 0)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (vxbRegMapGetByPhandle (offset, "samsung,syscon-phandle",
                               &pCtrData->pPmuRegMap) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxClkInit (pDev, pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* GIC and mailbox must be initialized before */

    if (pCtrData->gicDev == NULL
        || pCtrData->mboxRxDev == NULL
        || pCtrData->mboxTxDev == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxMailboxRegisterIpcHandler (pCtrData->mboxRxDev,
                                       aboxIpcRxIsr,
                                       pDev) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxDevResInit (pFdtDev->offset, pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxReqInitGic (pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* check DSP boot status */

    if (aboxQueryDspStatus (pDev, pCtrData) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* component register */

    if (vxSndCpntRegister (pDev, &aboxCmpnt, aboxDais,
                           NELEMENTS (aboxDais)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* attach other modules, the sequence cannot be changed */

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              rdmaMatchName, NELEMENTS (rdmaMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              pcmPlaybackMatchName,
                              NELEMENTS (pcmPlaybackMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              uaifMatchName, NELEMENTS (uaifMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              wdmaMatchName, NELEMENTS (wdmaMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (aboxCtrSubNodeAttach (pDev, pFdtDev->offset,
                              pcmCaptureMatchName,
                              NELEMENTS (pcmCaptureMatchName)) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* enable abox clks */

    if (vxbClkEnable (pCtrData->aboxClks[CLK_DIV_AUD_CPU]) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (vxbClkEnable (pCtrData->aboxClks[CLK_DIV_AUD_AUDIF]) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    return OK;

errOut:
    snd_soc_unregister_component (pDev);

    aboxMailboxUnregisterIpcHandler (pCtrData->mboxRxDev);

    if (pCtrData->ipcTxTask != NULL)
        {
        (void) taskDelete (pCtrData->ipcTxTask);
        }

    if (pCtrData->ipcTxSem != SEM_ID_NULL)
        {
        (void) semDelete (pCtrData->ipcTxSem);
        }

    if (pCtrData != NULL)
        {
        pAboxCtrData = NULL;
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pCtrData);
        }

    return ERROR;
    }

LOCAL void aboxCtrlAttachTask
    (
    VXB_DEV_ID  pDev
    )
    {
    if (_aboxCtrAttach (pDev) != OK)
        {
        DBG_MSG (DBG_ERR, "abox ctrl attach init failed\n");
        return;
        }

    /* all abox devices are ready, let machine to attach */

    if (aboxMachineAttachTaskId == TASK_ID_ERROR)
        {
        DBG_MSG (DBG_ERR, "abox machine attach task is not ready\n");
        return;
        }

    taskActivate (aboxMachineAttachTaskId);
    }

/******************************************************************************
*
* aboxCtrAttach - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

LOCAL STATUS aboxCtrAttach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;
    TASK_ID         taskId;

    if (pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    /* abox attach init will start while system init is done */

    taskId = taskSpawn (ABOX_ATTACH_TASK_NAME,
                        ABOX_ATTACH_TASK_PRI,
                        0,
                        ABOX_ATTACH_TASK_STACK,
                        (FUNCPTR) aboxCtrlAttachTask,
                        (_Vx_usr_arg_t) pDev,
                        0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (taskId == TASK_ID_ERROR)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    return OK;
    }

LOCAL VXB_RESOURCE * aboxCtrResAlloc
    (
    VXB_DEV_ID              pDev,
    VXB_DEV_ID              pChild,
    UINT32                  id
    )
    {
    VXB_FDT_BUS_DEV_INFO *  pVxbFdtDevInfo;
    VXB_RESOURCE *          vxbRes;

    if (pChild == NULL)
        {
        return (NULL);
        }

    pVxbFdtDevInfo = (VXB_FDT_BUS_DEV_INFO *) vxbDevIvarsGet (pChild);
    if (pVxbFdtDevInfo == NULL)
        {
        return (NULL);
        }

    vxbRes = vxbResourceFind (&pVxbFdtDevInfo->vxbResList, id);
    if (vxbRes == NULL)
        {
        return (NULL);
        }

    if ((VXB_RES_TYPE (vxbRes->id) == VXB_RES_MEMORY)
        && (vxbRegMap (vxbRes) == OK))
        {
        return (vxbRes);
        }
    else if ((VXB_RES_TYPE (vxbRes->id) == VXB_RES_IRQ)
             && (vxbIntMap (vxbRes) == OK))
        {
        return (vxbRes);
        }

    return (NULL);
    }

LOCAL STATUS aboxCtrResFree
    (
    VXB_DEV_ID              pDev,
    VXB_DEV_ID              pChild,
    VXB_RESOURCE *          pRes
    )
    {
    VXB_FDT_BUS_DEV_INFO *  pVxbFdtDevInfo;

    if (pChild == NULL)
        {
        return (ERROR);
        }

    pVxbFdtDevInfo = (VXB_FDT_BUS_DEV_INFO *) vxbDevIvarsGet (pChild);
    if (pVxbFdtDevInfo == NULL)
        {
        return (ERROR);
        }

    if (VXB_RES_TYPE (pRes->id) == VXB_RES_MEMORY)
        {
        return (vxbRegUnmap(pRes));
        }

    /* catch unsupported VXB_RES types */

    return (ERROR);
    }

LOCAL VXB_RESOURCE_LIST * aboxCtrResListGet
    (
    VXB_DEV_ID              pDev,
    VXB_DEV_ID              pChild
    )
    {
    VXB_FDT_BUS_DEV_INFO *  pVxbFdtDevInfo;

    if (pChild == NULL)
        {
        return (NULL);
        }

    pVxbFdtDevInfo = (VXB_FDT_BUS_DEV_INFO *) vxbDevIvarsGet (pChild);
    if (pVxbFdtDevInfo == NULL)
        {
        return (NULL);
        }

    return (&pVxbFdtDevInfo->vxbResList);
    }

LOCAL VXB_FDT_DEV * aboxCtrDevGet
    (
    VXB_DEV_ID              pDev,
    VXB_DEV_ID              pChild
    )
    {
    VXB_FDT_BUS_DEV_INFO *  pVxbFdtDevInfo;

    if (pChild == NULL)
        {
        return (NULL);
        }

    pVxbFdtDevInfo = (VXB_FDT_BUS_DEV_INFO *) vxbDevIvarsGet (pChild);
    if (pVxbFdtDevInfo == NULL)
        {
        return (NULL);
        }

    return (&pVxbFdtDevInfo->vxbFdtDev);
    }

//abox_convert_lb_to_dma
LOCAL int32_t aboxConvertLbToDma
    (
    uint32_t  loopbackId
    )
    {
    switch (loopbackId)
        {
        case ABOX_LOOPBACK0:
            return ABOX_RDMA20;

        case ABOX_LOOPBACK1:
            return ABOX_RDMA21;

        default:
            DBG_MSG (DBG_ERR, "Wrong Loopback id : %u", loopbackId);
            return -EINVAL;
        }
    }

//abox_sifsx_cnt_val
LOCAL uint32_t aboxSifsxCntVal
    (
    uint64_t    bclkCnt,
    uint32_t    rate,
    uint32_t    physicalWidth,
    uint32_t    channels
    )
    {
    /*
     * SIFSx_CNT_VAL = K x BCLK_CNT / SampleRate - 1
     * K means the number of Sample in 64bit arch
     * (e.g> 16bit 2Ch --> 64 / 16 bit / 2Ch  --> k = 2
     * But this value must be Zero, when SPUS_SIFS connects to UAIF
     * =======================================================
     * Bit width    |     Channel Number    |       k
     * =======================================================
     * 16 bit       |         mono          |       4
     *              |        stereo         |       2
     *              |          4ch          |       1
     *              |          6ch          |       2/3
     *              |          8ch          |       0.5
     *              |         16ch          |       0.25
     *              |         32ch          |       0.125
     * =======================================================
     * 24/32 bit    |         mono          |       2
     *              |        stereo         |       1
     *              |          4ch          |       0.5
     *              |          6ch          |       1/3
     *              |          8ch          |       0.25
     *              |         16ch          |       0.125
     *              |         32ch          |       0.0625
     * =======================================================
     */
    uint32_t  sifsxCntVal;
    uint32_t  theNumOfData;

    if (physicalWidth == 0 || channels == 0 || rate == 0)
        {
        return 0;
        }

    theNumOfData = 64 / physicalWidth;
    sifsxCntVal = (uint32_t) (((bclkCnt * theNumOfData / channels) / rate) - 1);

    return sifsxCntVal;
    }

//abox_loopback_dai_set_fmt
LOCAL STATUS aboxLoopbackDaiSetFmt
    (
    VX_SND_SOC_DAI *    dai,
    uint32_t            fmt
    )
    {
    DBG_MSG (DBG_INFO, "\n");
    return OK;
    }

//abox_loopback_dai_hw_params
LOCAL STATUS aboxLoopbackDaiHwParams
    (
    SND_PCM_SUBSTREAM *     substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai
    )
    {
    ABOX_CTR_DATA *         pCtrData;
    struct abox_dma_data *  pDmaData;
    VX_SND_SOC_COMPONENT *  cmpnt;
    uint32_t                loopbackId;
    uint32_t                rate;
    uint32_t                width;
    uint32_t                channels;
    int32_t                 dmaId;
    uint64_t                audcntClk;
    uint32_t                cntVal;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (dai->pDev);
    if (pCtrData == NULL)
        {
        DBG_MSG (DBG_ERR, "invalid pCtrData\n");
        return ERROR;
        }

    loopbackId = dai->id;
    rate = PARAMS_RATE (params);
    width = PARAMS_WIDTH (params);
    channels = PARAMS_CHANNELS (params);

    if (substream->stream->direct != SNDRV_PCM_STREAM_CAPTURE)
        {
        DBG_MSG (DBG_ERR, "invalid direction\n");
        return ERROR;
        }

    dmaId = aboxConvertLbToDma (loopbackId);
    if (dmaId < 0)
        {
        DBG_MSG (DBG_ERR, "invalid dmaId\n");
        return ERROR;
        }

    pDmaData = (struct abox_dma_data *) vxbDevSoftcGet (pCtrData->rdmaDev[dmaId]);
    if (pDmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "can't get pDmaData\n");
        return ERROR;
        }

    cmpnt = pDmaData->cmpnt;
    if (cmpnt == NULL)
        {
        DBG_MSG (DBG_ERR, "can't get cmpnt\n");
        return ERROR;
        }

    if (abox_check_bclk_from_cmu_clock (pCtrData, rate, channels, width) != OK)
        {
        DBG_MSG (DBG_ERR, "Unable to register bclk usage\n");
        }

    if (aboxClkRateSet (pCtrData->aboxClks[CLK_DIV_AUD_CNT],
                        (rate * width * channels)) != OK)
        {
        DBG_MSG (DBG_ERR, "Failed to set audcnt clock\n");
        return ERROR;
        }

    audcntClk = vxbClkRateGet (pCtrData->aboxClks[CLK_DIV_AUD_CNT]);
    cntVal = aboxSifsxCntVal (audcntClk, rate, width, channels);

    DBG_MSG (DBG_INFO,
             "[%d](width:%ubit channels:%u SampleRate: %uHz at %luHz): %u\n",
             dmaId, width, channels, rate, audcntClk, cntVal);

    (void) rdmaSifsCntValSet (pDmaData, cntVal);

    return OK;
    }

//abox_loopback_dai_hw_free
LOCAL STATUS aboxLoopbackDaiHwFree
    (
    SND_PCM_SUBSTREAM *     substream,
    VX_SND_SOC_DAI *        dai
    )
    {
    ABOX_CTR_DATA *         pCtrData;
    struct abox_dma_data *  pDmaData;
    VX_SND_SOC_COMPONENT *  cmpnt;
    uint32_t                loopbackId;
    int32_t                 dmaId;

    loopbackId = dai->id;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (dai->pDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    if (substream->stream->direct != SNDRV_PCM_STREAM_CAPTURE)
        {
        return ERROR;
        }

    DBG_MSG (DBG_INFO, "[%d]\n", loopbackId);

    (void) abox_check_bclk_from_cmu_clock (pCtrData, 0, 0, 0);

    dmaId = aboxConvertLbToDma (loopbackId);
    if (dmaId < 0)
        {
        //return dmaId;
        return ERROR;
        }

    pDmaData = (struct abox_dma_data *) vxbDevSoftcGet (pCtrData->rdmaDev[dmaId]);
    if (pDmaData == NULL)
        {
        return ERROR;
        }

    cmpnt = pDmaData->cmpnt;
    if (cmpnt == NULL)
        {
        return ERROR;
        }

    (void) rdmaSifsCntValSet (pDmaData, 0);

    return OK;
    }

//primary_capture_dev_get
LOCAL STATUS primaryCaptureDevGet
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    VX_SND_SOC_COMPONENT *  cmpnt;
    ABOX_CTR_DATA *         pCtrData;
    ABOX_PCM_CTRL_DATA *    pcmDevData;
    uint32_t                id;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)

        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpnt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        }

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    for (id = 0; id < pCtrData->numOfPcmCapture; id++)
        {
        if (pCtrData->pcmCaptureDev[id] != NULL)
            {
            pcmDevData = (ABOX_PCM_CTRL_DATA *)
                             vxbDevSoftcGet (pCtrData->pcmCaptureDev[id]);
            ucontrol->value.integer32.value[0] = pcmDevData->pcm_dev_num;
            return OK;
            }
        }

    if (id == pCtrData->numOfPcmCapture)
        {
        ucontrol->value.integer32.value[0] = ABOX_PCM_CAPTURE_NO_CONNECT;
        return OK;
        }

    return OK;
    }

//primary_capture_dev_put
LOCAL STATUS primaryCaptureDevPut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    DBG_MSG (DBG_INFO, "does not support to revise pcm_device number\n");

    return OK;
    }

//primary_playback_dev_get
LOCAL STATUS primaryPlaybackDevGet
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    VX_SND_SOC_COMPONENT *  cmpnt;
    ABOX_CTR_DATA *         pCtrData;
    ABOX_PCM_CTRL_DATA *    pcmDevData;
    int                     id;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpnt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        }

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    for (id = 0; id < pCtrData->numOfPcmPlayback; ++id)
        {
        if (pCtrData->pcmPlaybackDev[id] != NULL)
            {
            pcmDevData = (ABOX_PCM_CTRL_DATA *)
                             vxbDevSoftcGet (pCtrData->pcmPlaybackDev[id]);
            ucontrol->value.integer32.value[0] = pcmDevData->pcm_dev_num;
            return OK;
            }
        }

    if (id == pCtrData->numOfPcmPlayback)
        {
        ucontrol->value.integer32.value[0] = ABOX_PCM_PLAYBACK_NO_CONNECT;
        return OK;
        }

    return OK;
    }

//primary_playback_dev_put
LOCAL STATUS primaryPlaybackDevPut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    DBG_MSG (DBG_INFO, "does not support to revise pcm_device number\n");

    return OK;
    }

//abox_array_ref_info
LOCAL STATUS aboxArrayRefInfo
    (
    VX_SND_CONTROL *    kcontrol,
    VX_SND_CTRL_INFO *  uinfo
    )
    {
    VX_SND_MIXER_CTRL * mc;

    mc = (VX_SND_MIXER_CTRL *) kcontrol->privateValue;

    if (mc->platformMax == 0)
        {
        mc->platformMax = mc->max;
        }

    uinfo->type = VX_SND_CTL_DATA_TYPE_INTEGER;
    uinfo->count = ABOX_ARRAY_REF_CNT;
    uinfo->value.integer32.min = 0;
    uinfo->value.integer32.max = mc->platformMax - mc->min;

    return OK;
    }

//abox_array_ref_get
LOCAL STATUS aboxArrayRefGet
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    uint32_t *              value = NULL;
    uint32_t                i;

    value = ucontrol->value.integer32.value;
    for (i = 0; i < ABOX_ARRAY_REF_CNT; i++)
        {
        value[i] = aboxArrayRefValue[i];
        DBG_MSG (DBG_INFO, "value[%d]:%ld\n", i, value[i]);
        }

    return OK;
    }

//abox_array_ref_put
LOCAL STATUS aboxArrayRefPut
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    uint32_t *              value = NULL;
    uint32_t                i;

    value = ucontrol->value.integer32.value;
    for (i = 0; i < ABOX_ARRAY_REF_CNT; i++)
        {
        aboxArrayRefValue[i] = value[i];
        DBG_MSG (DBG_INFO, "%s(%d) value[%d]:%ld\n", i, value[i]);
        }

    return OK;
    }

//abox_get_avb
LOCAL STATUS aboxGetAvb
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    VX_SND_SOC_COMPONENT *  cmpnt;
    ABOX_CTR_DATA *         pCtrData;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpnt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        }

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    DBG_MSG (DBG_INFO, "value=%u\n", pCtrData->avbStatus);
    ucontrol->value.enumerated.item[0] = pCtrData->avbStatus;

    return OK;
    }

//abox_set_avb
LOCAL STATUS aboxSetAvb
    (
    VX_SND_CONTROL *        kcontrol,
    VX_SND_CTRL_DATA_VAL *  ucontrol
    )
    {
    ABOX_IPC_MSG            msg;
    VX_SND_SOC_COMPONENT *  cmpnt;
    ABOX_CTR_DATA *         pCtrData;
    uint32_t                status;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpnt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        }

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    status = ucontrol->value.enumerated.item[0];

    msg.ipcid = IPC_SYSTEM;
    msg.msg.system.msgtype = ABOX_SET_AVB;
    msg.msg.system.param1 = status;
    DBG_MSG (DBG_INFO, "status = %d\n", status);
    pCtrData->avbStatus = status;

    abox_request_ipc (cmpnt->pDev, msg.ipcid, &msg, sizeof (msg), 1, 1);

    return OK;
    }

LOCAL uint32_t aboxCmpntRead
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs
    )
    {
    ABOX_CTR_DATA *         pCtrData;
    uint32_t                regVal;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return NULL;
        }

    aboxCtrRegmapRead (pCtrData, pCtrData->sfrBase, regOfs, &regVal);

    return regVal;
    }

LOCAL int32_t aboxCmpntWrite
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs,
    uint32_t                val
    )
    {
    ABOX_CTR_DATA *         pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (cmpnt->pDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    aboxCtrRegmapWrite (pCtrData, pCtrData->sfrBase, regOfs, val);

    return 0;
    }

//abox_cmpnt_probe
LOCAL int32_t aboxCmpntProbe
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    ABOX_CTR_DATA *         pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (component->pDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    pCtrData->cmpnt = component;

    return 0;
    }

//abox_cmpnt_remove
LOCAL void aboxCmpntRemove
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    DBG_MSG (DBG_INFO, "\n");
    }

/* APIs */

//abox_get_abox_data
ABOX_CTR_DATA * aboxCtrGetData (void)
    {
    return pAboxCtrData;
    }

//abox_register_pcm_playback
int32_t abox_register_pcm_playback
    (
    VXB_DEV_ID      ctrDev,
    VXB_DEV_ID      pcmPlaybackDev,
    uint32_t        id
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    if (id < pCtrData->numOfPcmPlayback)
        {
        pCtrData->pcmPlaybackDev[id] = pcmPlaybackDev;
        }
    else
        {
        return -EINVAL;
        }

    return 0;
    }

//abox_register_pcm_capture
int32_t abox_register_pcm_capture
    (
    VXB_DEV_ID      ctrDev,
    VXB_DEV_ID      pcmCaptureDev,
    uint32_t        id
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    if (id < pCtrData->numOfPcmCapture)
        {
        pCtrData->pcmCaptureDev[id] = pcmCaptureDev;
        }
    else
        {
        return -EINVAL;
        }

    return 0;
    }

//abox_register_rdma
int32_t abox_register_rdma
    (
    VXB_DEV_ID      ctrDev,
    VXB_DEV_ID      rdmaDev,
    uint32_t        id
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    if (id < pCtrData->numOfRdma)
        {
        pCtrData->rdmaDev[id] = rdmaDev;
        }
    else
        {
        return -EINVAL;
        }

    return 0;
    }

//abox_register_wdma
int32_t abox_register_wdma
    (
    VXB_DEV_ID      ctrDev,
    VXB_DEV_ID      wdmaDev,
    uint32_t        id
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    if (id < pCtrData->numOfWdma)
        {
        pCtrData->wdmaDev[id] = wdmaDev;
        }
    else
        {
        return -EINVAL;
        }

    return 0;
    }

//abox_register_uaif
STATUS abox_register_uaif
    (
    VXB_DEV_ID      ctrDev,
    VXB_DEV_ID      uaifDev,
    uint32_t        id
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return ERROR;
        }

    if (id < pCtrData->numOfUaif)
        {
        pCtrData->uaifDev[id] = uaifDev;
        }
    else
        {
        return ERROR;
        }

    return OK;
    }

//abox_enable
int32_t abox_enable
    (
    VXB_DEV_ID      ctrDev
    )
    {
    ABOX_IPC_MSG    msg = {0};
    ABOX_CTR_DATA * pCtrData;
    uint32_t        status;
    int32_t         bootdone_retry_cnt = MAX_BOOT_RETRY_COUNT;
    int             ret = 0;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    while (pCtrData->adspState != ADSP_ENABLED && bootdone_retry_cnt--)
        {
        if (aboxQueryDspStatus (ctrDev, pCtrData) != OK)
            {
            DBG_MSG (DBG_WARN,
                     "HiFi core is not ready, Invalid state: %d retry_cnt:%d\n",
                     pCtrData->adspState, bootdone_retry_cnt);
            }
        }

    if (bootdone_retry_cnt <= 0)
        {
        return -EBUSY;
        }

    pCtrData->enabled = TRUE;

    SPIN_LOCK_ISR_TAKE (&fwDlSpinlockIsr);

    status = getEarlyAudState (pCtrData);
    if (status)
        {
        msg.ipcid = IPC_ABOX_STOP_LOOPBACK;
        ret = abox_request_ipc (ctrDev, msg.ipcid, &msg, sizeof(msg), 1, 1);
        if (ret < 0)
            {
            SPIN_LOCK_ISR_GIVE (&fwDlSpinlockIsr);
            return ret;
            }
        }

    SPIN_LOCK_ISR_GIVE (&fwDlSpinlockIsr);

    aboxPadRetention (pCtrData, FALSE);
    aboxSftPadRetention (pCtrData, FALSE);

    return ret;
    }

//abox_disable
int32_t abox_disable
    (
    VXB_DEV_ID      ctrDev
    )
    {
    ABOX_CTR_DATA * pCtrData;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return -ENODATA;
        }

    aboxPadRetention (pCtrData, TRUE);
    pCtrData->enabled = FALSE;

    return 0;
    }

//abox_request_dram_on
void abox_request_dram_on
    (
    VXB_DEV_ID          ctrDev,
    void *              id,
    BOOL                on
    )
    {
    ABOX_CTR_DATA *     pCtrData;
    ABOX_DRAM_REQUEST * pRequest;
    uint32_t            dramNum;
    uint32_t            i;
    uint32_t            regVal = 0x0;

    pCtrData = (ABOX_CTR_DATA *) vxbDevSoftcGet (ctrDev);
    if (pCtrData == NULL)
        {
        return;
        }

    dramNum = NELEMENTS (pCtrData->dramRequests);

    for (i = 0; i < dramNum; i++)
        {
        pRequest = &pCtrData->dramRequests[i];
        if (pRequest->id == NULL
            || pRequest->id == id)
            {
            break;
            }
        }

    pRequest->on = on;
    VX_MEM_BARRIER_W (); /* on is read after id in reading function */
    pRequest->id = id;

    aboxCtrRegmapRead (pCtrData, pCtrData->sfrBase,
                       ABOX_SYSPOWER_CTRL, &regVal);

    if (pRequest->on)
        {
        regVal |= ABOX_SYSPOWER_CTRL_MASK;
        }
    else
        {
        regVal &= ~ABOX_SYSPOWER_CTRL_MASK;
        }

    aboxCtrRegmapWrite (pCtrData, pCtrData->sfrBase,
                        ABOX_SYSPOWER_CTRL, regVal);
    }

//abox_disable_qchannel
STATUS abox_disable_qchannel
    (
    ABOX_CTR_DATA *     pCtrData,
    Q_CHANNEL           clk,
    int32_t             disable
    )
    {
    uint32_t            regVal = 0;

    if (clk >= Q_CHANNEL_MAX)
        {
        return ERROR;
        }

    disable = (disable) ? 0x1 : 0x0;

    aboxCtrRegmapRead (pCtrData, pCtrData->sfrBase,
                       ABOX_QCHANNEL_DISABLE, &regVal);

    if ((regVal & (0x1 << clk)) == (disable << clk))
        {
        return OK;
        }

    aboxCtrRegmapWrite (pCtrData, pCtrData->sfrBase,
                        ABOX_QCHANNEL_DISABLE, regVal);

    return OK;
    }

//abox_check_bclk_from_cmu_clock
STATUS abox_check_bclk_from_cmu_clock
    (
    ABOX_CTR_DATA * pCtrData,
    uint32_t        rate,
    uint32_t        channels,
    uint32_t        width
    )
    {
    if (AUD_PLL_RATE_HZ_FOR_48000
        != vxbClkRateGet (pCtrData->aboxClks[CLK_PLL_OUT_AUD]))
        {
        DBG_MSG (DBG_ERR, "Check default PLL_AUD: %lu\n",
                 vxbClkRateGet (pCtrData->aboxClks[CLK_PLL_OUT_AUD]));
        return ERROR;
        }

    if (AUDIF_RATE_HZ
        != vxbClkRateGet (pCtrData->aboxClks[CLK_DIV_AUD_AUDIF]))
        {
        DBG_MSG (DBG_ERR, "Check default AUDIF_CLK: %lu\n",
                 vxbClkRateGet (pCtrData->aboxClks[CLK_DIV_AUD_AUDIF]));
        return ERROR;
        }

    return OK;
    }

//abox_hw_params_fixup_helper
STATUS abox_hw_params_fixup_helper
    (
    VX_SND_SOC_PCM_RUNTIME *    rtd,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    #if 0
    ABOX_CTR_DATA *                 pCtrData;
    VXB_DEV_ID                      feDev;
    struct snd_interval *           prate;
    struct snd_interval *           pchannels;
    struct snd_mask *               pfmt;
    ABOX_PCM_CTRL_DATA *     pcmData;
    struct abox_dma_data *          dmaData;
    uint32_t                          rate = 0;
    uint32_t                          bitWidth = 0;
    uint32_t                          channel = 0;
    snd_pcm_format_t                format = 0;

    pCtrData = ABOX_CTR_GLB_DATA;
    if (pCtrData == NULL)
        {
        return 0;
        }

    feDev = aboxFindFeDevFromRtd (rtd, pCtrData);
    if (feDev == NULL)
        {
        return 0;
        }

    pcmData = vxbDevSoftcGet (feDev);
    if (pcmData == NULL)
        {
        return 0;
        }

    abox_solution_get_hw_info_from_sw_solution(feDev, &pcmData->solution, &rate, &bitWidth, &channel);

    dmaData = pcmData->dmaData;

    /* First solution is list head */
    if (dmaData && dmaData->solution.next)
        {
        abox_solution_get_hw_info_from_hw_solution(feDev, &dmaData->solution,
            &rate, &bitWidth, &channel);
        }
    else
        {
        DBG_MSG (DBG_INFO, "%s dma data (%d) is not ready\n", __func__,
                 dmaData ? dmaData->id : -1);
        }

    if (!rate && !bitWidth && !channel)
        {
        return 0;
        }

    prate = hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE);
    pchannels = hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS);
    pfmt = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);

    switch (bitWidth)
        {
        case 24:
            format = SNDRV_PCM_FORMAT_S24;
            break;
        case 32:
            format = SNDRV_PCM_FORMAT_S32;
            break;
        default: //16bit_width and others
            format = SNDRV_PCM_FORMAT_S16;
            break;
        }

    if (pfmt && bitWidth)
        {
        snd_mask_none(pfmt);
        params_set_format(params, format);
        }
    if (prate && rate)
        {
        prate->min = prate->max = rate;
        }

    if (pchannels && channel)
        {
        pchannels->min = pchannels->max = channel;
        }

    return 0;
    #endif
    return OK;
    }

STATUS aboxClkRateSet
    (
    VXB_CLK_ID  pClk,
    uint64_t    rate
    )
    {
    uint32_t    disCnt = 0;
    STATUS      ret;

    while (vxbClkDisable (pClk) == OK)
        {
        disCnt++;
        }

    ret = vxbClkRateSet (pClk, rate);

    while (disCnt > 0)
        {
        (void) vxbClkEnable (pClk);
        disCnt--;
        }

    return ret;
    }

