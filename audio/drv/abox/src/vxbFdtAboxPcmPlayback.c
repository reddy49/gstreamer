
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

#undef ABOX_PLAYBACK_DEBUG
#ifdef ABOX_PLAYBACK_DEBUG
#include <private/kwriteLibP.h>         /* _func_kprintf */

#define DBG_OFF             0x00000000
#define DBG_WARN            0x00000001
#define DBG_ERR             0x00000002
#define DBG_INFO            0x00000004
#define DBG_ALL             0xffffffff
LOCAL uint32_t dbgMask = DBG_INFO|DBG_ERR;

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
#endif  /* ABOX_PLAYBACK_DEBUG */

#ifdef ARMBE8
#    define SWAP64 vxbSwap64
#else
#    define SWAP64
#endif /* ARMBE8 */

#undef REG_READ_8
#define REG_READ_8(regBase, offset)                                     \
                SWAP64 (* ((volatile uint64_t *) ((UINT8 *) regBase + offset)))

/* typedefs */

/* forward declarations */

IMPORT void vxbUsDelay(int delayTime);

LOCAL STATUS aboxPcmPlaybackProbe(VXB_DEV_ID pDev);
LOCAL STATUS aboxPcmPlaybackAttach(VXB_DEV_ID pdev);
LOCAL int aboxPcmPlaybackCmpntProbe(VX_SND_SOC_COMPONENT * component);
LOCAL void aboxPcmPlaybackCmpntRemove(VX_SND_SOC_COMPONENT * component);
LOCAL int aboxPcmPlaybackCmpntConstruct(VX_SND_SOC_COMPONENT * component,
                                        VX_SND_SOC_PCM_RUNTIME * runtime);
LOCAL void aboxPcmPlaybackCmpntDestruct(VX_SND_SOC_COMPONENT * component,
                                        VX_SND_PCM * pcm);
