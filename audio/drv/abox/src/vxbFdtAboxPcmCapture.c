
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/* includes */

#include <vxWorks.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/util/vxbAccess.h>
#include <vxbAboxPcm.h>
#include <aboxUtil.h>
#include <vxbFdtAboxGic.h>


/* debug macro */

#undef ABOX_CAPTURE_DEBUG
#ifdef ABOX_CAPTURE_DEBUG
#include <private/kwriteLibP.h>         /* _func_kprintf */

#define DBG_OFF             0x00000000
#define DBG_WARN            0x00000001
#define DBG_ERR             0x00000002
#define DBG_INFO            0x00000004
#define DBG_ALL             0xffffffff
LOCAL uint32_t dbgMask = DBG_ALL;

#undef DBG_MSG
#define DBG_MSG(mask,...)                          \
do                                                 \
    {                                                  \
    if ((dbgMask & (mask)) || ((mask) == DBG_ALL)) \
        {                                              \
            if (_func_kprintf != NULL)                 \
            {                                          \
            (* _func_kprintf)("[%s,%d]: ",__FUNCTION__,__LINE__);\
            (* _func_kprintf)(__VA_ARGS__);            \
            }                                          \
        }                                              \
    }while (0)
#else
#define DBG_MSG(...)
#endif  /* ABOX_CAPTURE_DEBUG */

#ifdef ARMBE8
#    define SWAP64 vxbSwap64
#else
#    define SWAP64
#endif /* ARMBE8 */

#undef REG_READ_8
#define REG_READ_8(regBase, offset)                                     \
        SWAP64 (* ((volatile uint64_t *) ((UINT8 *) regBase + offset)))


/* forward declarations */

LOCAL int aboxPcmCaptureIpcRequest(ABOX_PCM_CTRL_DATA * data,
                                   ABOX_IPC_MSG *msg, int atomic, int sync);
LOCAL int aboxPcmCaptureIsEnabled(ABOX_PCM_CTRL_DATA * data);
LOCAL int aboxPcmCaptureHwParams(VX_SND_SOC_COMPONENT * component,
                                 SND_PCM_SUBSTREAM * substream,
                                 VX_SND_PCM_HW_PARAMS * params);
