
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/* vxbFdtAboxWdma.c - Samsung Abox Wdma driver */

/*
DESCRIPTION

This is the VxBus driver for Samsung Abox Wdma.
*/

/* includes */

#include <vxWorks.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/util/vxbAccess.h>
#include <vxbFdtAboxDma.h>
#include <vxbFdtAboxUaif.h>

/* debug macro */

#undef ABOX_WDMA_DEBUG
#ifdef ABOX_WDMA_DEBUG
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
#endif  /* ABOX_WDMA_DEBUG */

/* register access */

#ifdef ARMBE8
#    define SWAP32 vxbSwap32
#else
#    define SWAP32
#endif /* ARMBE8 */

#undef REG_READ_4
#define REG_READ_4(pHandle, regBase, offset)                            \
        SWAP32 (vxbRead32(pHandle, (uint32_t *)((UINT8 *)regBase + offset)))

#undef REG_WRITE_4
#define REG_WRITE_4(pHandle, regBase, offset, value)                    \
        vxbWrite32((pHandle),                                           \
                   (uint32_t *)((UINT8 *)regBase + offset), SWAP32(value))

#undef REG_SETBIT_4
#define REG_SETBIT_4(pHandle, regBase, offset, data)                    \
        REG_WRITE_4(pHandle, regBase, offset,                           \
                    REG_READ_4(pHandle, regBase, offset) | (data))

#undef REG_CLEARBIT_4
#define REG_CLEARBIT_4(pHandle, regBase, offset, mask)                  \
        REG_WRITE_4 (pHandle, regBase, offset,                          \
                     REG_READ_4(pHandle, regBase, offset) & (~(mask)))

/* typedefs */

/* forward declarations */

LOCAL STATUS fdtAboxWdmaProbe(VXB_DEV_ID  pDev);
LOCAL STATUS fdtAboxWdmaAttach(VXB_DEV_ID  pDev);
LOCAL STATUS wdmaComponentRegister(VXB_DEV_ID  pDev,
                                   struct abox_dma_data * data);
LOCAL int wdmaRouteValidate(VXB_DEV_ID  pDev);

LOCAL int wdmaReduceGet(VX_SND_CONTROL *kcontrol,
                        VX_SND_CTRL_DATA_VAL *ucontrol);
LOCAL int wdmaReduceSet(VX_SND_CONTROL *kcontrol,
                        VX_SND_CTRL_DATA_VAL *ucontrol);

LOCAL int wdmaAsrcRouteGet(VX_SND_CONTROL *kcontrol,
                           VX_SND_CTRL_DATA_VAL *ucontrol);
LOCAL int wdmaAsrcRouteSet(VX_SND_CONTROL *kcontrol,
                           VX_SND_CTRL_DATA_VAL *ucontrol);
LOCAL STATUS wdmaControlsMake(VXB_DEV_ID pDev, struct abox_dma_data *data,
                             VX_SND_SOC_CMPT_DRV *cmpnt);
