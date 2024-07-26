
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/* vxbFdtAboxRdma.c - Samsung Abox Rdma driver */

/*
DESCRIPTION

This is the VxBus driver for Samsung Abox Rdma.
*/

/* includes */

#include <vxWorks.h>
#include <audioLibCore.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/util/vxbAccess.h>
#include <vxbFdtAboxDma.h>


/* debug macro */

#undef ABOX_RDMA_DEBUG
#define ABOX_RDMA_DEBUG
#ifdef ABOX_RDMA_DEBUG
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
#endif  /* ABOX_RDMA_DEBUG */
#undef ABOX_RDMA_DEBUG

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

LOCAL STATUS fdtAboxRdmaProbe (VXB_DEV_ID pDev);
LOCAL STATUS fdtAboxRdmaAttach (VXB_DEV_ID pDev);
LOCAL STATUS rdmaComponentRegister(VXB_DEV_ID pDev, struct abox_dma_data * data);
LOCAL STATUS rdmaSifsFlush (VX_SND_SOC_COMPONENT *cmpnt);

LOCAL STATUS rdmaControlsMake(VXB_DEV_ID pDev, struct abox_dma_data *data,
                              VX_SND_SOC_CMPT_DRV * cmpnt);
LOCAL STATUS rdmaComponentProbe(VX_SND_SOC_COMPONENT * component);
LOCAL void rdmaComponentRemove(VX_SND_SOC_COMPONENT * component);
LOCAL uint32_t rdmaRegRead(ABOX_DMA_CTRL  * pDevCtrl,void  * regBase,
                           uint32_t  offset);
LOCAL void rdmaRegWrite(ABOX_DMA_CTRL  * pDevCtrl, void  * regBase,
                        uint32_t  offset, uint32_t  value);
LOCAL STATUS rdmaComponentPrepare (struct vxSndSocComponent * component,
                                   SND_PCM_SUBSTREAM * substream);
LOCAL STATUS rdmaComponentClose (struct vxSndSocComponent * component,
                                   SND_PCM_SUBSTREAM * substream);
LOCAL uint32_t rdmaCmpntRead(VX_SND_SOC_COMPONENT * component, uint32_t reg);
LOCAL STATUS rdmaCmpntWrite(VX_SND_SOC_COMPONENT * component, uint32_t reg,
                            uint32_t val);

IMPORT int aboxAsrcSpusMaxGet(void);
IMPORT const char * const *aboxAsrcSpusTextsGet(void);
IMPORT int aboxExpandSpusTextsNumGet(void);
IMPORT const char * const *aboxExpandSpusTextsGet(void);


/* locals */

LOCAL VX_SND_SOC_DAI_DRIVER abox_rdma_dai_drv =
    {
    .name = "RDMA%d",
    .playback =
        {
        .streamName = "RDMA%d Interface",
        .channelsMin = 1,
        .channelsMax = 32,
        .rates = ABOX_SAMPLING_RATES,
        .rateMin = 8000,
        .rateMax = 384000,
        .formats = ABOX_SAMPLE_FORMATS,
        },
    };

LOCAL VX_SND_SOC_CMPT_DRV abox_rdma_base =
    {
    .probe = rdmaComponentProbe,
    .remove = rdmaComponentRemove,
    .probe_order = SND_SOC_COMP_ORDER_FIRST,
    .prepare = rdmaComponentPrepare,
    .close = rdmaComponentClose,
    .read = rdmaCmpntRead,
    .write = rdmaCmpntWrite,
    };


LOCAL VXB_DRV_METHOD    fdtAboxRdmaMethodList[] =
    {
    { VXB_DEVMETHOD_CALL(vxbDevProbe), fdtAboxRdmaProbe},
    { VXB_DEVMETHOD_CALL(vxbDevAttach), fdtAboxRdmaAttach},
    { 0, NULL }
    };

LOCAL const VXB_FDT_DEV_MATCH_ENTRY aboxRdmaMatch[] =
    {
        {
        ABOX_RDMA_DRIVER_NAME,               /* compatible */
        (void *)NULL,
        },
        {}                                      /* Empty terminated list */
    };

LOCAL char expandSpusTexts[][ENUM_CTRL_STR_SIZE] = {"ASRC Route"};

LOCAL char asrcSpusTexts[][ENUM_CTRL_STR_SIZE] =
    {
    "SIFS",                                 /* SIFS */
    };



/* globals */

/* Samsung Abox rdma driver */