LOCAL int aboxPcmPlaybackPpRouteGet (VX_SND_CONTROL * kcontrol,
                                     VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int aboxPcmPlaybackPpRoutePut (VX_SND_CONTROL * kcontrol,
                                     VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL STATUS aboxPcmPlaybackCmpntRegister(VXB_DEV_ID pdev, ABOX_PCM_CTRL_DATA * data);
LOCAL void aboxPcmPlaybackValidation(VXB_DEV_ID  pDev);
LOCAL STATUS aboxPcmPlaybackControlMake(VXB_DEV_ID pdev, ABOX_PCM_CTRL_DATA * data,
                                     VX_SND_SOC_CMPT_DRV * cmpnt);
LOCAL int aboxPcmPlaybackRdmaRouteGet (VX_SND_CONTROL * kcontrol,
                                       VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int aboxPcmPlaybackRdmaRoutePut (VX_SND_CONTROL * kcontrol,
                                       VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL void aboxPcmPlaybackOpsSet(VX_SND_SOC_CMPT_DRV * drv);
LOCAL int aboxPcmPlaybackOpen(VX_SND_SOC_COMPONENT * component,
                              SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmPlaybackClose(VX_SND_SOC_COMPONENT *component,
                               SND_PCM_SUBSTREAM *substream);
LOCAL int aboxPcmPlaybackIpcRequest(ABOX_PCM_CTRL_DATA * data,
                                    ABOX_IPC_MSG * msg, int atomic, int sync);
LOCAL int aboxPcmPlaybackIoctl(VX_SND_SOC_COMPONENT * component,
                               SND_PCM_SUBSTREAM * substream,
                               unsigned int cmd, void * arg);
LOCAL int aboxPcmPlaybackHwParams(VX_SND_SOC_COMPONENT * component,
                                  SND_PCM_SUBSTREAM * substream,
                                  VX_SND_PCM_HW_PARAMS * params);
LOCAL void aboxPcmPlaybackBarrierDisable(VXB_DEV_ID pdev, ABOX_PCM_CTRL_DATA * data);
LOCAL int aboxPcmPlaybackIsEnabled(ABOX_PCM_CTRL_DATA * data);
LOCAL int aboxPcmPlaybackHwFree(VX_SND_SOC_COMPONENT * component,
                                SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmPlaybackPrepare(VX_SND_SOC_COMPONENT * component,
                                 SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmPlaybackTrigger(VX_SND_SOC_COMPONENT * component,
                                 SND_PCM_SUBSTREAM * substream, int cmd);
LOCAL SND_FRAMES_T aboxPcmPlaybackPointer(VX_SND_SOC_COMPONENT * component,
                                            SND_PCM_SUBSTREAM * substream);
LOCAL int aboxPcmPlaybackMmap(VX_SND_SOC_COMPONENT * component,
                              SND_PCM_SUBSTREAM * substream);
STATUS abox_pcm_playback_irq_handler(uint32_t pcm_id, VXB_DEV_ID pdev);

/* locals */

LOCAL char pcm_rdma_route_texts[][ENUM_CTRL_STR_SIZE] =
    {
    "PCM_OUT",
    "NO_CONNECT", "NO_CONNECT", /* RDMA0, RDMA1 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA2, RDMA3 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA4, RDMA5 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA6, RDMA7 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA8, RDMA9 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA10, RDMA11 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA12, RDMA13 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA14, RDMA15 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA16, RDMA17 */
    "NO_CONNECT", "NO_CONNECT", /* RDMA18, RDMA19 */
    "NO_CONNECT", "NO_CONNECT"  /* RDMA20, RDMA21 */
    };


LOCAL VX_SND_SOC_DAI_DRIVER aboxPcmPlaybackDaiDrv[] =
    {
        {
        .name = "PCM%dp",
        .playback =
            {
            .streamName = "PCM%d Playback",
            .channelsMin = 1,
            .channelsMax = 32,
            .rates = ABOX_SAMPLING_RATES,
            .rateMin = 8000,
            .rateMax = 384000,
            .formats = ABOX_SAMPLE_FORMATS,
            },
        },
    };

LOCAL const char * const pcmPlaybackRouteTexts[] =
    {
    "Direct",
    };


LOCAL VXB_DRV_METHOD aboxPcmPlayBackMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxPcmPlaybackProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxPcmPlaybackAttach},
    {0, NULL}
    };


LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxPcmPlaybackMatch[] =
    {
        {
        "samsung,abox-pcm-playback",
        NULL
        },
        {}                              /* empty terminated list */
    };


LOCAL VX_SND_SOC_CMPT_DRV aboxPcmPlaybackBase =
    {
    .probe = aboxPcmPlaybackCmpntProbe,
    .remove = aboxPcmPlaybackCmpntRemove,
    .pcmConstruct	= aboxPcmPlaybackCmpntConstruct,
    .pcm_destruct	= aboxPcmPlaybackCmpntDestruct,
    .probe_order	= SND_SOC_COMP_ORDER_FIRST,
    };


/* globals */

VXB_DRV vxbFdtAboxPcmPlaybackDrv =
    {
    {NULL},
    "aboxPcmPlayback",                          /* Name */
    "Abox Pcm Playback Fdt driver",             /* Description */
    VXB_BUSID_FDT,                              /* Class */
    0,                                          /* Flags */
    0,                                          /* Reference count */
    aboxPcmPlayBackMethodList,                  /* Method table */
    };

VXB_DRV_DEF(vxbFdtAboxPcmPlaybackDrv);


/* functions */

LOCAL int aboxPcmPlaybackCmpntProbe
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID            pdev = component->pDev;
    ABOX_PCM_CTRL_DATA  * data = vxbDevSoftcGet(pdev);

    data->cmpnt = component;

    return 0;
    }

LOCAL void aboxPcmPlaybackCmpntRemove
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    DBG_MSG(DBG_INFO, "aboxPcmPlaybackCmpntRemove\n");
    }

LOCAL int aboxPcmPlaybackCmpntConstruct
    (
    VX_SND_SOC_COMPONENT *   component,
    VX_SND_SOC_PCM_RUNTIME * runtime
    )
    {
    VX_SND_PCM * pcm = runtime->pcm;
    SND_PCM_STREAM *stream = &pcm->stream[SNDRV_PCM_STREAM_PLAYBACK];
    SND_PCM_SUBSTREAM *substream = stream->substream;
    VXB_DEV_ID pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = (ABOX_PCM_CTRL_DATA *)vxbDevSoftcGet(pdev);
    int ret = 0;

    ret = dmaMemAlloc(pdev, data->aboxData, substream);
    if (ret == ERROR)
        {
        DBG_MSG(DBG_ERR, "Can't get reserved memory (size:%zd)\n",
                data->aboxDmaHardware.bufferBytesMax);
        }
    data->pcm_dev_num = runtime->num;
    DBG_MSG(DBG_INFO, "[%d] dma_buffer.addr=%llx dma_buffer.bytes=%zd buffer_bytes: %zd\n",
        data->id, substream->dmaBuf.phyAddr, substream->dmaBuf.bytes,
        data->aboxDmaHardware.bufferBytesMax);
    return ret;
    }

LOCAL void aboxPcmPlaybackCmpntDestruct
    (
    VX_SND_SOC_COMPONENT * component,
    VX_SND_PCM *           pcm
    )
    {
    SND_PCM_STREAM *stream = &pcm->stream[SNDRV_PCM_STREAM_PLAYBACK];
    SND_PCM_SUBSTREAM *substream = stream->substream;
    VX_SND_SUBSTREAM_DMA_BUF *dmaBuf = &substream->dmaBuf;
    dmaBuf->private_data = NULL;
    dmaBuf->area = NULL;
    dmaBuf->phyAddr = 0;
    dmaBuf->bytes = 0;
    }

LOCAL int aboxPcmPlaybackPpRouteGet
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

LOCAL int aboxPcmPlaybackPpRoutePut
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

LOCAL void aboxPcmPlaybackValidation
    (
    VXB_DEV_ID  pDev
    )
    {
    int              offset;
    int              len;
    int              propLen;
    char *           pName;
    uint32_t         rdmaId;
    uint32_t *       prop;
    VXB_FDT_DEV *    pFdtDevParent = NULL;
    VXB_DEV_ID       pDevParent = vxbDevParent(pDev);
    ABOX_CTR_DATA *  aboxData = vxbDevSoftcGet(pDevParent);

    pFdtDevParent = vxbFdtDevGet (pDevParent);
    offset = pFdtDevParent->offset;
    for (offset = vxFdtFirstSubnode (offset);
         offset >= 0;
         offset  = vxFdtNextSubnode (offset))
        {
        pName = (char *)vxFdtGetName(offset, &len);
        if(pName == NULL)
            continue;
        if (strncmp(pName, "abox_rdma", strlen("abox_rdma")) == 0)
            {
            prop = (uint32_t *)vxFdtPropGet(offset, "samsung,id", &propLen);
            if (prop == NULL)
                continue;
            rdmaId = vxFdt32ToCpu(*prop);
            if (rdmaId < aboxData->numOfRdma)
                {
                if (vxFdtIsEnabled(offset))
                    {
                    snprintf_s(pcm_rdma_route_texts[rdmaId + 1],
                               ENUM_CTRL_STR_SIZE - 1, "RDMA%d", rdmaId);
                    }
                else
                    {
                    snprintf_s(pcm_rdma_route_texts[rdmaId + 1],
                               ENUM_CTRL_STR_SIZE - 1, "NO_CONNECT");
                    }
                }
            }
        }
    return;
    }


LOCAL int aboxPcmPlaybackRdmaRouteGet
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


LOCAL int aboxPcmPlaybackRdmaRoutePut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_PCM_CTRL_DATA *   data;
    VX_SND_ENUM *          pEnum = (VX_SND_ENUM *) kcontrol->privateValue;
    char                   frontName[SND_DEV_NAME_LEN];
    char                   backName[SND_DEV_NAME_LEN];
    ABOX_DMA_CTRL *        pDevCtrl = NULL;


    if (pEnum == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find VX_SND_ENUM from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

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
        data->pdevDma = data->aboxData->rdmaDev[data->dmaId];
        if (data->pdevDma == NULL)
            {
            DBG_MSG (DBG_ERR, "%s: data->aboxData->rdmaDev[%d] is NULL",
                     kcontrol->id.name, data->dmaId);
            return ERROR;
            }
        pDevCtrl = vxbDevSoftcGet(data->pdevDma);
        data->dmaData = pDevCtrl->pDmaData;
        }

    /* set backend */

    (void) snprintf_s (frontName, SND_DEV_NAME_LEN, "PCM%dp",
                       data->daiDrv[0].id);
    if (data->dmaId < 0)
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "NULL");
        }
    else
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "RDMA%d", data->dmaId);
        }

    if (sndSocBeConnectByName (cmpt->card, (const char *) frontName,
                               (const char *) backName,
                               SNDRV_PCM_STREAM_PLAYBACK) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to connect dynamic PCM\n");
        return ERROR;
        }

    return OK;
    }