LOCAL STATUS wdmaCmpntProbe(VX_SND_SOC_COMPONENT * component);
LOCAL void wdmaCmpntRemove(VX_SND_SOC_COMPONENT * component);
LOCAL int wdmaRouteGet(VX_SND_CONTROL * kcontrol,
                       VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL int wdmaRoutePut(VX_SND_CONTROL * kcontrol,
                       VX_SND_CTRL_DATA_VAL * ucontrol);
LOCAL void wdmaRegBitSet(ABOX_DMA_CTRL  * pDevCtrl,void  * regBase,
                         uint32_t  offset, uint32_t  data);
LOCAL void wdmaRegBitClean(ABOX_DMA_CTRL  * pDevCtrl, void  * regBase,
                           uint32_t  offset, uint32_t  mask);
LOCAL uint32_t wdmaRegRead(ABOX_DMA_CTRL  * pDevCtrl, void  * regBase,
                           uint32_t  offset);
LOCAL void wdmaRegWrite(ABOX_DMA_CTRL  * pDevCtrl, void  * regBase,
                        uint32_t  offset, uint32_t  value);
LOCAL uint32_t wdmaCmpntRead(VX_SND_SOC_COMPONENT * component,
                             uint32_t reg);
LOCAL STATUS wdmaCmpntWrite(VX_SND_SOC_COMPONENT * component,
                            uint32_t reg, uint32_t val);

IMPORT int aboxAsrcSpmMaxGet(void);
IMPORT const char * const *aboxAsrcSpumTextsGet(void);
IMPORT int aboxReduceSpusTextsNumGet(void);
IMPORT const char * const *aboxReduceSpusTextsGet(void);


/* locals */

LOCAL VXB_DRV_METHOD    fdtAboxWdmaMethodList[] =
    {
    { VXB_DEVMETHOD_CALL(vxbDevProbe), fdtAboxWdmaProbe},
    { VXB_DEVMETHOD_CALL(vxbDevAttach), fdtAboxWdmaAttach},
    { 0, NULL }
    };

LOCAL const VXB_FDT_DEV_MATCH_ENTRY aboxWdmaMatch[] =
    {
        {
        ABOX_WDMA_DRIVER_NAME,                  /* compatible */
        (void *)NULL,
        },
        {}                                      /* Empty terminated list */
    };

LOCAL VX_SND_SOC_DAI_DRIVER aboxWdmaDaiDrv =
    {
    .name = "WDMA%d",
    .capture =
        {
        .streamName = "WDMA%d Interface",
        .channelsMin = 1,
        .channelsMax = 32,
        .rates = ABOX_SAMPLING_RATES,
        .rateMin = 8000,
        .rateMax = 384000,
        .formats = ABOX_SAMPLE_FORMATS,
        },
    };

LOCAL VX_SND_SOC_CMPT_DRV aboxWdmaBase =
    {
    .probe = wdmaCmpntProbe,
    .remove = wdmaCmpntRemove,
    .probe_order	= SND_SOC_COMP_ORDER_NORMAL,
    .read = wdmaCmpntRead,
    .write = wdmaCmpntWrite,
    };

LOCAL char nsrc_v920_ctl_texts[][ENUM_CTRL_STR_SIZE] =
    {
     "NO_CONNECT", "NO_CONNECT", /* UAIF0 Main, UAIF0 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF0 Third, UAIF0 Fourth */
     "NO_CONNECT", "NO_CONNECT", /* UAIF1 Main, UAIF1 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF1 Third, UAIF1 Fourth */
     "NO_CONNECT", "NO_CONNECT", /* UAIF2 Main, UAIF2 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF3 Main, UAIF3 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF4 Main, UAIF4 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF5 Main, UAIF5 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF6 Main, UAIF6 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF7 Main, UAIF7 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF8 Main, UAIF8 Sec */
     "NO_CONNECT", "NO_CONNECT", /* UAIF9 Main, UAIF9 Sec */
    };

LOCAL char reduceSpusTexts[][ENUM_CTRL_STR_SIZE] = {"WDMA",};

LOCAL char asrcSpmsTexts[][ENUM_CTRL_STR_SIZE] =
    {
    "Reduce Route",         /* Reduce Route */
    };



/* globals */

/* Samsung Abox Wdma driver */

VXB_DRV vxbFdtAboxWdmaDrv =
    {
    {NULL},
    "aboxWdma",                                 /* Name */
    "Abox Wdma Fdt driver",                     /* Description */
    VXB_BUSID_FDT,                              /* Class */
    0,                                          /* Flags */
    0,                                          /* Reference count */
    fdtAboxWdmaMethodList,                      /* Method table */
    };

VXB_DRV_DEF(vxbFdtAboxWdmaDrv);

/* functions */

inline const char * const *aboxReduceSpusTextsGet(void)
    {
    static const char * const reduceTexts[] =
        {
        reduceSpusTexts[0]
        };
    return reduceTexts;
    }

inline int aboxReduceSpusTextsNumGet(void)
    {
    return ELEMENTS(reduceSpusTexts);
    }

inline const char * const *aboxAsrcSpumTextsGet(void)
    {
    static const char * const asrcTexts[] =
        {
        asrcSpmsTexts[0]
        };
    return asrcTexts;
    }

inline int aboxAsrcSpmMaxGet(void)
    {
    return ELEMENTS(asrcSpmsTexts);
    }

LOCAL uint32_t wdmaCmpntRead
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg
    )
    {
    struct abox_dma_data * data = NULL;
    VXB_DEV_ID             pDev = component->pDev;
    ABOX_DMA_CTRL  *       pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    data = (struct abox_dma_data *) pDevCtrl->pDmaData;

    return wdmaRegRead(pDevCtrl, data->sfr_base, reg);
    }

LOCAL uint32_t wdmaRegRead
    (
    ABOX_DMA_CTRL  * pDevCtrl,
    void           * regBase,
    uint32_t         offset
    )
    {
    uint32_t  regValue;

    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    regValue = REG_READ_4(pDevCtrl->handle, regBase, offset);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);

    return regValue;
    }