VXB_DRV vxbFdtAboxRdmaDrv =
    {
    {NULL},
    "aboxRdma",                                 /* Name */
    "Abox Rdma Fdt driver",                     /* Description */
    VXB_BUSID_FDT,                              /* Class */
    0,                                          /* Flags */
    0,                                          /* Reference count */
    fdtAboxRdmaMethodList,                      /* Method table */
    };

VXB_DRV_DEF(vxbFdtAboxRdmaDrv);

/* functions */

inline const char * const *aboxExpandSpusTextsGet(void)
    {
    static const char * const expandTexts[] =
        {
        expandSpusTexts[0]
        };
    return expandTexts;
    }

inline int aboxExpandSpusTextsNumGet(void)
    {
    return ELEMENTS(expandSpusTexts);
    }

inline const char * const *aboxAsrcSpusTextsGet(void)
    {
    static const char * const asrcTexts[] =
        {
        asrcSpusTexts[0]
        };
    return asrcTexts;
    }

inline int aboxAsrcSpusMaxGet(void)
    {
    return ELEMENTS(asrcSpusTexts);
    }


/*******************************************************************************
*
* fdtAboxRdmaProbe - probe for device presence at specific address
*
* Check for Samsung Abox rdma device at the specified base address.
* We assume one is present at that address, but we need to verify.
*
* RETURNS: OK if probe passes and assumed a valid Samsung Abox rdma device.
* ERROR otherwise.
*
* ERRNO: N/A
*/

LOCAL STATUS fdtAboxRdmaProbe
    (
    VXB_DEV_ID  pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxRdmaMatch, NULL);
    }

uint32_t rdmaSifsCntValSet
    (
    struct abox_dma_data * data,
    uint32_t               val
    )
    {
    VXB_DEV_ID       pDev = data->pdev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    val &= ABOX_RDMA_SIFS_CNT_VAL_MASK;
    DBG_MSG(DBG_INFO, "sfr base: 0x%x, offset: 0x%x, val: 0x%x\n",
            data->sfr_base, ABOX_RDMA_SIFS_CNT_VAL, val);

    pDevCtrl = vxbDevSoftcGet(pDev);
    rdmaRegWrite(pDevCtrl, data->sfr_base, ABOX_RDMA_SIFS_CNT_VAL, val);

    return 0;
    }

/*******************************************************************************
*
* aboxRdmaBufferStatusGet - get dma buffer offset and buffer cnt
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

uint32_t aboxRdmaBufferStatusGet
    (
    struct abox_dma_data * data
    )
    {
    unsigned int     sfr_val = 0;
    unsigned int     rdma_buffer_status = 0;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;
    VXB_DEV_ID       pDev = data->pdev;
    uint32_t         rdmaStuMsk = (ABOX_RDMA_RBUF_STATUS_MASK|
                                   ABOX_RDMA_RBUF_CNT_MASK);

    pDevCtrl = vxbDevSoftcGet (pDev);
    sfr_val = rdmaRegRead (pDevCtrl, data->sfr_base, ABOX_RDMA_STATUS);

    rdma_buffer_status = (sfr_val & rdmaStuMsk);

    return rdma_buffer_status;
    }

uint32_t aboxRdmaStatusGet
    (
    struct abox_dma_data * data
    )
    {
    unsigned int     sfr_val = 0;
    unsigned int     rdma_status = 0;
    VXB_DEV_ID       pDev = data->pdev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    sfr_val = rdmaRegRead(pDevCtrl, data->sfr_base, ABOX_RDMA_STATUS);

    rdma_status = sfr_val >> ABOX_RDMA_PROGRESS_L;

    return rdma_status;
    }

uint32_t aboxRdmaIsEnabled
    (
    struct abox_dma_data * data
    )
    {
    unsigned int value = 0;
    VXB_DEV_ID pDev = data->pdev;
    ABOX_DMA_CTRL  * pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    value = rdmaRegRead(pDevCtrl, data->sfr_base, ABOX_RDMA_CTRL0);
    value &= 1;

    return value;
    }

int aboxRdmaIpcRequest
    (
    struct abox_dma_data * data,
    ABOX_IPC_MSG *         msg,
    int                    atomic,
    int                    sync
    )
    {
    VXB_DEV_ID pDevAbox = data->pdev_abox;
    return abox_request_ipc(pDevAbox, msg->ipcid, msg, sizeof(*msg),
                            atomic, sync);
    }

/*******************************************************************************
*
* __abox_rdma_flush_sifs - Flush audio data which is in SIFS
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

int __abox_rdma_flush_sifs
    (
    struct abox_dma_data * data
    )
    {
    uint32_t        regValue;
    VXB_DEV_ID      pDev = data->pdev;
    ABOX_DMA_CTRL * pDevCtrl = NULL;

    /* write the value to ABOX_RDMA_SPUS_CTRL1 */

    pDevCtrl = vxbDevSoftcGet(pDev);
    regValue = rdmaRegRead(pDevCtrl, data->sfr_base, ABOX_RDMA_SPUS_CTRL1);
    regValue |= ABOX_RDMA_SPUS_CTRL1_FLUSH;

    rdmaRegWrite(pDevCtrl, data->sfr_base, ABOX_RDMA_SPUS_CTRL1, regValue);

    return 0;
    }