LOCAL STATUS aboxPcmPlaybackControlMake
    (
    VXB_DEV_ID            pdev,
    ABOX_PCM_CTRL_DATA  * data,
    VX_SND_SOC_CMPT_DRV * cmpnt
    )
    {
    VX_SND_CONTROL *   playbackControls = NULL;
    VX_SND_ENUM *      rdmaRouteEnum    = NULL;
    size_t             size;
    int                idx;
    char *             name             = NULL;
    static const char * const enum_rdma_str[] =
        {
        pcm_rdma_route_texts[0], pcm_rdma_route_texts[1],
        pcm_rdma_route_texts[2], pcm_rdma_route_texts[3],
        pcm_rdma_route_texts[4], pcm_rdma_route_texts[5],
        pcm_rdma_route_texts[6], pcm_rdma_route_texts[7],
        pcm_rdma_route_texts[8], pcm_rdma_route_texts[9],
        pcm_rdma_route_texts[10], pcm_rdma_route_texts[11],
        pcm_rdma_route_texts[12], pcm_rdma_route_texts[13],
        pcm_rdma_route_texts[14], pcm_rdma_route_texts[15],
        pcm_rdma_route_texts[16], pcm_rdma_route_texts[17],
        pcm_rdma_route_texts[18], pcm_rdma_route_texts[19],
        pcm_rdma_route_texts[20], pcm_rdma_route_texts[21],
        pcm_rdma_route_texts[22]
        };
    const unsigned int numOfRdma = data->aboxData->numOfRdma + 1;
    VX_SND_ENUM rdma_route_enum_base = SOC_ENUM_DOUBLE (SND_SOC_NOPM, 0, 0,
                                                        numOfRdma, enum_rdma_str);
    static VX_SND_ENUM pcmPlaybackRouteEnum = SOC_ENUM_DOUBLE(SND_SOC_NOPM, 0, 0,
                        NELEMENTS (pcmPlaybackRouteTexts), pcmPlaybackRouteTexts);

    /*
     * amixer cset name='ABOX PCM0p PP Route' Direct / PostProcessing
     * amixer cset name='ABOX PCM0p WDMA Route' PCM_OUT / RDMA0 / RDMA1 /...
     */

    VX_SND_CONTROL playCtrlTemplate[2] =
    {
    SOC_ENUM_EXT ("PCM%dp PP Route", pcmPlaybackRouteEnum,
                  aboxPcmPlaybackPpRouteGet, aboxPcmPlaybackPpRoutePut),
    SOC_ENUM_EXT ("PCM%dp RDMA Route", rdma_route_enum_base,
                  aboxPcmPlaybackRdmaRouteGet, aboxPcmPlaybackRdmaRoutePut)
    };

    /* allocate memory for two controls*/

    size = sizeof (VX_SND_CONTROL) * ELEMENTS (playCtrlTemplate);
    playbackControls = (VX_SND_CONTROL *) vxbMemAlloc (size);
    if (playbackControls == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to alloc memory for playbackControls\n");
        goto errOut;
        }

    (void) memcpy_s (playbackControls, size, playCtrlTemplate, size);

    /* allocate memory for enum */

    rdmaRouteEnum = (VX_SND_ENUM *) vxbMemAlloc (sizeof (VX_SND_ENUM));
    if (rdmaRouteEnum == NULL)
        {
        DBG_MSG(DBG_ERR, "failed to alloc memory for rdmaRouteEnum\n");
        goto errOut;
        }

    (void) memcpy_s (rdmaRouteEnum, sizeof (VX_SND_ENUM),
                     &rdma_route_enum_base, sizeof (VX_SND_ENUM));

    playbackControls[1].privateValue = (unsigned long) rdmaRouteEnum;

    /* allocate memroy for control name */

    for (idx = 0; idx < ELEMENTS (playCtrlTemplate); idx++)
        {
        name = vxbMemAlloc (SND_DEV_NAME_LEN);
        if (name == NULL)
            {
            DBG_MSG (DBG_ERR, "failed to alloc name memory\n");
            goto errOut;
            }
        (void) snprintf_s (name, SND_DEV_NAME_LEN, playbackControls[idx].id.name,
                           data->daiDrv[0].id);
        playbackControls[idx].id.name = name;
        }

    /* register controls via component */

    cmpnt->controls     = (const VX_SND_CONTROL *) playbackControls;
    cmpnt->num_controls = ELEMENTS (playCtrlTemplate);

    return OK;

errOut:
    if (playbackControls != NULL)
        {
        for (idx = 0; idx < ELEMENTS (playCtrlTemplate); idx++)
            {
            if (playbackControls[idx].id.name != NULL)
                {
                vxbMemFree (playbackControls[idx].id.name);
                }
            }
        vxbMemFree (playbackControls);
        }
    if (rdmaRouteEnum != NULL)
        {
        vxbMemFree (rdmaRouteEnum);
        }

    return ERROR;
    }