LOCAL STATUS wdmaCmpntWrite
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg,
    uint32_t               val
    )
    {
    struct abox_dma_data * data = NULL;
    VXB_DEV_ID             pDev = component->pDev;
    ABOX_DMA_CTRL *        pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    data = (struct abox_dma_data *) pDevCtrl->pDmaData;

    wdmaRegWrite(pDevCtrl, data->sfr_base, reg, val);

    return OK;
    }

LOCAL void wdmaRegWrite
    (
    ABOX_DMA_CTRL  * pDevCtrl,
    void           * regBase,
    uint32_t         offset,
    uint32_t         value
    )
    {
    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    REG_WRITE_4(pDevCtrl->handle, regBase, offset, value);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);
    }

LOCAL void wdmaRegBitSet
    (
    ABOX_DMA_CTRL  * pDevCtrl,
    void           * regBase,
    uint32_t         offset,
    uint32_t         data
    )
    {
    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    REG_SETBIT_4(pDevCtrl->handle, regBase, offset, data);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);
    }

LOCAL void wdmaRegBitClean
    (
    ABOX_DMA_CTRL  * pDevCtrl,
    void           * regBase,
    uint32_t         offset,
    uint32_t         mask
    )
    {
    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    REG_CLEARBIT_4(pDevCtrl->handle, regBase, offset, mask);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);
    }


LOCAL STATUS wdmaCmpntProbe
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID      pDev;
    ABOX_DMA_CTRL * pDevCtrl = NULL;

    pDev = component->pDev;
    pDevCtrl = vxbDevSoftcGet(pDev);
    pDevCtrl->pDmaData->cmpnt = component;

    return OK;
    }

LOCAL void wdmaCmpntRemove
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID             pDev;
    ABOX_DMA_CTRL *        pDevCtrl = NULL;
    struct abox_dma_data * pDmaData = NULL;

    pDev = component->pDev;
    pDevCtrl = vxbDevSoftcGet(pDev);
    pDmaData = pDevCtrl->pDmaData;

    }

//abox_wdma_get_status
uint32_t aboxWdmaGetStatus
    (
    struct abox_dma_data *  pDmaData
    )
    {
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL  *        pDevCtrl = NULL;
    uint32_t                sfrVal = 0;
    uint32_t                wdmaStatus = 0;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet (pDev);

    sfrVal = wdmaRegRead (pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_STATUS);
    wdmaStatus =
        (sfrVal & ABOX_WDMA_PROGRESS_MASK) >> ABOX_WDMA_PROGRESS_SHIFT;

    return wdmaStatus;
    }

void aboxWdmaDisable
    (
    struct abox_dma_data  * pDmaData
    )
    {
    VXB_DEV_ID       pDev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet(pDev);

    wdmaRegBitClean(pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_CTRL, 0x1);
    }

uint32_t wdmaIsEnable
    (
    struct abox_dma_data  * pDmaData
    )
    {
    uint32_t           retVal;
    VXB_DEV_ID       pDev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet(pDev);
    retVal = wdmaRegRead(pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_CTRL);
    retVal &= 0x1;

    return retVal;
    }

void aboxWdmaEnable
    (
    struct abox_dma_data  * pDmaData
    )
    {
    VXB_DEV_ID       pDev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet(pDev);

    wdmaRegBitSet(pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_CTRL, 0x1);
    }

uint32_t aboxWdmaBufferStatuGet
    (
    struct abox_dma_data   * pDmaData
    )
    {
    VXB_DEV_ID       pDev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;
    uint32_t         retVal;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet(pDev);

    retVal = wdmaRegRead(pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_STATUS);
    retVal = retVal & (~(ABOX_WDMA_WBUF_STATUS_MASK << 28));
    return retVal;
    }

uint32_t wdmaReduceEnable
    (
    struct abox_dma_data   * pDmaData
    )
    {
    uint32_t         retVal;
    VXB_DEV_ID       pDev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDev = pDmaData->pdev;
    pDevCtrl = vxbDevSoftcGet(pDev);

    retVal = wdmaRegRead(pDevCtrl, pDmaData->sfr_base, ABOX_WDMA_CTRL);
    retVal = (retVal & BIT(ABOX_WDMA_REDUCE_L)) >> ABOX_WDMA_REDUCE_L;
    return retVal;
    }

int wdmaRequestIpc
    (
    struct abox_dma_data * data,
    ABOX_IPC_MSG *         msg,
    int                    atomic,
    int                    sync
    )
    {
    VXB_DEV_ID pDevAbox;

    pDevAbox = data->pdev_abox;
    return abox_request_ipc(pDevAbox, msg->ipcid, msg, sizeof(*msg),
                            atomic, sync);
    }