LOCAL STATUS rdmaComponentPrepare
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    )
    {
    return rdmaSifsFlush (component);
    }

LOCAL STATUS rdmaComponentClose
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    )
    {
    return rdmaSifsFlush (component);
    }

/*******************************************************************************
*
* rdmaSifsFlush - Flush audio data which is in SIFS
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/


LOCAL STATUS rdmaSifsFlush
    (
    VX_SND_SOC_COMPONENT * cmpnt
    )
    {
    VXB_DEV_ID             pDev     = cmpnt->pDev;
    ABOX_DMA_CTRL *        pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet (pDev);
    struct abox_dma_data * data     = pDevCtrl->pDmaData;

    (void) __abox_rdma_flush_sifs (data);

    return OK;
    }

/*******************************************************************************
*
* rdmaComponentProbe - RDMA component probe function called during the
*                      initlization
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

LOCAL STATUS rdmaComponentProbe
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID             pdev = component->pDev;
    ABOX_DMA_CTRL *        pDevCtrl = (ABOX_DMA_CTRL *)vxbDevSoftcGet(pdev);
    struct abox_dma_data * data = pDevCtrl->pDmaData;

    data->cmpnt = component;

    return OK;
    }

LOCAL uint32_t rdmaRegRead
    (
    ABOX_DMA_CTRL *  pDevCtrl,
    void *           regBase,
    uint32_t         offset
    )
    {
    uint32_t  regValue;

    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    regValue = REG_READ_4(pDevCtrl->handle, regBase, offset);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);

    return regValue;
    }

LOCAL uint32_t rdmaCmpntRead
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg
    )
    {
    struct abox_dma_data * data = NULL;
    VXB_DEV_ID             pDev = component->pDev;
    ABOX_DMA_CTRL *        pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    data = (struct abox_dma_data *) pDevCtrl->pDmaData;

    return rdmaRegRead(pDevCtrl, data->sfr_base, reg);
    }

LOCAL void rdmaRegWrite
    (
    ABOX_DMA_CTRL *  pDevCtrl,
    void *           regBase,
    uint32_t         offset,
    uint32_t         value
    )
    {
    SPIN_LOCK_ISR_TAKE(&pDevCtrl->spinlockIsr);

    REG_WRITE_4(pDevCtrl->handle, regBase, offset, value);

    SPIN_LOCK_ISR_GIVE(&pDevCtrl->spinlockIsr);
    }

LOCAL STATUS rdmaCmpntWrite
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

    rdmaRegWrite(pDevCtrl, data->sfr_base, reg, val);

    return OK;
    }

/*******************************************************************************
*
* rdmaComponentRemove - Remove RDMA component
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

LOCAL void rdmaComponentRemove
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    }

/*******************************************************************************
*
* abox_rdma_disable - Disable RDMA
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

void aboxRdmaDisable
    (
    struct abox_dma_data * data
    )
    {
    uint32_t         regValue = 0;
    VXB_DEV_ID       pDev = data->pdev;
    ABOX_DMA_CTRL *  pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    regValue = rdmaRegRead(pDevCtrl, data->sfr_base, ABOX_RDMA_CTRL0);

    regValue &= ~ABOX_RDMA_CTRL0_ENABLE;

    rdmaRegWrite(pDevCtrl, data->sfr_base, ABOX_RDMA_CTRL0, regValue);
    }

/*******************************************************************************
*
* abox_rdma_enable - enable RDMA
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

void aboxRdmaEnable
    (
    struct abox_dma_data * data
    )
    {
    uint32_t         regValue = 0;
    VXB_DEV_ID       pDev = data->pdev;
    ABOX_DMA_CTRL *  pDevCtrl = NULL;

    pDevCtrl = vxbDevSoftcGet(pDev);
    regValue = rdmaRegRead(pDevCtrl, data->sfr_base, ABOX_RDMA_CTRL0);

    regValue |= ABOX_RDMA_CTRL0_ENABLE;

    rdmaRegWrite(pDevCtrl, data->sfr_base, ABOX_RDMA_CTRL0, regValue);

    }

LOCAL STATUS rdmaAsrcRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data *  data;
    VX_SND_SOC_COMPONENT *  cmpt;
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL *         pDevCtrl;

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

    pDevCtrl = (ABOX_DMA_CTRL *)vxbDevSoftcGet(pDev);
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

    ucontrol->value.enumerated.item[0] = 0;
    return OK;
    }
#define ABOX_RDMA_DEBUG
LOCAL STATUS rdmaAsrcRouteSet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data *  data;
    VX_SND_SOC_COMPONENT *  cmpt;
    VXB_DEV_ID              pDev;
    ABOX_DMA_CTRL        *  pDevCtrl;
    unsigned int            item = ucontrol->value.enumerated.item[0];

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
    pDevCtrl = (ABOX_DMA_CTRL *)vxbDevSoftcGet(pDev);
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

    VXB_DEV_ID               pDevAbox;
    ABOX_IPC_MSG             msg = {0, };
    IPC_ABOX_CONFIG_MSG    * abox_config_msg = &msg.msg.config;

    if (item != 0)
        {
        DBG_MSG(DBG_ERR, "The item of value should be zero!\n");
        return ERROR;
        }

    rdmaRegWrite(pDevCtrl, data->sfr_base, ABOX_RDMA_SPUS_CTRL0, 0);

    pDevAbox = vxbDevParent(pDev);
    if (pDevAbox == NULL)
        {
        DBG_MSG(DBG_ERR, "Can't find device abox.\n");
        return ERROR;
        }
    msg.ipcid = IPC_ABOX_CONFIG;
    abox_config_msg->msgtype = ASRC_HW_INFO;
    abox_config_msg->param.asrc_hw_info.enable = FALSE;
    abox_request_ipc(pDevAbox, msg.ipcid, &msg, sizeof(msg), 0, 0);

    return OK;
    }

LOCAL STATUS aboxRdmaExpandGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    return snd_soc_get_enum_double (kcontrol, ucontrol);
    }

LOCAL STATUS aboxRdmaExpandSet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    struct abox_dma_data * dmaData;
    VX_SND_SOC_COMPONENT * cmpt;
    VXB_DEV_ID             pDev;
    ABOX_DMA_CTRL *        pDevCtrl;

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

    pDevCtrl = (ABOX_DMA_CTRL *)vxbDevSoftcGet(pDev);
    if (pDevCtrl == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find pDevCtrl\n");
        return ERROR;
        }

    dmaData = (struct abox_dma_data *) pDevCtrl->pDmaData;
    if (dmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot find struct abox_dma_data from kcontrol %s",
                 kcontrol->id.name);
        return ERROR;
        }

    return snd_soc_put_enum_double (kcontrol, ucontrol);
    }

/*******************************************************************************
*
* rdmaControlsMake - Make RDMA controls
*
*
*
* RETURNS:
* ERROR otherwise.
*
* ERRNO:
*/