LOCAL int aboxPcmPlaybackIpcRequest
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



LOCAL int aboxPcmPlaybackOpen
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    VXB_DEV_ID           pdevAbox = vxbDevParent(pdev);
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    STATUS               ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG * pcmtask_msg = &msg.msg.pcmtask;
    VX_SND_PCM_SUPPORT_HW_PARAMS * pSupHwParams;

    ret = abox_enable(pdevAbox);
    if (ret < 0)
        {
        DBG_MSG(DBG_ERR, "[%d] Can't set abox_enable : %d\n", id, ret);
        return ret;
        }

    substream->runtime->hw = data->aboxDmaHardware;
    pSupHwParams = &substream->runtime->supportHwParams;
    ret = sndIntervalSetinteger(&pSupHwParams->intervals[VX_SND_HW_PARAM_PERIODS]);
    if (ret < 0)
        {
        DBG_MSG(DBG_ERR, "[%d] Can't set hw_constraint: %d\n", id, ret);
        return ret;
        }

    data->substream = substream;

    msg.ipcid = IPC_PCMPLAYBACK;
    pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
    pcmtask_msg->hw_dma_id = data->dmaId;
    pcmtask_msg->irq_id = data->irqId;
    pcmtask_msg->pcm_alsa_id = data->pcm_dev_num;
    pcmtask_msg->domain_id = data->aboxData->myDomainId;
    msg.task_id = pcmtask_msg->pcm_device_id = id;

    ret = aboxPcmPlaybackIpcRequest(data, &msg, 0, 0);

    return ret;
    }