LOCAL int aboxPcmCaptureHwFree(VX_SND_SOC_COMPONENT * component,
                               SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmCapturePrepare(VX_SND_SOC_COMPONENT * component,
                                SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmCaptureTrigger(VX_SND_SOC_COMPONENT * component,
                                SND_PCM_SUBSTREAM * substream, int cmd);
LOCAL SND_FRAMES_T aboxPcmCapturePointer(VX_SND_SOC_COMPONENT * component,
                                           SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmCaptureOpen(VX_SND_SOC_COMPONENT * component,
                             SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmCaptureClose(VX_SND_SOC_COMPONENT * component,
                              SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmCaptureIoctl(VX_SND_SOC_COMPONENT * component,
                              SND_PCM_SUBSTREAM * substream, unsigned int cmd,
                              void *arg);
LOCAL int aboxPcmCaptureMmap(VX_SND_SOC_COMPONENT *component,
                             SND_PCM_SUBSTREAM *substream);
LOCAL void aboxPcmCaptureOpsSet(VX_SND_SOC_CMPT_DRV * drv);
LOCAL int aboxPcmCaptureConstruct(VX_SND_SOC_COMPONENT * component,
                                  VX_SND_SOC_PCM_RUNTIME * runtime);
LOCAL void aboxPcmCaptureDestruct(VX_SND_SOC_COMPONENT * component,
                                  VX_SND_PCM * pcm);
LOCAL int aboxPcmCaptureCmpntProbe(VX_SND_SOC_COMPONENT * component);
LOCAL void aboxPcmCaptureCmpntRemove(VX_SND_SOC_COMPONENT * component);
LOCAL int aboxPcmCapturePpRouteGet (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int aboxPcmCapturePpRoutePut (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int aboxPcmCaptureValidation(VXB_DEV_ID pdev);
LOCAL STATUS aboxPcmCaptureControlMake(VXB_DEV_ID pdev, ABOX_PCM_CTRL_DATA * data,
                                    unsigned int wdmaNum, VX_SND_SOC_CMPT_DRV * cmpnt);
LOCAL STATUS aboxPcmCaptureComponentRegister(VXB_DEV_ID pdev, ABOX_PCM_CTRL_DATA * data);
LOCAL STATUS aboxPcmCaptureProbe(VXB_DEV_ID pDev);
LOCAL int aboxPcmCaptureAttach(VXB_DEV_ID pdev);
LOCAL int aboxPcmCaptureWdmaRouteGet (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int aboxPcmCaptureWdmaRoutePut (VX_SND_CONTROL * kcontrol,
                                    VX_SND_CTRL_DATA_VAL * ucontrol);

/* locals */


LOCAL VX_SND_SOC_DAI_DRIVER aboxPcmCaptureDaiDrv[] =
    {
        {
        .name = "PCM%dc",
        .capture =
            {
            .streamName = "PCM%d Capture",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        },
    };

LOCAL VX_SND_SOC_CMPT_DRV aboxPcmCaptureBase =
    {
    .probe = aboxPcmCaptureCmpntProbe,
    .remove = aboxPcmCaptureCmpntRemove,
    .pcmConstruct	= aboxPcmCaptureConstruct,
    .pcm_destruct	= aboxPcmCaptureDestruct,
    .probe_order	= SND_SOC_COMP_ORDER_FIRST,
    };

LOCAL const char * const pcmCaptureRouteTexts[] =
    {
    "Direct",
    };

LOCAL char pcmWdmaRouteTexts[][ENUM_CTRL_STR_SIZE] =
    {
    "PCM_IN",
    "NO_CONNECT", "NO_CONNECT", /* WDMA0, WDMA1 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA2, WDMA3 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA4, WDMA5 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA6, WDMA7 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA8, WDMA9 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA10, WDMA11 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA12, WDMA13 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA14, WDMA15 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA16, WDMA17 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA18, WDMA19 */
    "NO_CONNECT", "NO_CONNECT", /* WDMA20, WDMA21 */
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxPcmCaptureMatch[] =
    {
        {
        "samsung,abox-pcm-capture",
        NULL
        },
        {}                              /* empty terminated list */
    };

LOCAL VXB_DRV_METHOD aboxPcmCaptureMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxPcmCaptureProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxPcmCaptureAttach},
    {0, NULL}
    };

/* globals */

VXB_DRV vxbFdtAboxPcmCaptureDrv =
    {
    {NULL},
    "aboxPcmCapture",                           /* Name */
    "Abox Pcm Capture Fdt driver",              /* Description */
    VXB_BUSID_FDT,                              /* Class */
    0,                                          /* Flags */
    0,                                          /* Reference count */
    aboxPcmCaptureMethodList,                   /* Method table */
    };

VXB_DRV_DEF(vxbFdtAboxPcmCaptureDrv);

/* functions */

LOCAL int aboxPcmCaptureIpcRequest
    (
    ABOX_PCM_CTRL_DATA * data,
    ABOX_IPC_MSG *       msg,
    int                  atomic,
    int                  sync
    )
    {
    VXB_DEV_ID pdevAbox = data->pdevAbox;

    return abox_request_ipc(pdevAbox, msg->ipcid, msg, sizeof(*msg),
                            atomic, sync);
    }

LOCAL int aboxPcmCaptureIsEnabled
    (
    ABOX_PCM_CTRL_DATA * data
    )
    {
    unsigned int wdma_enabled = 0;

    if (!data->dmaData)
        {
        DBG_MSG(DBG_ERR, "WDMA%d connected to PCM%dp is not ready\n",
                data->dmaId, data->id);
        return 0;
        }
    wdma_enabled = wdmaIsEnable(data->dmaData);

    return wdma_enabled;
    }

LOCAL void abox_pcm_capture_disable_barrier
    (
    VXB_DEV_ID           pdev,
    ABOX_PCM_CTRL_DATA * data
    )
    {
    unsigned long        delayTicks;
    unsigned long        oldTicks;
    unsigned long        newTicks;
    unsigned long        elapsedTicks = 0;

    delayTicks = (sysTimestampFreq() * (unsigned long)ABOX_DMA_TIMEOUT_US + 99999) /
                 1000000;

    oldTicks = sysTimestamp();
    while (aboxPcmCaptureIsEnabled(data))
        {
        if (elapsedTicks <= delayTicks)
            {
            taskDelay(NO_WAIT);
            newTicks = sysTimestamp();
            if (newTicks >= oldTicks)
                {
                elapsedTicks += newTicks - oldTicks;
                }
            else
                {
                elapsedTicks += sysTimestampPeriod() - oldTicks + newTicks + 1;
                }
            oldTicks = newTicks;
            continue;
            }
        break;
        }
    }

LOCAL int aboxPcmCaptureHwParams
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream,
    VX_SND_PCM_HW_PARAMS * params
    )
    {
    VXB_DEV_ID pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int id = data->id;
    int ret;
    long long period_time = 0;
    ABOX_IPC_MSG msg = {0, };
    IPC_PCMTASK_MSG * pcmtask_msg = &msg.msg.pcmtask;
    unsigned int reduce_enabled = 0;

    ret = snd_pcm_lib_malloc_pages(substream,
                               INTER_VAL(params, VX_SND_HW_PARAM_BUFFER_BYTES));
    if (ret < 0)
        {
        return ret;
        }

    msg.ipcid = IPC_PCMCAPTURE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    pcmtask_msg->msgtype = PCM_SET_BUFFER;

    pcmtask_msg->param.setbuff.phyaddr = (int32_t)(substream->dmaBuf.phyAddr);
    pcmtask_msg->param.setbuff.size = INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES);
    pcmtask_msg->param.setbuff.count = INTER_VAL(params, VX_SND_HW_PARAM_PERIODS);

    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        return ret;

    pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
    pcmtask_msg->param.hw_params.sample_rate = PARAMS_RATE(params);// INTER_VAL(params, VX_SND_HW_PARAM_RATE);
    pcmtask_msg->param.hw_params.bit_depth = PARAMS_WIDTH(params);
    pcmtask_msg->param.hw_params.channels = INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS);

    /* When reduce is enabled, increase DMA channel (1->2) to activate REDUCE feature in WDMA */

    if (data->dmaData)
        {
        reduce_enabled = wdmaReduceEnable(data->dmaData);
        if (reduce_enabled)
            ++pcmtask_msg->param.hw_params.channels;
        }
    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        {
        return ret;
        }

    period_time = GET_PERIOD_TIME(PARAMS_WIDTH(params),
                                  INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS),
                                  PARAMS_RATE(params), //INTER_VAL(params, VX_SND_HW_PARAM_RATE),
                                  INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES));

    if (data->aboxData->perfData.checked == FALSE)
        {
        int first_buf_size = INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES) - 
                                  (DEFAULT_SBANK_SIZE + UAIF_INTERNAL_BUF_SIZE);
        signed long long pt =
            GET_PERIOD_TIME(PARAMS_WIDTH(params),
                            INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS),
                            PARAMS_RATE(params), //INTER_VAL(params, VX_SND_HW_PARAM_RATE),
                            first_buf_size);

        if (pt < 0)
            pt = 0;

        data->aboxData->perfData.period_t = (unsigned long long)pt;
        }
    return 0;
    }

LOCAL int aboxPcmCaptureHwFree
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;
    int                  ret = 0;

    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        return ERROR;
        }

    msg.ipcid = IPC_PCMCAPTURE;
    pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        {
        return ret;
        }

    abox_pcm_capture_disable_barrier(pdev, data);

    return snd_pcm_lib_free_pages(substream);
    }