LOCAL STATUS rdmaControlsMake
    (
    VXB_DEV_ID  pDev,
    struct abox_dma_data *data,
    VX_SND_SOC_CMPT_DRV * cmpnt
    )
    {
    const char * const  * asrc_texts             = aboxAsrcSpusTextsGet();
    int                   num_of_max_asrc        = aboxAsrcSpusMaxGet();
    const char * const  * expand_texts           = aboxExpandSpusTextsGet();
    int                   num_of_expand_texts    = aboxExpandSpusTextsNumGet();
    VX_SND_CONTROL      * rdma_controls          = NULL;
    VX_SND_ENUM         * rdma_route_enum        = NULL;
    size_t                size;
    uint32_t              idx;
    VX_SND_ENUM           rdma_route_enum_base[] =
        {
        SOC_ENUM_DOUBLE (ABOX_RDMA_CTRL0, ABOX_RDMA_EXPEND_L,
                         ABOX_RDMA_EXPEND_H, num_of_expand_texts, expand_texts),
        SOC_ENUM_DOUBLE (SND_SOC_NOPM, 0, 0, num_of_max_asrc, asrc_texts),
        };
    const VX_SND_CONTROL rdma_controls_base []   =
        {
        SOC_ENUM_EXT ("RDMA%d Expand Route", rdma_route_enum_base[0],
                      aboxRdmaExpandGet, aboxRdmaExpandSet),
        SOC_ENUM_EXT ("RDMA%d ASRC Route", rdma_route_enum_base[1],
                      rdmaAsrcRouteGet, rdmaAsrcRouteSet),
        };

    /* allocate enums */

    size = sizeof (VX_SND_ENUM) * ELEMENTS (rdma_route_enum_base);
    rdma_route_enum = (VX_SND_ENUM *) vxbMemAlloc (size);
    if (rdma_route_enum == NULL)
        {
        goto errOut;
        }
    (void) memcpy_s (rdma_route_enum, size, rdma_route_enum_base, size);

    /* allocate kcontrols */

    size = sizeof (VX_SND_CONTROL) * ELEMENTS (rdma_controls_base);
    rdma_controls = (VX_SND_CONTROL *) vxbMemAlloc (size);
    if (rdma_controls == NULL )
        {
        goto errOut;
        }
    (void) memcpy_s (rdma_controls, size, rdma_controls_base, size);

    for (idx = 0; idx < ELEMENTS (rdma_controls_base); idx++)
        {
        rdma_controls[idx].privateValue = (unsigned long) &rdma_route_enum[idx];
        rdma_controls[idx].id.name = vxbMemAlloc (SND_DEV_NAME_LEN);
        (void) snprintf_s (rdma_controls[idx].id.name, SND_DEV_NAME_LEN,
                           rdma_controls_base[idx].id.name, data->dai_drv->id);
        }

    cmpnt->controls     = rdma_controls;
    cmpnt->num_controls = ELEMENTS (rdma_route_enum_base);

    return OK;

errOut:
    if (rdma_controls != NULL)
        {
        for (idx = 0; idx < ELEMENTS (rdma_controls_base); idx++)
            {
            if (rdma_controls[idx].id.name != NULL)
                {
                vxbMemFree (rdma_controls[idx].id.name);
                }
            }
        vxbMemFree (rdma_controls);
        }
    if (rdma_route_enum != NULL)
        {
        vxbMemFree (rdma_route_enum);
        }

    return ERROR;
    }