LOCAL int aboxPcmPlaybackClose
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID pdev = component->pDev;
    VXB_DEV_ID pdevAbox = vxbDevParent(pdev);
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int id = data->id;
    ABOX_IPC_MSG msg = {0, };
    IPC_PCMTASK_MSG * pcmtask_msg = &msg.msg.pcmtask;

    DBG_MSG(DBG_INFO, "PCM%d Playback : DMA %d\n", id, data->dmaId);

    data->substream = NULL;
    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        DBG_MSG(DBG_ERR, "[%d] DMA is disconnected\n", id);
        return ERROR;
        }

    msg.ipcid = IPC_PCMPLAYBACK;
    pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    aboxPcmPlaybackIpcRequest(data, &msg, 0, 1);

    abox_disable(pdevAbox);
    return OK;
    }

LOCAL int aboxPcmPlaybackIoctl
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream,
    unsigned int           cmd,
    void *                 arg
    )
    {
    return sndPcmDefaultIoctl(substream, cmd, arg);
    }

LOCAL int aboxPcmPlaybackHwParams
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

    DBG_MSG(DBG_INFO, "PCM%d Playback : DMA %d\n", id, data->dmaId);

    ret = snd_pcm_lib_malloc_pages(substream,
                                   INTER_VAL(params, VX_SND_HW_PARAM_BUFFER_BYTES));
    if (ret < 0) 
        {
        DBG_MSG(DBG_ERR, "Memory allocation failed (size:%u)\n",
                INTER_VAL(params, VX_SND_HW_PARAM_BUFFER_BYTES));
        return ret;
        }

    msg.ipcid = IPC_PCMPLAYBACK;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    pcmtask_msg->msgtype = PCM_SET_BUFFER;

    pcmtask_msg->param.setbuff.phyaddr = (int32_t)(substream->dmaBuf.phyAddr);
    pcmtask_msg->param.setbuff.size = INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES);
    pcmtask_msg->param.setbuff.count = INTER_VAL(params, VX_SND_HW_PARAM_PERIODS);

    ret = aboxPcmPlaybackIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        return ret;

    pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
    pcmtask_msg->param.hw_params.sample_rate = PARAMS_RATE(params);//INTER_VAL(params, VX_SND_HW_PARAM_RATE);
    pcmtask_msg->param.hw_params.bit_depth = PARAMS_WIDTH(params);
    pcmtask_msg->param.hw_params.channels = INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS);
    ret = aboxPcmPlaybackIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        {
        return ret;
        }

    period_time = GET_PERIOD_TIME(PARAMS_WIDTH(params),
                                  INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS),
                                  PARAMS_RATE(params),//INTER_VAL(params, VX_SND_HW_PARAM_RATE),
                                  INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES));

    if (data->aboxData->perfData.checked == FALSE)
        {
        int first_buf_size = INTER_VAL(params, VX_SND_HW_PARAM_PERIOD_BYTES) -
                             (DEFAULT_SBANK_SIZE + UAIF_INTERNAL_BUF_SIZE);
        signed long long pt =
            GET_PERIOD_TIME(PARAMS_WIDTH(params),
                            INTER_VAL(params, VX_SND_HW_PARAM_CHANNELS),
                            PARAMS_RATE(params),//INTER_VAL(params, VX_SND_HW_PARAM_RATE),
                            first_buf_size);

        if (pt < 0)
            pt = 0;

        data->aboxData->perfData.period_t = (unsigned long long)pt;
        }

    return 0;
    }