LOCAL int aboxPcmCapturePrepare
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    int                  ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;

    data->pointer = substream->dmaBuf.phyAddr;

    msg.ipcid = IPC_PCMCAPTURE;
    pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 0);

    return ret;
    }

LOCAL int aboxPcmCaptureTrigger
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream,
    int                    cmd
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;
    int                  ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;

    msg.ipcid = IPC_PCMCAPTURE;
    pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    pcmtask_msg->param.dma_trigger.is_real_dma = !data->pp_enabled;

    switch (cmd)
        {
        case SNDRV_PCM_TRIGGER_START:
        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
            abox_request_dram_on(data->pdevAbox, pdev, TRUE);
            pcmtask_msg->param.dma_trigger.trigger = 1;
            pcmtask_msg->start_threshold = runtime->startThreshold;
            ret = aboxPcmCaptureIpcRequest(data, &msg, 1, 0);
            break;
        case SNDRV_PCM_TRIGGER_STOP:
        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
            if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
                {
                return ERROR;
                }

            pcmtask_msg->param.dma_trigger.trigger = 0;
            ret = aboxPcmCaptureIpcRequest(data, &msg, 1, 0);
            abox_request_dram_on(data->pdevAbox, pdev, FALSE);
            break;
        default:
            ret = ERROR;
            break;
        }

    return ret;
    }