/*******************************************************************************
*
* aboxRdmaComponentRegister -
*
*
*
* RETURNS: OK if probe passes and assumed a valid Samsung Abox rdma device.
* ERROR otherwise.
*
* ERRNO: N/A
*/

LOCAL STATUS rdmaComponentRegister
    (
    VXB_DEV_ID             pDev,
    struct abox_dma_data * data
    )
    {
    VX_SND_SOC_CMPT_DRV * cmpntDrv = NULL;
    STATUS                ret;
    char *                name = NULL;
    char *                streamName = NULL;

    data->dai_drv = (VX_SND_SOC_DAI_DRIVER *)vxbMemAlloc(sizeof(VX_SND_SOC_DAI_DRIVER));
    if (data->dai_drv == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto err;
        }
    memcpy(data->dai_drv, &abox_rdma_dai_drv, sizeof(abox_rdma_dai_drv));

    data->dai_drv->id = data->id;

    name = (char *)vxbMemAlloc(MAX_ABOX_DMA_DAI_NAME_LEN);
    streamName = (char *)vxbMemAlloc(MAX_PCM_STREAM_NAME_LEN);
    if (name == NULL || streamName == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto err;
        }

    (void) snprintf_s(name, MAX_ABOX_DMA_DAI_NAME_LEN,
                      abox_rdma_dai_drv.name, data->id);
    data->dai_drv->name = (const char *)name;

    (void) snprintf_s(streamName, MAX_PCM_STREAM_NAME_LEN,
                      abox_rdma_dai_drv.playback.streamName, data->id);
    data->dai_drv->playback.streamName = (const char *)streamName;

    DBG_MSG(DBG_INFO, "dai id: %d, dai name %s, streamName %s.\n",
            data->dai_drv->id, data->dai_drv->name,
            data->dai_drv->playback.streamName);

    cmpntDrv = (VX_SND_SOC_CMPT_DRV *)vxbMemAlloc(sizeof(VX_SND_SOC_CMPT_DRV));
     if (cmpntDrv == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto err;
        }
    memcpy(cmpntDrv, &abox_rdma_base, sizeof(abox_rdma_base));

    if (rdmaControlsMake(pDev, data, cmpntDrv) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to make RDMA controls\n");
        goto err;
        }

    ret = vxSndCpntRegister(pDev, cmpntDrv, data->dai_drv, 1);
    if (ret == ERROR)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto err;
        }

    return OK;