LOCAL int aboxPcmPlaybackIsEnabled
    (
    ABOX_PCM_CTRL_DATA * data
    )
    {
    unsigned int rdma_enabled = 0;

    if (!data->dmaData)
        {
        return 0;
        }
    switch (data->dmaData->type)
        {
        case DMA_TO_NON_SFT_UAIF:
        case DMA_TO_DMA:
            rdma_enabled = aboxRdmaIsEnabled(data->dmaData);
            break;
        default:
            break;
        }

    return rdma_enabled;
    }


LOCAL void aboxPcmPlaybackBarrierDisable
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
    while (aboxPcmPlaybackIsEnabled(data))
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

LOCAL int aboxPcmPlaybackHwFree
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

    msg.ipcid = IPC_PCMPLAYBACK;
    pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    ret = aboxPcmPlaybackIpcRequest(data, &msg, 0, 0);
    if (ret < 0)
        {
        return ret;
        }

    aboxPcmPlaybackBarrierDisable(pdev, data);

    /* Delay between RDMA and UAIF stop to flush FIFO in UAIF */

   vxbUsDelay(1200);

    return snd_pcm_lib_free_pages(substream);
    }

LOCAL int aboxPcmPlaybackPrepare
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

    msg.ipcid = IPC_PCMPLAYBACK;
    pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
    msg.task_id = pcmtask_msg->pcm_device_id = id;
    ret = aboxPcmPlaybackIpcRequest(data, &msg, 0, 0);

    return ret;
    }

LOCAL int aboxPcmPlaybackTrigger
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream,
    int cmd
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;
    int                  ret;
    ABOX_IPC_MSG         msg = {0, };
    IPC_PCMTASK_MSG *    pcmtask_msg = &msg.msg.pcmtask;

    msg.ipcid = IPC_PCMPLAYBACK;
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
            ret = aboxPcmPlaybackIpcRequest(data, &msg, 1, 0);
            break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
            {
            return ERROR;
            }

        pcmtask_msg->param.dma_trigger.trigger = 0;
        ret = aboxPcmPlaybackIpcRequest(data, &msg, 1, 0);
        abox_request_dram_on(data->pdevAbox, pdev, FALSE);
        break;
    default:
        ret = -EINVAL;
        break;
        }

    return ret;
    }