LOCAL SND_FRAMES_T aboxPcmCapturePointer
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;
    ssize_t              pointer = 0;
    unsigned int         buf_status = 0;
    SND_FRAMES_S_T       frames = 0;

    if (!data->dmaData)
        {
        return 0;
        }
    if (data->pp_enabled)
        {
        pointer = REG_READ_8(data->pp_pointer_base, 0);
        }
    else
        {
        buf_status = aboxWdmaBufferStatuGet(data->dmaData);
        if (data->pointer >= substream->dmaBuf.phyAddr)
            {
            pointer = data->pointer - substream->dmaBuf.phyAddr;
            }
        else if (buf_status)
            {
            ssize_t offset, count;
            ssize_t buffer_bytes, period_bytes;
            uint32_t wdmaStuMsk = ~(ABOX_WDMA_WBUF_STATUS_MASK << 28);

            buffer_bytes = sndPcmlibBufferBytes(substream);
            period_bytes = sndPcmlibPeriodBytes(substream);

            offset = (((buf_status & wdmaStuMsk) >>
                      ABOX_WDMA_WBUF_OFFSET_L) << 4);
            count = (buf_status & ABOX_WDMA_WBUF_CNT_MASK);

            while ((offset % period_bytes) && (buffer_bytes >= 0))
                {
                buffer_bytes -= period_bytes;
                if ((buffer_bytes & offset) == offset)
                    offset = buffer_bytes;
                }
            pointer = offset + count;
            }
        else
            {
            pointer = 0;
            }
    }

    frames = bytesToFrames(runtime, pointer);
    DBG_MSG(DBG_INFO, "PCM[%d]: pointer:%zd frames:%ld\n", data->id, pointer, frames);

    return frames;
    }

LOCAL int aboxPcmCaptureOpen
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    VXB_DEV_ID           pdevAbox = vxbDevParent(pdev);
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    int                  ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;
    VX_SND_PCM_SUPPORT_HW_PARAMS * pSupHwParams;

    ret = abox_enable(pdevAbox);
    if (ret < 0)
        {
        return ret;
        }

    substream->runtime->hw = data->aboxDmaHardware;
    pSupHwParams = &substream->runtime->supportHwParams;
    ret = sndIntervalSetinteger(&pSupHwParams->intervals[VX_SND_HW_PARAM_PERIODS]);
    if (ret < 0)
        {
        return ret;
        }

    data->substream = substream;

    msg.ipcid = IPC_PCMCAPTURE;
    pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
    pcmtask_msg->hw_dma_id = data->dmaId;
    pcmtask_msg->irq_id = data->irqId;
    pcmtask_msg->pcm_alsa_id = data->pcm_dev_num;
    pcmtask_msg->domain_id = data->aboxData->myDomainId;
    msg.task_id = pcmtask_msg->pcm_device_id = id;

    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 0);
    return ret;
    }

LOCAL int aboxPcmCaptureClose
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    VXB_DEV_ID           pdevAbox = vxbDevParent(pdev);
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    int                  ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;

    data->substream = NULL;
    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        return ERROR;
        }

    msg.ipcid = IPC_PCMCAPTURE;
    pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    ret = aboxPcmCaptureIpcRequest(data, &msg, 0, 1);

    abox_disable(pdevAbox);

    return ret;
    }

LOCAL int aboxPcmCaptureIoctl
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream,
    unsigned int           cmd,
    void *                 arg
    )
    {
    return sndPcmDefaultIoctl(substream, cmd, arg);
    }

LOCAL int aboxPcmCaptureMmap
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
#ifdef ABOX_CAPTURE_DEBUG
    VXB_DEV_ID pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int id = data->id;

    DBG_MSG(DBG_INFO, "PCM%d Capture : DMA %d\n", id, data->dmaId);
#endif
    return 0;
    }

LOCAL void aboxPcmCaptureOpsSet
    (
    VX_SND_SOC_CMPT_DRV * drv
    )
    {
    drv->open		= aboxPcmCaptureOpen;
    drv->close		= aboxPcmCaptureClose;
    drv->ioctl		= aboxPcmCaptureIoctl;
    drv->hwParams	= aboxPcmCaptureHwParams;
    drv->hwFree	= aboxPcmCaptureHwFree;
    drv->prepare	= aboxPcmCapturePrepare;
    drv->trigger	= aboxPcmCaptureTrigger;
    drv->pointer	= aboxPcmCapturePointer;
    drv->mmap		= aboxPcmCaptureMmap;
    }