err:
    if (cmpntDrv)
        {
        vxbMemFree(cmpntDrv);
        }
    if (streamName)
        {
        vxbMemFree(streamName);
        }
    if (name)
        {
        vxbMemFree(name);
        }
    if (data->dai_drv)
        {
        vxbMemFree(data->dai_drv);
        }
    return ERROR;
    }

/*******************************************************************************
*
* fdtAboxRdmaAttach - attach Samsung abox rdma
*
* This is the Samsung abox rdma initialization routine.
*
* RETURNS: OK or ERROR when initialize failed
*
* ERRNO: N/A
*/

LOCAL STATUS fdtAboxRdmaAttach
    (
    VXB_DEV_ID  pDev
    )
    {
    VXB_FDT_DEV                  * pFdtDev = NULL;
    VXB_RESOURCE                 * pMemRegs = NULL;
    VXB_RESOURCE_ADR             * pRegsAddr = NULL;
    ABOX_DMA_CTRL                * pDevCtrl = NULL;
    struct abox_dma_data         * pDmaData = NULL;
    int                            len;
    uint32_t                     * prop = NULL;
    STATUS                         ret = OK;

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

    pDmaData = (struct abox_dma_data *)vxbMemAlloc(sizeof(struct abox_dma_data));
    if (pDmaData == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }
    pDevCtrl->pDmaData = pDmaData;
    pDmaData->pdev = pDev;
    vxbDevSoftcSet(pDev, pDevCtrl);

    pDmaData->stream_type = SNDRV_PCM_STREAM_PLAYBACK;
    pDmaData->pdev_abox = vxbDevParent(pDev);
    if (pDmaData->pdev_abox == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to get abox device.\n");
        goto errOut;
        }

    /* get sfr resource */

    pMemRegs = vxbResourceAlloc(pDev, VXB_RES_MEMORY, 0);
    if (pMemRegs == NULL)
        {
        DBG_MSG (DBG_ERR, "Error!\n");
        goto errOut;
        }

    pRegsAddr = (VXB_RESOURCE_ADR *)(pMemRegs->pRes);
    pDevCtrl->handle = (void *)(pRegsAddr->pHandle);
    pDmaData->sfr_base = (void *)(pRegsAddr->virtAddr);
    DBG_MSG(DBG_INFO, "sfr_base address = %llx\n", pDmaData->sfr_base);
    DBG_MSG(DBG_INFO, "sfr_base phy address = %x\n", (pRegsAddr->start));

    SPIN_LOCK_ISR_INIT(&pDevCtrl->spinlockIsr, 0);

    /* need to check abox_data from abox drv */
    pDmaData->abox_data = (ABOX_CTR_DATA *)vxbDevSoftcGet(pDmaData->pdev_abox);
    if (pDmaData->abox_data == NULL)
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
    pDmaData->id = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "id = %d\n", pDmaData->id);

    prop = (uint32_t *)vxFdtPropGet(pFdtDev->offset, "samsung,type", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        DBG_MSG (DBG_ERR, "failed to get samsung type.\n");
        goto errOut;
        }
    pDmaData->type = vxFdt32ToCpu(*prop);
    DBG_MSG(DBG_INFO, "type = %d.\n", pDmaData->type);

    ret = abox_register_rdma(pDmaData->pdev_abox, pDmaData->pdev, pDmaData->id);
    if (ret == ERROR)
        {
        DBG_MSG (DBG_ERR, "failed to register RDMA pdev.\n");
        goto errOut;
        }

    ret = rdmaComponentRegister(pDev, pDmaData);
    if (ret == ERROR)
        {
        DBG_MSG (DBG_ERR, "abox rdma failed to register component.\n");
        goto errOut;
        }
    DBG_MSG (DBG_INFO, "End\n");
    return OK;

errOut:
    if (pDmaData)
        {
        vxbMemFree(pDmaData);
        }

    if (pDevCtrl)
        {
        vxbMemFree(pDevCtrl);
        }
    if (pMemRegs)
        {
        vxbResourceFree(pDev, pMemRegs);
        }
    DBG_MSG (DBG_ERR, "%s error.\n", __func__);
    return ERROR;
    }