LOCAL SND_FRAMES_T aboxPcmPlaybackPointer
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM *    substream
    )
    {
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;

    ssize_t pointer = 0;
    unsigned int buf_status = 0;
    SND_FRAMES_S_T frames = 0;

      if (!data->dmaData && !data->pp_enabled)
        {
        return 0;
        }

    if (data->pp_enabled)
        {
        pointer = REG_READ_8(data->pp_pointer_base, 0);
        }
    else
        {
        switch (data->dmaData->type)
            {
            case DMA_TO_NON_SFT_UAIF:
            case DMA_TO_DMA:
                buf_status = aboxRdmaBufferStatusGet(data->dmaData);

                DBG_MSG(DBG_INFO, "buf_status=0x%x\n", buf_status);
                break;
            default:
                break;
            }
        if (data->pointer >= substream->dmaBuf.phyAddr)
            {
            pointer = data->pointer - substream->dmaBuf.phyAddr;
            }
        else if (buf_status)
            {
            ssize_t offset, count;
            ssize_t buffer_bytes, period_bytes;

            buffer_bytes = sndPcmlibBufferBytes(substream);
            period_bytes = sndPcmlibPeriodBytes(substream);

            offset = (((buf_status & ABOX_RDMA_RBUF_STATUS_MASK) >>
                      ABOX_RDMA_RBUF_OFFSET_L) << 4);
            count = (buf_status & ABOX_RDMA_RBUF_CNT_MASK);

            DBG_MSG(DBG_INFO, "buffer_bytes=%lld period_bytes=%lld"
                    "offset=%lld count=%lld\n", buffer_bytes, period_bytes,
                    offset, count);

            while ((offset % period_bytes) && (buffer_bytes >= 0))
                {
                buffer_bytes -= period_bytes;
                if ((buffer_bytes & offset) == offset)
                    offset = buffer_bytes;
                }
            pointer = offset + count;

            DBG_MSG(DBG_INFO, "offset=%lld count=%lld pointer=%lld\n",
                    offset, count, pointer);
            }
        else
            {
            pointer = 0;
            }
        }

    frames = bytesToFrames(runtime, pointer);

    DBG_MSG(DBG_INFO, "PCM[%d]: pointer:%zd frames:%ld\n", id, pointer, frames);

    return frames;
    }

LOCAL int aboxPcmPlaybackMmap
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM    * substream
    )
    {
#ifdef ABOX_PLAYBACK_DEBUG
    VXB_DEV_ID           pdev = component->pDev;
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);
    int                  id = data->id;

    DBG_MSG(DBG_INFO, "PCM%d Capture : DMA %d\n", id, data->dmaId);
#endif

    return 0;

    }


LOCAL void aboxPcmPlaybackOpsSet
    (
    VX_SND_SOC_CMPT_DRV * drv
    )
    {
    drv->open		= aboxPcmPlaybackOpen;
    drv->close		= aboxPcmPlaybackClose;
    drv->ioctl		= aboxPcmPlaybackIoctl;
    drv->hwParams		= aboxPcmPlaybackHwParams;
    drv->hwFree		= aboxPcmPlaybackHwFree;
    drv->prepare		= aboxPcmPlaybackPrepare;
    drv->trigger		= aboxPcmPlaybackTrigger;
    drv->pointer		= aboxPcmPlaybackPointer;
    drv->mmap		= aboxPcmPlaybackMmap;
    }


LOCAL STATUS aboxPcmPlaybackCmpntRegister
    (
    VXB_DEV_ID pdev,
    ABOX_PCM_CTRL_DATA * data
    )
    {
    VX_SND_SOC_CMPT_DRV * cmpntDrv;
    char *                daiName = NULL;
    char *                streamName = NULL;
    int                   i = 0;

    data->daiDrv = vxbMemAlloc(sizeof(aboxPcmPlaybackDaiDrv));
    if (data->daiDrv == NULL)
        {
        return ERROR;
        }

    memcpy(data->daiDrv, aboxPcmPlaybackDaiDrv, sizeof(aboxPcmPlaybackDaiDrv));
    for (i = 0; i < ELEMENTS (aboxPcmPlaybackDaiDrv); i++)
        {
        data->daiDrv[i].id = data->id;
        daiName = (char *)vxbMemAlloc(strlen(data->daiDrv[i].name) + 1);
        snprintf_s(daiName, strlen(data->daiDrv[i].name) + 1,
                   data->daiDrv[i].name, data->id);
        data->daiDrv[i].name = daiName;
        streamName = (char *)vxbMemAlloc(strlen(data->daiDrv[i].playback.streamName) + 1);
        snprintf_s(streamName, strlen(data->daiDrv[i].playback.streamName) + 1,
                   data->daiDrv[i].playback.streamName, data->id);
        data->daiDrv[i].playback.streamName = streamName;
        }

    cmpntDrv = (VX_SND_SOC_CMPT_DRV *)vxbMemAlloc(sizeof(VX_SND_SOC_CMPT_DRV));
    memcpy(cmpntDrv, &aboxPcmPlaybackBase, sizeof(aboxPcmPlaybackBase));

    aboxPcmPlaybackValidation(pdev);

    if (aboxPcmPlaybackControlMake (pdev, data, cmpntDrv) != OK)
        {
        DBG_MSG(DBG_ERR, "failed to make playback controls\n");
        return ERROR;
        }

    aboxPcmPlaybackOpsSet(cmpntDrv);

    if (vxSndCpntRegister (pdev, cmpntDrv, data->daiDrv,
                           ELEMENTS (aboxPcmPlaybackDaiDrv)) != OK)
        {
        DBG_MSG (DBG_ERR, "component register failed\n");
        return ERROR;
        }

    return OK;
    }