/*******************************************************************************
*
* fdtAboxWdmaProbe - probe for device presence at specific address
*
* Check for Samsung Abox wdma device at the specified base address.
* We assume one is present at that address, but we need to verify.
*
* RETURNS: OK if probe passes and assumed a valid Samsung Abox wdma device.
* ERROR otherwise.
*
* ERRNO: N/A
*/

LOCAL STATUS fdtAboxWdmaProbe
    (
    VXB_DEV_ID  pDev
    )
    {
    DBG_MSG (DBG_INFO, "Start\n");
    return vxbFdtDevMatch (pDev, aboxWdmaMatch, NULL);
    }

LOCAL STATUS wdmaReduceGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    return snd_soc_get_enum_double (kcontrol, ucontrol);
    }

LOCAL STATUS wdmaReduceSet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data *  pDmaData;
    VX_SND_SOC_COMPONENT *  cmpt;
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL *         pDevCtrl  = NULL;
    unsigned int            item      = ucontrol->value.enumerated.item[0];

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

    pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet (pDev);
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find pDevCtrl\n");
        return ERROR;
        }

    pDmaData = (struct abox_dma_data *) pDevCtrl->pDmaData;
    if (pDmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find struct abox_dma_data from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    if (item != 0)
        {
        DBG_MSG(DBG_ERR, "The value of item should be zero!\n");
        return ERROR;
        }
    return snd_soc_put_enum_double (kcontrol, ucontrol);
    }

LOCAL STATUS wdmaAsrcRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data *  pDmaData;
    VX_SND_SOC_COMPONENT *  cmpt;
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL *         pDevCtrl = NULL;

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

    pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet (pDev);
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find pDevCtrl\n");
        return ERROR;
        }

    pDmaData = (struct abox_dma_data *) pDevCtrl->pDmaData;
    if (pDmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find struct abox_dma_data from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    ucontrol->value.enumerated.item[0] = 0;
    return OK;
    }

LOCAL int wdmaAsrcRouteSet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data *  pDmaData;
    VX_SND_SOC_COMPONENT *  cmpt;
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL *         pDevCtrl = NULL;
    unsigned int            item     = ucontrol->value.enumerated.item[0];

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

    pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet (pDev);
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find pDevCtrl\n");
        return ERROR;
        }

    pDmaData = (struct abox_dma_data *) pDevCtrl->pDmaData;
    if (pDmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find struct abox_dma_data from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    VXB_DEV_ID  pDevAbox;
    ABOX_IPC_MSG             msg = {0, };
    IPC_ABOX_CONFIG_MSG    * abox_config_msg = &msg.msg.config;

    if (item != 0)
        {
        DBG_MSG(DBG_ERR, "The item of value should be zero!\n");
        return ERROR;
        }

    wdmaRegBitClean(pDevCtrl, pDmaData->sfr_base,
                        ABOX_WDMA_SPUM_CTRL0, ABOX_SPUM_FUNC_CHAIN_ASRC);
    wdmaRegBitClean(pDevCtrl, pDmaData->sfr_base,
                        ABOX_WDMA_SPUM_CTRL0, 0x1f << ABOX_SPUM_ASRC_ID);

    pDevAbox = vxbDevParent(pDev);
    if (pDevAbox == NULL)
        {
        DBG_MSG(DBG_INFO, "Can't find device abox.\n");
        return ERROR;
        }
    msg.ipcid = IPC_ABOX_CONFIG;
    abox_config_msg->msgtype = ASRC_HW_INFO;
    abox_config_msg->param.asrc_hw_info.enable = FALSE;
    abox_request_ipc(pDevAbox, msg.ipcid, &msg, sizeof(msg), 0, 0);
    return OK;
    }

/*******************************************************************************
*
* wdmaRouteValidate -
*
*
*
* RETURNS: the number of the validate uaif data line.
*
* ERRNO: N/A
*/