LOCAL int aboxPcmCaptureConstruct
    (
    VX_SND_SOC_COMPONENT *   component,
    VX_SND_SOC_PCM_RUNTIME * runtime
    )
    {
    VX_SND_PCM *         pcm = runtime->pcm;
    SND_PCM_STREAM *     stream = &pcm->stream[SNDRV_PCM_STREAM_CAPTURE];
    SND_PCM_SUBSTREAM *  substream = stream->substream;
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  ret = 0;

    ret = dmaMemAlloc(pdev, data->aboxData, substream);
    if (ret == ERROR)
        {
        DBG_MSG(DBG_ERR, "Can't get reserved memory.\n");
        }
    data->pcm_dev_num = runtime->num;
    DBG_MSG(DBG_INFO, "[%d] dma_buffer.addr=%llx dma_buffer.bytes=%zd buffer_bytes: %zd\n",
            data->id, substream->dmaBuf.phyAddr, substream->dmaBuf.bytes,
            data->aboxDmaHardware.bufferBytesMax);

    return ret;
    }

LOCAL void aboxPcmCaptureDestruct
    (
    VX_SND_SOC_COMPONENT * component,
    VX_SND_PCM *           pcm
    )
    {
    SND_PCM_STREAM *           stream = &pcm->stream[SNDRV_PCM_STREAM_CAPTURE];
    SND_PCM_SUBSTREAM *        substream = stream->substream;
    VX_SND_SUBSTREAM_DMA_BUF * dmaBuf = &substream->dmaBuf;

    dmaBuf->private_data = NULL;
    dmaBuf->area = NULL;
    dmaBuf->phyAddr = 0;
    dmaBuf->bytes = 0;
    }

LOCAL int aboxPcmCaptureCmpntProbe
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);

    data->cmpnt = component;

    return 0;
    }

LOCAL void aboxPcmCaptureCmpntRemove
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    }

LOCAL int aboxPcmCapturePpRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_PCM_CTRL_DATA *   data;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        pDev = cmpt->pDev;
        }

    data = (ABOX_PCM_CTRL_DATA *) vxbDevSoftcGet(pDev);
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find ABOX_PCM_CTRL_DATA from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    ucontrol->value.enumerated.item[0] = data->pp_enabled;
    return OK;
    }

LOCAL int aboxPcmCapturePpRoutePut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_PCM_CTRL_DATA *   data;
    unsigned int           item = ucontrol->value.enumerated.item[0];

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        pDev = cmpt->pDev;
        }

    data = (ABOX_PCM_CTRL_DATA *) vxbDevSoftcGet(pDev);
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find ABOX_PCM_CTRL_DATA from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    if (item != 0)
        {
        DBG_MSG(DBG_ERR, "The value of item should be zero!\n");
        return ERROR;
        }
    data->pp_enabled = FALSE;

    return OK;
    }

LOCAL int aboxPcmCaptureValidation
    (
    VXB_DEV_ID pdev
    )
    {
    int              offset = 0;
    int              len;
    int              propLen;
    char *           pName;
    uint32_t *       prop;
    uint32_t         wdmaType;
    uint32_t         wdmaId;
    uint32_t         dma_to_non_sft_uaif = 0;
    VXB_FDT_DEV *    pFdtDevParent = NULL;
    VXB_DEV_ID       pdevAbox = vxbDevParent(pdev);

    pFdtDevParent = vxbFdtDevGet (pdevAbox);
    offset = pFdtDevParent->offset;

    for (offset = vxFdtFirstSubnode (offset);
         offset >= 0;
         offset  = vxFdtNextSubnode (offset))
        {
        pName =(char *)vxFdtGetName(offset, &len);
        if (pName == NULL)
            continue;
        if(strncmp(pName, "abox_wdma", strlen("abox_wdma")) == 0)
            {
            prop = (uint32_t *)vxFdtPropGet(offset, "samsung,type", &propLen);
            if (prop == NULL)
                continue;
            wdmaType = vxFdt32ToCpu(*prop);
            if ((wdmaType == DMA_TO_NON_SFT_UAIF) || (wdmaType == DMA_TO_DMA))
                {
                prop = (uint32_t *)vxFdtPropGet(offset, "samsung,id", &propLen);
                wdmaId = vxFdt32ToCpu(*prop);
                if (vxFdtIsEnabled(offset))
                    {
                    snprintf(pcmWdmaRouteTexts[wdmaId + 1], ENUM_CTRL_STR_SIZE - 1,
                             "WDMA%d", wdmaId);
                    }
                else
                    {
                    snprintf(pcmWdmaRouteTexts[wdmaId + 1], ENUM_CTRL_STR_SIZE - 1,
                             "NO_CONNECT", wdmaId);
                    }
                dma_to_non_sft_uaif++;
                }
            }
        }
    return dma_to_non_sft_uaif;
    }

LOCAL int aboxPcmCaptureWdmaRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_PCM_CTRL_DATA *   data;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *)(kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        pDev = cmpt->pDev;
        }

    data = (ABOX_PCM_CTRL_DATA *) vxbDevSoftcGet (pDev);
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find ABOX_PCM_CTRL_DATA from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    ucontrol->value.enumerated.item[0] = data->dmaId + 1;

    return OK;
    }


LOCAL int aboxPcmCaptureWdmaRoutePut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_PCM_CTRL_DATA *   data;
    char                   frontName[SND_DEV_NAME_LEN];
    char                   backName[SND_DEV_NAME_LEN];
    ABOX_DMA_CTRL        * pDevCtrl = NULL;

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *)(kcontrol->privateData))->pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        pDev = cmpt->pDev;
        }

    data = (ABOX_PCM_CTRL_DATA *) vxbDevSoftcGet (pDev);
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find ABOX_PCM_CTRL_DATA from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    data->dmaId = ucontrol->value.enumerated.item[0] - 1;

    if (data->dmaId < 0)
        {
        data->dmaData = NULL;
        }
    else
        {
        data->pdevDma = data->aboxData->wdmaDev[data->dmaId];
        if (data->pdevDma == NULL)
            {
            DBG_MSG (DBG_ERR, "%s: data->aboxData->wdmaDev[%d] is NULL",
                     kcontrol->id.name, data->dmaId);
            return ERROR;
            }
        pDevCtrl = vxbDevSoftcGet(data->pdevDma);
        data->dmaData = pDevCtrl->pDmaData;
        }

    /* set backend */

    (void) snprintf_s (frontName, SND_DEV_NAME_LEN, "PCM%dc",
                       data->daiDrv[0].id);
    if (data->dmaId < 0)
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "NULL");
        }
    else
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "WDMA%d", data->dmaId);
        }

    if (sndSocBeConnectByName (cmpt->card, (const char *) frontName,
                               (const char *) backName,
                               SNDRV_PCM_STREAM_CAPTURE) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to connect dynamic PCM\n");
        return ERROR;
        }

    return OK;
    }