STATUS abox_pcm_playback_irq_handler
    (
    uint32_t pcm_id,
    VXB_DEV_ID pdev
    )
    {
    ABOX_PCM_CTRL_DATA * data = vxbDevSoftcGet(pdev);

    data->pointer = 0;
    snd_pcm_period_elapsed(data->substream);

    return OK;
    }

LOCAL STATUS aboxPcmPlaybackProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxPcmPlaybackMatch, NULL);
    }

LOCAL STATUS aboxPcmPlaybackAttach
    (
    VXB_DEV_ID pdev
    )
    {
    VXB_FDT_DEV *                  pFdtDev = NULL;
    uint32_t *                     prop = NULL;
    VXB_RESOURCE *                 pMemRegs = NULL;
    VXB_RESOURCE_ADR *             pRegsAddr = NULL;
    int                            len;
    ABOX_PCM_CTRL_DATA *           data;
    STATUS                         ret;

    DBG_MSG(DBG_INFO, "Start.\n");

    pFdtDev = vxbFdtDevGet (pdev);
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "no FDT device.\n");
        return ERROR;
        }

    data = (ABOX_PCM_CTRL_DATA *)vxbMemAlloc(sizeof(ABOX_PCM_CTRL_DATA));
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "no memory.\n");
        return ERROR;
        }

    vxbDevSoftcSet(pdev, data);
    data->pdev = pdev;
    data->dmaId = ABOX_RDMA_NO_CONNECT;
    data->streamType = SNDRV_PCM_STREAM_PLAYBACK;

    data->pdevAbox = vxbDevParent(pdev);
    if (data->pdevAbox == NULL)
        {
        return ERROR;
        }
    data->aboxData = vxbDevSoftcGet(data->pdevAbox);

    // Reads the PCM playback id from the device tree and stores in data->id

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

    data->pp_pointer_base = (VIRT_ADDR)
                            ((char *)data->aboxData->adspData[HIFI_CORE1].virtAddr
                            + pRegsAddr->start);
    if (data->pp_pointer_base)
        {
        DBG_MSG(DBG_INFO, "pp_pointer:%p pp_pointer_phys:%llx\n",
        data->pp_pointer_base, data->pp_pointer_phys);
        }

    // Reads the PCM playback ALSA Max Buffer size from the device tree

    hwParamsSet(pFdtDev->offset, &data->aboxDmaHardware);

    DBG_MSG(DBG_INFO, "id:%d irq_id:%d buffer_bytes_max : %lu\n",
        data->id, data->irqId,
        data->aboxDmaHardware.bufferBytesMax);

    ret = abox_register_pcm_playback(data->aboxData->pDev, pdev, data->id);
    if (ret < 0)
        {
        return ret;
        }

    ret = abox_gic_set_pcm_irq_register(data->irqId,
                                        abox_pcm_playback_irq_handler, pdev);
    if (ret == ERROR)
        {
        return ret;
        }

    ret = aboxPcmPlaybackCmpntRegister(pdev, data);
    if (ret == ERROR)
        {
        return ret;
        }

    DBG_MSG(DBG_INFO, "End.\n");
    return ret;
    }