LOCAL int wdmaRouteValidate
    (
    VXB_DEV_ID  pDev
    )
    {
    int                    ret = -1;
    VXB_DEV_ID             pDevParent;
    VXB_FDT_DEV          * pFdtDevParent;
    int                    offset;
    int                    nameLen;
    int                    propLen;
    uint32_t             * prop;
    uint32_t               dataLine;
    uint32_t               totalUaifLine = 0;
    uint32_t               index = 0;
    uint32_t               uaifId;
    uint32_t               uaifType;
    char                 * pName = NULL;
    char (*nsrc_texts)[ENUM_CTRL_STR_SIZE] = nsrc_v920_ctl_texts;

    pDevParent = vxbDevParent(pDev);
    pFdtDevParent = vxbFdtDevGet(pDevParent);
    if (pFdtDevParent == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        return ret;
        }
    offset = pFdtDevParent->offset;

    for(offset = vxFdtFirstSubnode (offset);
        offset >= 0;
        offset  = vxFdtNextSubnode (offset))
        {
        pName = (char *)vxFdtGetName(offset, &nameLen);
        if (pName != NULL)
            {

            /* find node "abox_uaif" */

            if (strncmp(pName, "abox_uaif", strlen("abox_uaif")) == 0)
                {
                prop = (uint32_t *)vxFdtPropGet(offset,
                                              "samsung,data-line", &propLen);
                if ((prop == NULL) || (propLen != sizeof(int)))
                    {
                    DBG_MSG(DBG_ERR, "Can't get data-line.\n");
                    return ret;
                    }
                dataLine = vxFdt32ToCpu(*prop);
                if (!vxFdtIsEnabled(offset))
                    {
                    index += dataLine;
                    continue;
                    }
                prop = (uint32_t *)vxFdtPropGet(offset, "samsung,id", &propLen);
                if ((prop == NULL) || (propLen != sizeof(int)))
                    {
                    DBG_MSG (DBG_ERR, "failed to get samsung id.\n");
                    return ret;
                    }
                uaifId = vxFdt32ToCpu(*prop);

                prop = (uint32_t *)vxFdtPropGet(offset, "samsung,type", &propLen);
                if ((prop == NULL) || (propLen != sizeof(int)))
                    {
                    DBG_MSG (DBG_ERR, "failed to get samsung type.\n");
                    return ret;
                    }
                uaifType = vxFdt32ToCpu(*prop);
                DBG_MSG(DBG_INFO, "id: %d, type: %d\n", uaifId, uaifType);

                if (uaifType == UAIF_NON_SFT)
                    {
                    snprintf_s(nsrc_texts[index++], ENUM_CTRL_STR_SIZE - 1,
                               "UAIF%d MAIN MIC", uaifId);
                    snprintf_s(nsrc_texts[index++], ENUM_CTRL_STR_SIZE - 1,
                               "UAIF%d SEC MIC", uaifId);
                    if (dataLine == UAIF_SD_MAX)
                        {
                        snprintf_s(nsrc_texts[index++], ENUM_CTRL_STR_SIZE - 1,
                                   "UAIF%d THIRD MIC", uaifId);
                        snprintf_s(nsrc_texts[index++], ENUM_CTRL_STR_SIZE - 1,
                                   "UAIF%d FOURTH MIC", uaifId);
                        }
                    totalUaifLine += dataLine;
                    }
                else
                    {
                    index += dataLine;
                    }
                }
            }
        }
    return totalUaifLine;
    }

LOCAL STATUS wdmaRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    return snd_soc_get_enum_double (kcontrol, ucontrol);
    }