LOCAL STATUS aboxPcmCaptureControlMake
    (
    VXB_DEV_ID            pdev,
    ABOX_PCM_CTRL_DATA *  data,
    unsigned int          wdmaNum,
    VX_SND_SOC_CMPT_DRV * cmpnt
    )
    {
    int                idx;
    char *             name            = NULL;
    VX_SND_ENUM *      wdmaRouteEnum   = NULL;
    VX_SND_CONTROL *   captureControls = NULL;
    size_t             size;
    static const char * const enumWdmaStr[]   =
        {
        pcmWdmaRouteTexts[0], pcmWdmaRouteTexts[1],
        pcmWdmaRouteTexts[2], pcmWdmaRouteTexts[3],
        pcmWdmaRouteTexts[4], pcmWdmaRouteTexts[5],
        pcmWdmaRouteTexts[6], pcmWdmaRouteTexts[7],
        pcmWdmaRouteTexts[8], pcmWdmaRouteTexts[9],
        pcmWdmaRouteTexts[10], pcmWdmaRouteTexts[11],
        pcmWdmaRouteTexts[12], pcmWdmaRouteTexts[13],
        pcmWdmaRouteTexts[14], pcmWdmaRouteTexts[15],
        pcmWdmaRouteTexts[16], pcmWdmaRouteTexts[17],
        pcmWdmaRouteTexts[18], pcmWdmaRouteTexts[19],
        pcmWdmaRouteTexts[20], pcmWdmaRouteTexts[21],
        pcmWdmaRouteTexts[22]
        };

    VX_SND_ENUM wdmaRouteEnumBase = SOC_ENUM_DOUBLE (SND_SOC_NOPM, 0, 0,
                                                     wdmaNum + 1, enumWdmaStr);

    static VX_SND_ENUM pcmCaptureRouteEnum = SOC_ENUM_DOUBLE(SND_SOC_NOPM, 0, 0,
						NELEMENTS (pcmCaptureRouteTexts), pcmCaptureRouteTexts);
    /*
     * amixer cset name='ABOX PCM0c PP Route' Direct / PostProcessing
     * amixer cset name='ABOX PCM0c WDMA Route' PCM_IN / WDMA0 / WDMA1 /...
     */

    VX_SND_CONTROL capCtrlTemplate[2] =
        {
        SOC_ENUM_EXT ("PCM%dc PP Route", pcmCaptureRouteEnum,
                      aboxPcmCapturePpRouteGet, aboxPcmCapturePpRoutePut),
        SOC_ENUM_EXT ("PCM%dc WDMA Route", wdmaRouteEnumBase,
                      aboxPcmCaptureWdmaRouteGet, aboxPcmCaptureWdmaRoutePut)
        };

    /* allocate memory for two controls*/

    size = sizeof (VX_SND_CONTROL) * ELEMENTS (capCtrlTemplate);
    captureControls = (VX_SND_CONTROL *) vxbMemAlloc (size);
    if (captureControls == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to alloc memory for captureControls\n");
        goto errOut;
        }

    (void) memcpy_s (captureControls, size, capCtrlTemplate, size);

    /* allocate memory for enum */

    wdmaRouteEnum = (VX_SND_ENUM *) vxbMemAlloc (sizeof (VX_SND_ENUM));
    if (wdmaRouteEnum == NULL)
        {
        DBG_MSG(DBG_ERR, "failed to alloc memory for wdmaRouteEnum\n");
        goto errOut;
        }

    (void) memcpy_s (wdmaRouteEnum, sizeof (VX_SND_ENUM),
                     &wdmaRouteEnumBase, sizeof (VX_SND_ENUM));

    captureControls[1].privateValue = (unsigned long) wdmaRouteEnum;

    /* allocate memroy for control name */

    for (idx = 0; idx < ELEMENTS (capCtrlTemplate); idx++)
        {
        name = vxbMemAlloc (SND_DEV_NAME_LEN);
        if (name == NULL)
            {
            DBG_MSG (DBG_ERR, "failed to alloc name memory\n");
            goto errOut;
            }
        (void) snprintf_s (name, SND_DEV_NAME_LEN, captureControls[idx].id.name,
                           data->daiDrv[0].id);
        captureControls[idx].id.name = name;
        }

    /* register controls via component */

    cmpnt->controls     = (const VX_SND_CONTROL *) captureControls;
    cmpnt->num_controls = ELEMENTS (capCtrlTemplate);

    return OK;

errOut:
    if (captureControls != NULL)
        {
        for (idx = 0; idx < ELEMENTS (capCtrlTemplate); idx++)
            {
            if (captureControls[idx].id.name != NULL)
                {
                vxbMemFree (captureControls[idx].id.name);
                }
            }
        vxbMemFree (captureControls);
        }
    if (wdmaRouteEnum != NULL)
        {
        vxbMemFree (wdmaRouteEnum);
        }

    return ERROR;
    }

LOCAL STATUS aboxPcmCaptureComponentRegister
    (
    VXB_DEV_ID           pdev,
    ABOX_PCM_CTRL_DATA * data
    )
    {
    int                   num_of_wdma = 0;
    VX_SND_SOC_CMPT_DRV * cmpntDrv;
    int                   i = 0;
    char *                daiName = NULL;
    char *                streamName = NULL;

    data->daiDrv = vxbMemAlloc(sizeof(VX_SND_SOC_DAI_DRIVER));
    if (!data->daiDrv)
        {
        DBG_MSG(DBG_ERR, "can't allocate memory for dai drv.\n");
        return ERROR;
        }

    memcpy(data->daiDrv, aboxPcmCaptureDaiDrv, sizeof(aboxPcmCaptureDaiDrv));
    for (i = 0; i < ELEMENTS (aboxPcmCaptureDaiDrv); i++)
        {
        data->daiDrv[i].id = data->id;
        daiName = vxbMemAlloc(strlen(data->daiDrv[i].name) + 1);
        snprintf_s(daiName, strlen(data->daiDrv[i].name) + 1,
                   data->daiDrv[i].name, data->id);
        data->daiDrv[i].name = daiName;

        streamName = vxbMemAlloc(strlen(data->daiDrv[i].capture.streamName) + 1);
        snprintf_s(streamName, strlen(data->daiDrv[i].capture.streamName) + 1,
                   data->daiDrv[i].capture.streamName, data->id);
        data->daiDrv[i].capture.streamName = streamName;
        }

    cmpntDrv = vxbMemAlloc(sizeof(aboxPcmCaptureBase));
    if (!cmpntDrv)
        {
        DBG_MSG(DBG_ERR, "can't allocate memory for cmpntDrv.\n");
        return ERROR;
        }
    memcpy(cmpntDrv, &aboxPcmCaptureBase, sizeof(aboxPcmCaptureBase));

    num_of_wdma = aboxPcmCaptureValidation(pdev);
    if (num_of_wdma < 0)
        {
        DBG_MSG(DBG_ERR, "property reading fail.\n");
        return ERROR;
        }
    if (aboxPcmCaptureControlMake(pdev, data, num_of_wdma, cmpntDrv) != OK)
        {
        DBG_MSG(DBG_ERR, "failed to make capture controls\n");
        return ERROR;
        }

    aboxPcmCaptureOpsSet(cmpntDrv);

    if (vxSndCpntRegister (pdev, cmpntDrv, data->daiDrv,
                           ELEMENTS (aboxPcmCaptureDaiDrv)) != OK)
        {
        return ERROR;
        }

    return OK;
    }

STATUS aboxPcmCaptureIrqHandler
    (
    uint32_t   pcmId,
    VXB_DEV_ID pdev
    )
    {
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);

    data->pointer = 0;
    snd_pcm_period_elapsed(data->substream);
    return OK;
    }

LOCAL STATUS aboxPcmCaptureAttach
    (
    VXB_DEV_ID pdev
    )
    {
    VXB_FDT_DEV *        pFdtDev = NULL;
    uint32_t *           prop = NULL;
    int                  len;
    VXB_RESOURCE *       pMemRegs = NULL;
    VXB_RESOURCE_ADR *   pRegsAddr = NULL;
    ABOX_PCM_CTRL_DATA * data;
    STATUS               ret = ERROR;

    pFdtDev = vxbFdtDevGet (pdev);
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "no FDT device.\n");
        return ERROR;
        }

    data = vxbMemAlloc(sizeof(ABOX_PCM_CTRL_DATA));
    if (data == NULL)
        {
        DBG_MSG(DBG_ERR,"can't allocate memory for data.\n");
        return ERROR;
        }

    vxbDevSoftcSet(pdev, data);
    data->pdev = pdev;
    data->dmaId = ABOX_WDMA_NO_CONNECT;
    data->streamType = SNDRV_PCM_STREAM_CAPTURE;

    data->pdevAbox = vxbDevParent(pdev);
    data->aboxData = vxbDevSoftcGet(data->pdevAbox);

    // Reads the PCM Capture id from the device tree and stores in data->id

    prop = (uint32_t *)vxFdtPropGet(pFdtDev->offset, "samsung,id", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        DBG_MSG (DBG_ERR, "failed to get samsung id.\n");
        return ERROR;
        }
    data->id = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "id = %d\n", data->id);

    prop = (uint32_t *)vxFdtPropGet(pFdtDev->offset, "samsung,irq_id", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        DBG_MSG (DBG_ERR, "failed to get samsung id.\n");
        return ERROR;
        }
    data->irqId = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "irq_id = %d\n", data->irqId);

    pMemRegs = vxbResourceAlloc(pdev, VXB_RES_MEMORY, 0);
    if (pMemRegs == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        return ERROR;
        }
    pRegsAddr = (VXB_RESOURCE_ADR *)pMemRegs->pRes;
    data->pp_pointer_phys = data->aboxData->adspData[HIFI_CORE1].physAddr +
                            pRegsAddr->start;
    data->pp_pointer_size = pRegsAddr->size;
    data->pp_pointer_base = (VIRT_ADDR)((char *)data->aboxData->adspData[HIFI_CORE1].virtAddr +
                            pRegsAddr->start);
    if (data->pp_pointer_base)
        DBG_MSG(DBG_INFO, "pp_pointer:%p pp_pointer_phys:%llx\n",
                data->pp_pointer_base, data->pp_pointer_phys);

    // Reads the PCM Capture ALSA Max Buffer size from the device tree
    
    hwParamsSet(pFdtDev->offset, &data->aboxDmaHardware);

    DBG_MSG(DBG_INFO, "id:%d irq_id:%d buffer_bytes_max : %lu\n",
            data->id, data->irqId, data->aboxDmaHardware.bufferBytesMax);

    ret = abox_register_pcm_capture(data->aboxData->pDev, pdev, data->id);
    if (ret == ERROR)
        {
        return ret;
        }

    ret = abox_gic_set_pcm_irq_register(data->irqId,
                                        aboxPcmCaptureIrqHandler, pdev);
    if (ret == ERROR)
        {
        return ret;
        }

    ret = aboxPcmCaptureComponentRegister(pdev, data);
    if (ret == ERROR)
        {
        return ret;
        }

    return OK;
    }

LOCAL STATUS aboxPcmCaptureProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxPcmCaptureMatch, NULL);
    }