LOCAL STATUS wdmaRoutePut
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    int routeToUaif[] =
        {
        ABOX_UAIF0, ABOX_UAIF0, ABOX_UAIF0, ABOX_UAIF0,
        ABOX_UAIF1, ABOX_UAIF1, ABOX_UAIF1, ABOX_UAIF1,
        ABOX_UAIF2, ABOX_UAIF2, ABOX_UAIF3, ABOX_UAIF3,
        ABOX_UAIF4, ABOX_UAIF4, ABOX_UAIF5, ABOX_UAIF5,
        ABOX_UAIF6, ABOX_UAIF6, ABOX_UAIF7, ABOX_UAIF7,
        ABOX_UAIF8, ABOX_UAIF8, ABOX_UAIF9, ABOX_UAIF9
        };
    int sdOfUaif[] =
        {
        UAIF_SD3, UAIF_SD2, UAIF_SD1, UAIF_SD0,
        UAIF_SD3, UAIF_SD2, UAIF_SD1, UAIF_SD0,
        UAIF_SD1, UAIF_SD0, UAIF_SD1, UAIF_SD0,
        UAIF_SD1, UAIF_SD0, UAIF_SD1, UAIF_SD0,
        UAIF_SD1, UAIF_SD0, UAIF_SD1, UAIF_SD0,
        UAIF_SD1, UAIF_SD0, UAIF_SD1, UAIF_SD0,
        };
    int32_t                route;
    uint32_t               item;
    ABOX_DMA_CTRL *        pDevCtrl = NULL;
    struct abox_dma_data * data;
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    VXB_DEV_ID             pUaifDev;
    char                   frontName[SND_DEV_NAME_LEN];
    char                   backName[SND_DEV_NAME_LEN];

    item = ucontrol->value.enumerated.item[0];
    route = item - 0x20;
    if (route < 0)
        {
        DBG_MSG (DBG_ERR, "item %d should be greater than 0x20", item);
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

    pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet (pDev);
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find pDevCtrl\n");
        return ERROR;
        }

    data = (struct abox_dma_data *) pDevCtrl->pDmaData;
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find struct abox_dma_data from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }


    pUaifDev = data->abox_data->uaifDev[routeToUaif[route]];
    vxbAboxUaifModeSet (vxbDevSoftcGet (pUaifDev), sdOfUaif[route],
                        SNDRV_PCM_STREAM_CAPTURE, UAIF_LINE_MODE_MIC);

    if (snd_soc_put_enum_double(kcontrol, ucontrol) != OK)
        {
        return ERROR;
        }

    /* set backend */

    (void) snprintf_s (frontName, SND_DEV_NAME_LEN, "WDMA%d",
                       data->dai_drv->id);

    (void) snprintf_s (backName, SND_DEV_NAME_LEN, "UAIF%d",
                       routeToUaif[route]);


    if (sndSocBeConnectByName (cmpt->card, (const char *) frontName,
                               (const char *) backName,
                               SNDRV_PCM_STREAM_CAPTURE) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to connect dynamic PCM\n");
        return ERROR;
        }

    return OK;
    }

LOCAL STATUS wdmaControlsMake
    (
    VXB_DEV_ID             pDev,
    struct abox_dma_data * data,
    VX_SND_SOC_CMPT_DRV  * cmpnt
    )
    {
    VX_SND_ENUM *             wdmaEnum    = NULL;
    VX_SND_CONTROL *          wdmaControl = NULL;
    size_t size;
    static const char * const nsrcx_v920_texts[] =
        {
        "NO_CONNECT", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "",
        "", "",
        nsrc_v920_ctl_texts[0], nsrc_v920_ctl_texts[1],
        nsrc_v920_ctl_texts[2], nsrc_v920_ctl_texts[3],
        nsrc_v920_ctl_texts[4], nsrc_v920_ctl_texts[5],
        nsrc_v920_ctl_texts[6], nsrc_v920_ctl_texts[7],
        nsrc_v920_ctl_texts[8], nsrc_v920_ctl_texts[9],
        nsrc_v920_ctl_texts[10], nsrc_v920_ctl_texts[11],
        nsrc_v920_ctl_texts[12], nsrc_v920_ctl_texts[13],
        nsrc_v920_ctl_texts[14], nsrc_v920_ctl_texts[15],
        nsrc_v920_ctl_texts[16], nsrc_v920_ctl_texts[17],
        nsrc_v920_ctl_texts[18], nsrc_v920_ctl_texts[19],
        nsrc_v920_ctl_texts[20], nsrc_v920_ctl_texts[21],
        nsrc_v920_ctl_texts[22], nsrc_v920_ctl_texts[23]
        };
    uint32_t              reg             = ABOX_WDMA_SPUM_CTRL0;
    uint32_t              shift           = ABOX_WDMA_ROUTE_NSRC_L;
    const char * const *  nsrcxTexts      = nsrcx_v920_texts;
    uint32_t              nsrcxSize       = ELEMENTS (nsrcx_v920_texts);
    int                   reduceTextsNum  = aboxReduceSpusTextsNumGet();
    const char * const *  reduceTexts     = aboxReduceSpusTextsGet();
    int                   num_of_max_asrc = aboxAsrcSpmMaxGet();
    const char * const *  asrcTexts       = aboxAsrcSpumTextsGet();
    uint32_t              idx;
    VX_SND_ENUM           wdmaEnumBase[]  =
        {
        SOC_ENUM_DOUBLE (ABOX_WDMA_CTRL, ABOX_WDMA_REDUCE_L,
                         ABOX_WDMA_REDUCE_H, reduceTextsNum, reduceTexts),
        SOC_ENUM_DOUBLE (SND_SOC_NOPM, 0, 0, num_of_max_asrc, asrcTexts),
        SOC_ENUM_DOUBLE (reg, shift, shift, nsrcxSize, nsrcxTexts),
        };
    const VX_SND_CONTROL wdmaControlBase[] =
        {
        SOC_ENUM_EXT ("WDMA%d Reduce Route", wdmaEnumBase[0], wdmaReduceGet,
                      wdmaReduceSet),
        SOC_ENUM_EXT ("SIFM%d", wdmaEnumBase[1], wdmaAsrcRouteGet,
                      wdmaAsrcRouteSet),
        SOC_ENUM_EXT ("NSRC%d", wdmaEnumBase[2], wdmaRouteGet, wdmaRoutePut),
        };

    size = sizeof (VX_SND_ENUM) * ELEMENTS (wdmaEnumBase);
    wdmaEnum = (VX_SND_ENUM *) vxbMemAlloc (size);
    if (wdmaEnum == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to alloc wdmaEnum\n");
        goto errOut;
        }
    (void) memcpy_s (wdmaEnum, size, wdmaEnumBase, size);

    size = sizeof (VX_SND_CONTROL) * ELEMENTS (wdmaControlBase);
    wdmaControl = (VX_SND_CONTROL *) vxbMemAlloc (size);
    if (wdmaControl == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to alloc wdmaControl\n");
        goto errOut;
        }
    (void) memcpy_s (wdmaControl, size, wdmaControlBase, size);

    for (idx = 0; idx < ELEMENTS (wdmaControlBase); idx++)
        {
        wdmaControl[idx].privateValue = (unsigned long) &wdmaEnum[idx];
        wdmaControl[idx].id.name = vxbMemAlloc (SND_DEV_NAME_LEN);
        (void) snprintf_s (wdmaControl[idx].id.name, SND_DEV_NAME_LEN,
                           wdmaControlBase[idx].id.name, data->dai_drv->id);
        }

    cmpnt->controls     = wdmaControl;
    cmpnt->num_controls = ELEMENTS (wdmaControlBase);

    return OK;

errOut:
    if (wdmaControl != NULL)
        {
        for (idx = 0; idx < ELEMENTS (wdmaControlBase); idx++)
            {
            if (wdmaControl[idx].id.name != NULL)
                {
                vxbMemFree (wdmaControl[idx].id.name);
                }
            }
        vxbMemFree (wdmaControl);
        }
    if (wdmaEnum != NULL)
        {
        vxbMemFree (wdmaEnum);
        }

    return ERROR;
    }

/*******************************************************************************
*
* wdmaComponentRegister -
*
*
*
* RETURNS: OK or ERROR when initialize failed
*
* ERRNO: N/A
*/

LOCAL STATUS wdmaComponentRegister
    (
    VXB_DEV_ID  pDev,
    struct abox_dma_data * data
    )
    {
    char                * name = NULL;
    char                * streamName = NULL;
    VX_SND_SOC_CMPT_DRV * cmpntDrv = NULL;

    data->dai_drv = (VX_SND_SOC_DAI_DRIVER *)vxbMemAlloc(sizeof(VX_SND_SOC_DAI_DRIVER));
    if (data->dai_drv == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    memcpy(data->dai_drv, &aboxWdmaDaiDrv, sizeof(VX_SND_SOC_DAI_DRIVER));

    data->dai_drv->id = data->id;

    name = (char *)vxbMemAlloc(MAX_ABOX_DMA_DAI_NAME_LEN);
    streamName = (char *)vxbMemAlloc(MAX_PCM_STREAM_NAME_LEN);
    if (name == NULL || streamName == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    (void) snprintf_s(name, MAX_ABOX_DMA_DAI_NAME_LEN,
                      aboxWdmaDaiDrv.name, data->id);
    (void) snprintf_s(streamName, MAX_PCM_STREAM_NAME_LEN,
                      aboxWdmaDaiDrv.capture.streamName, data->id);
    data->dai_drv->name = name;
    data->dai_drv->capture.streamName = streamName;
    DBG_MSG(DBG_INFO, "dai id %d name %s, streamName %s.\n", data->dai_drv->id,
            data->dai_drv->name, data->dai_drv->capture.streamName);

     cmpntDrv = (VX_SND_SOC_CMPT_DRV *)vxbMemAlloc(sizeof(VX_SND_SOC_CMPT_DRV));
     if (cmpntDrv == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    memcpy(cmpntDrv, &aboxWdmaBase, sizeof(aboxWdmaBase));

    if (wdmaRouteValidate(pDev) < 0)
        {
        DBG_MSG (DBG_ERR,"Validation check is failed.\n");
        return ERROR;
        }

    if (wdmaControlsMake (pDev, data, cmpntDrv) != OK)
        {
        DBG_MSG(DBG_ERR, "Making controls failed.\n");
        return ERROR;
        }

    if (vxSndCpntRegister(pDev, cmpntDrv, data->dai_drv, 1) != OK)
        {
        DBG_MSG(DBG_ERR, "%s: component register failed:%\n");
        return ERROR;
        }

    return OK;

errOut:
    if (data->dai_drv != NULL)
        {
        vxbMemFree(data->dai_drv);
        }
    if (name != NULL)
        {
        vxbMemFree(name);
        }
    if (streamName != NULL)
        {
        vxbMemFree(streamName);
        }
    if (cmpntDrv != NULL)
        {
        vxbMemFree(cmpntDrv);
        }
    return ERROR;

    }

/*******************************************************************************
*
* fdtAboxRdmaAttach - attach Samsung abox wdma
*
* This is the Samsung abox wdma initialization routine.
*
* RETURNS: OK or ERROR when initialize failed
*
* ERRNO: N/A
*/

LOCAL STATUS fdtAboxWdmaAttach
    (
    VXB_DEV_ID  pDev
    )
    {
    ABOX_DMA_CTRL                * pDevCtrl = NULL;
    VXB_FDT_DEV                  * pFdtDev = NULL;
    struct abox_dma_data         * data = NULL;
    VXB_RESOURCE                 * pMemRegs = NULL;
    VXB_RESOURCE_ADR             * pRegsAddr = NULL;
    STATUS                         ret = ERROR;
    int                            len;
    uint32_t                       * prop;

    DBG_MSG (DBG_INFO, "Start\n");


    pFdtDev = vxbFdtDevGet (pDev);
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "no FDT device.\n");
        goto errOut;
        }

    pDevCtrl = (ABOX_DMA_CTRL *)vxbMemAlloc(sizeof(ABOX_DMA_CTRL));
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    pDevCtrl->pdev = pDev;

    data = (struct abox_dma_data *)vxbMemAlloc(sizeof(struct abox_dma_data));
    if (data == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    pDevCtrl->pDmaData = data;
    vxbDevSoftcSet(pDev, pDevCtrl);

    SPIN_LOCK_ISR_INIT(&pDevCtrl->spinlockIsr, 0);

    data->pdev = pDev;
    data->stream_type = SNDRV_PCM_STREAM_CAPTURE;

    /* get sfr resource */

    pMemRegs = vxbResourceAlloc(pDev, VXB_RES_MEMORY, 0);
    if (pMemRegs == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    pRegsAddr = (VXB_RESOURCE_ADR *)(pMemRegs->pRes);
    data->sfr_base = (void *)(pRegsAddr->virtAddr);
    pDevCtrl->handle = pRegsAddr->pHandle;
    DBG_MSG(DBG_INFO, "sfr_base address = %llx\n", data->sfr_base);

    data->pdev_abox = vxbDevParent(pDev);
    if (data->pdev_abox == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to get abox device.\n");
        goto errOut;
        }

    /* need to check abox_data from abox drv */
    data->abox_data = (ABOX_CTR_DATA *)vxbDevSoftcGet(data->pdev_abox);
    if (data->abox_data == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to get abox data.\n");
        goto errOut;
        }

    prop = (uint32_t *)vxFdtPropGet(pFdtDev->offset, "samsung,id", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        DBG_MSG (DBG_ERR, "failed to get samsung id.\n");
        goto errOut;
        }
    data->id = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "id = %d\n", data->id);

    prop = (uint32_t *)vxFdtPropGet(pFdtDev->offset, "samsung,type", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        DBG_MSG (DBG_ERR, "failed to get samsung type.\n");
        goto errOut;
        }
    data->type = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "type = %d.\n", data->type);

    ret = abox_register_wdma(data->pdev_abox, data->pdev, data->id);
    if (ret == ERROR)
        {
        DBG_MSG (DBG_ERR, "failed to register RDMA pdev.\n");
        goto errOut;
        }

    ret = wdmaComponentRegister(pDev, data);
    if (ret == ERROR)
        {
        DBG_MSG(DBG_ERR, "abox wdma failed to register component:%d\n", ret);
        goto errOut;
        }

    return OK;

errOut:
    if (pDevCtrl != NULL)
        {
        vxbMemFree(pDevCtrl);
        }
    if (data != NULL)
        {
        vxbMemFree(data);
        }
    if (pMemRegs != NULL)
        {
        vxbResourceFree(pDev, pMemRegs);
        }
    return ERROR;
    }
