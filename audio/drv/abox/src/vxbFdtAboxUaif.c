
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

This is the driver for the Abox Unified Audio Interface(UAIF) modules
as unified audio interface on V920.
*/

#include <vxWorks.h>
#include <subsys/clk/vxbClkLib.h>
#include <subsys/pinmux/vxbPinMuxLib.h>
#include <vxbFdtAboxDma.h>
#include <vxbFdtAboxUaif.h>

#undef ABOX_UAIF_DEBUG
#ifdef ABOX_UAIF_DEBUG
#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             (0x00000000)
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             (0xffffffff)

LOCAL uint32_t uaifDbgMask = DBG_ALL;
#undef  UAIF_DBG
#define UAIF_DBG(mask, ...)                                             \
    do                                                                  \
        {                                                               \
        if ((uaifDbgMask & (mask)) || ((mask) == DBG_ALL))          \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef  UAIF_DBG
#define UAIF_DBG(...)
#endif  /* ABOX_UAIF_DEBUG */

/* defines */

#define CTRL_STR_SIZE   32

/* register read and write interface */

#define REG_READ_4(pData, reg)             \
        vxbRead32((pData)->regHandle, (uint32_t *)((pData)->regBase + (reg)))

#define REG_WRITE_4(pData, reg, data)      \
        vxbWrite32((pData)->regHandle,     \
                   (uint32_t *)((pData)->regBase + (reg)), (data))

/* forward declarations */

LOCAL STATUS aboxUaifProbe (VXB_DEV_ID pDev);
LOCAL STATUS aboxUaifAttach (VXB_DEV_ID pDev);
LOCAL STATUS aboxUaifFmtSet (VX_SND_SOC_DAI * dai, uint32_t fmt);
LOCAL STATUS aboxUaifTristateSet (VX_SND_SOC_DAI * dai, int32_t tristate);
LOCAL STATUS aboxUaifStartup (SND_PCM_SUBSTREAM * substream,
                              VX_SND_SOC_DAI * dai);
LOCAL void aboxUaifShutdown (SND_PCM_SUBSTREAM * substream,
                             VX_SND_SOC_DAI * dai);
LOCAL STATUS aboxUaifHwParams (SND_PCM_SUBSTREAM * substream,
                               VX_SND_PCM_HW_PARAMS * hw_params,
                               VX_SND_SOC_DAI * dai);
LOCAL STATUS aboxUaifHwFree (SND_PCM_SUBSTREAM * substream,
                             VX_SND_SOC_DAI * dai);
LOCAL STATUS aboxUaifTrigger (SND_PCM_SUBSTREAM * substream,
                              int32_t trigger, VX_SND_SOC_DAI * dai);
LOCAL STATUS aboxUaifCmpntProbe (VX_SND_SOC_COMPONENT * cmpnt);
LOCAL void aboxUaifCmpntRemove (VX_SND_SOC_COMPONENT * cmpnt);
LOCAL uint32_t aboxUaifCmpntRead (VX_SND_SOC_COMPONENT * cmpnt,
                                  uint32_t regOfs);
LOCAL int32_t aboxUaifCmpntWrite (VX_SND_SOC_COMPONENT * cmpnt,
                                  uint32_t regOfs, uint32_t val);

/* locals */

//uaif_spkx_texts
LOCAL char aboxUaifSIFS[][CTRL_STR_SIZE] =
    {
    "NO_CONNECT",               /* NO_CONNECT */
    "NO_CONNECT", "NO_CONNECT", /* SIFS0 SIFS1 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS2 SIFS3 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS4 SIFS5 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS6 SIFS7 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS8 SIFS9 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS10 SIFS11 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS12 SIFS13 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS14 SIFS15 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS16 SIFS17 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS18 SIFS19 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS20 SIFS21 */
    "NO_CONNECT", "NO_CONNECT", /* SIFS22 SIFS23 */
    };

/* DAI */

//abox_uaif_dai_ops abox_uaif_set_mux abox_uaif_is_enabled
LOCAL VX_SND_SOC_DAI_OPS aboxUaifDaiOps =
    {
    .setFmt         = aboxUaifFmtSet,
    .set_tristate   = aboxUaifTristateSet,
    .startup        = aboxUaifStartup,
    .shutdown       = aboxUaifShutdown,
    .hwParams       = aboxUaifHwParams,
    .hwFree         = aboxUaifHwFree,
    .trigger        = aboxUaifTrigger,
    };

//abox_uaif_dai_drv
LOCAL VX_SND_SOC_DAI_DRIVER aboxUaifDaiDrv =
    {
    .name = "UAIF%d",
    .playback =
        {
        .streamName = "UAIF%d Playback",
        .channelsMin = 1,
        .channelsMax = 32,
        .rates = ABOX_SAMPLING_RATES,
        .rateMin = 8000,
        .rateMax = 384000,
        .formats = ABOX_SAMPLE_FORMATS,
        },
    .capture =
        {
        .streamName = "UAIF%d Capture",
        .channelsMin = 1,
        .channelsMax = 32,
        .rates = ABOX_SAMPLING_RATES,
        .rateMin = 8000,
        .rateMax = 384000,
        .formats = ABOX_SAMPLE_FORMATS,
        },
    .ops = &aboxUaifDaiOps,
    };

/* component */

//abox_uaif_cmpnt
LOCAL VX_SND_SOC_CMPT_DRV aboxUaifCmpnt =
    {
    .name   = "samsung-uaif",
    .probe  = aboxUaifCmpntProbe,
    .remove = aboxUaifCmpntRemove,
    .probe_order = SND_SOC_COMP_ORDER_EARLY,
    .read   = aboxUaifCmpntRead,
    .write  = aboxUaifCmpntWrite,
    };

LOCAL VXB_DRV_METHOD aboxUaifMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxUaifProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxUaifAttach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxUaifMatch[] =
    {
        {
        "samsung,abox-uaif",
        NULL
        },
        {}                           /* empty terminated list */
    };

/* globals */

VXB_DRV aboxUaifDrv =
    {
    {NULL},
    ABOX_UAIF_DRV_NAME,              /* Name */
    "Abox Uaif FDT driver",          /* Description */
    VXB_BUSID_FDT,                   /* Class */
    0,                               /* Flags */
    0,                               /* Reference count */
    aboxUaifMethodList               /* Method table */
    };

VXB_DRV_DEF(aboxUaifDrv)


LOCAL STATUS aboxUaifProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxUaifMatch, NULL);
    }

LOCAL void aboxUaifRegmapRead
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    UINT64                      regOfs,
    uint32_t *                  pRegVal
    )
    {
    SPIN_LOCK_ISR_TAKE (&pUaifData->regmapSpinlockIsr);

    *pRegVal = REG_READ_4 (pUaifData, regOfs);

    SPIN_LOCK_ISR_GIVE (&pUaifData->regmapSpinlockIsr);
    }

LOCAL void aboxUaifRegmapWrite
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    UINT64                      regOfs,
    uint32_t                    regVal
    )
    {
    SPIN_LOCK_ISR_TAKE (&pUaifData->regmapSpinlockIsr);

    REG_WRITE_4 (pUaifData, regOfs, regVal);

    SPIN_LOCK_ISR_GIVE (&pUaifData->regmapSpinlockIsr);
    }

LOCAL void aboxUaifRegmapWriteBits
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    UINT64                      regOfs,
    uint32_t                    regVal,
    uint32_t                    regMask
    )
    {
    volatile uint32_t           value = 0;

    SPIN_LOCK_ISR_TAKE (&pUaifData->regmapSpinlockIsr);

    value = REG_READ_4 (pUaifData, regOfs);
    value = (value & ~regMask) | (regVal & regMask);
    REG_WRITE_4 (pUaifData, regOfs, value);

    SPIN_LOCK_ISR_GIVE (&pUaifData->regmapSpinlockIsr);
    }

LOCAL uint32_t aboxUaifCmpntRead
    (
    VX_SND_SOC_COMPONENT *      cmpnt,
    uint32_t                    regOfs
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    uint32_t                    regVal;

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (cmpnt->pDev);
    if (pUaifData == NULL)
        {
        return NULL;
        }

    aboxUaifRegmapRead (pUaifData, regOfs, &regVal);

    return regVal;
    }

LOCAL int32_t aboxUaifCmpntWrite
    (
    VX_SND_SOC_COMPONENT *      cmpnt,
    uint32_t                    regOfs,
    uint32_t                    val
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (cmpnt->pDev);
    if (pUaifData == NULL)
        {
        return -ENODATA;
        }

    aboxUaifRegmapWrite (pUaifData, regOfs, val);

    return 0;
    }

//abox_uaif_set_mode_pair
LOCAL void aboxUaifModePairSet
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    int32_t                     sd,
    int32_t                     route,
    int32_t                     value
    )
    {
    uint32_t                    modeValue = 0;
    uint32_t                    modeMask = 0;

    UAIF_DBG (DBG_INFO, "%s sd:%d route:%d, value:%x\n",
              __func__, sd, route, value);

    /*
     * UAIF 0/1:
     *   supports nTX + mRX mode.
     *   TX mode - 1st source is SD0, 2nd source is SD1,
     *             3rd is SD2, and 4th is SD3.
     *   RX mode - 1st source is SD3, 2nd source is SD2,
     *             3rd is SD1, and 4th is SD0.
     * UAIF 2/3/4/5/6/7/8/9:
     *   supports 2TX, 1TX + 1RX, or 2RX mode.
     *
     */
    if (route)
        {
        switch (sd)
            {
            case UAIF_SD3:
                modeValue = (value << ABOX_UAIF_SD3_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD2_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD1_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD0_MODE_OFFSET);

                modeMask = ABOX_UAIF_SD3_MODE_MASK |
                           ABOX_UAIF_SD2_MODE_MASK |
                           ABOX_UAIF_SD1_MODE_MASK |
                           ABOX_UAIF_SD0_MODE_MASK;
                break;
            case UAIF_SD2:
                modeValue = (value << ABOX_UAIF_SD2_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD1_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD0_MODE_OFFSET);

                modeMask = ABOX_UAIF_SD2_MODE_MASK |
                           ABOX_UAIF_SD1_MODE_MASK |
                           ABOX_UAIF_SD0_MODE_MASK;
                break;
            case UAIF_SD1:
                modeValue = (value << ABOX_UAIF_SD1_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD0_MODE_OFFSET);

                modeMask = ABOX_UAIF_SD1_MODE_MASK |
                           ABOX_UAIF_SD0_MODE_MASK;
                break;
            case UAIF_SD0:
                modeValue = value << ABOX_UAIF_SD0_MODE_OFFSET;
                modeMask = ABOX_UAIF_SD0_MODE_MASK;
                break;
            default:
                break;
            }
        }
    else
        {
        switch (sd)
            {
            case UAIF_SD0:
                modeValue = (value << ABOX_UAIF_SD0_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD1_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD2_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD3_MODE_OFFSET);
                modeMask = ABOX_UAIF_SD0_MODE_MASK |
                           ABOX_UAIF_SD1_MODE_MASK |
                           ABOX_UAIF_SD2_MODE_MASK |
                           ABOX_UAIF_SD3_MODE_MASK;
                break;
            case UAIF_SD1:
                modeValue = (value << ABOX_UAIF_SD1_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD2_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD3_MODE_OFFSET);
                modeMask = ABOX_UAIF_SD1_MODE_MASK |
                           ABOX_UAIF_SD2_MODE_MASK |
                           ABOX_UAIF_SD3_MODE_MASK;
                break;
            case UAIF_SD2:
                modeValue = (value << ABOX_UAIF_SD2_MODE_OFFSET) |
                            (value << ABOX_UAIF_SD3_MODE_OFFSET);
                modeMask = ABOX_UAIF_SD2_MODE_MASK |
                           ABOX_UAIF_SD3_MODE_MASK;
                break;
            case UAIF_SD3:
                modeValue = value << ABOX_UAIF_SD3_MODE_OFFSET;
                modeMask = ABOX_UAIF_SD3_MODE_MASK;
                break;
            default:
                break;
            }
        }

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1, modeValue, modeMask);

    UAIF_DBG (DBG_INFO, "%s modeMask:0x%08x modeValue:0x%08x, value:0x%08x\n",
              __func__, modeMask, modeValue, value);
    }

//abox_uaif_enable abox_uaif_enable_4line abox_uaif_enable_2line
LOCAL void aboxUaifEnable
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    int32_t                     stream,
    int32_t                     enable
    )
    {
    uint32_t                    ctrl1 = 0;
    uint32_t                    mask = 0;
    uint32_t                    value = 0;

    aboxUaifRegmapRead (pUaifData, ABOX_UAIF_CTRL1, &ctrl1);
    if (pUaifData->dataLine == UAIF_SD_MAX) /* enable 4 data lines */
        {
        if (stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
            if ((ctrl1 & ABOX_UAIF_SD3_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD3_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_3TX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_4TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_3TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_4TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD2_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD2_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_3TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_3TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid SPK mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        else
            {
            if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_3RX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_4RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_3RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_4RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_3RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_3RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD2_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD2_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD3_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD3_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid MIC mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        }
    else /* enable 2 data line2 */
        {
        if (stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
            if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        enable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_SPK_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid SPK mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        else
            {
            if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        enable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = enable << ABOX_UAIF_MIC_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid MIC mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        }

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL0, value, mask);
    UAIF_DBG (DBG_INFO, "%s stream:%d ctrl1:0x%08x mask:0x%08x value:0x%08x\n",
              __func__, stream, ctrl1, mask, value);
    }

//abox_uaif_disable abox_uaif_disable_4line abox_uaif_disable_2line
LOCAL void aboxUaifDisable
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    int32_t                     stream,
    int32_t                     disable
    )
    {
    uint32_t                    ctrl1 = 0;
    uint32_t                    mask = 0;
    uint32_t                    value = 0;

    aboxUaifRegmapRead (pUaifData, ABOX_UAIF_CTRL1, &ctrl1);
    if (pUaifData->dataLine == UAIF_SD_MAX) /* disable 4 data lines */
        {
        if (stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
            if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_3TX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_4TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_3TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_4TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                     (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_3TX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_4TX_ENABLE_OFFSET;
                mask =  ABOX_UAIF_SPK_2TX_ENABLE_MASK |
                        ABOX_UAIF_SPK_3TX_ENABLE_MASK |
                        ABOX_UAIF_SPK_4TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD2_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD2_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_3TX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_4TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_3TX_ENABLE_MASK |
                       ABOX_UAIF_SPK_4TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD3_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD3_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_4TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_4TX_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid SPK mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        else
            {
            if ((ctrl1 & ABOX_UAIF_SD3_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD3_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_3RX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_4RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_3RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_4RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD2_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD2_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_3RX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_4RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_2RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_3RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_4RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_3RX_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_4RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_3RX_ENABLE_MASK |
                       ABOX_UAIF_MIC_4RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_4RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_4RX_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid MIC mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        }
    else  /* disable 2 line */
        {
        if (stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
            if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_ENABLE_OFFSET |
                        disable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET;
                mask = ABOX_UAIF_SPK_ENABLE_MASK |
                       ABOX_UAIF_SPK_2TX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_SPK << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_SPK_2TX_ENABLE_OFFSET;
                mask =  ABOX_UAIF_SPK_2TX_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid SPK mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        else
            {
            if ((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD1_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_ENABLE_OFFSET |
                        disable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_ENABLE_MASK |
                       ABOX_UAIF_MIC_2RX_ENABLE_MASK;
                }
            else if ((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) ==
                (UAIF_LINE_MODE_MIC << ABOX_UAIF_SD0_MODE_OFFSET))
                {
                value = disable << ABOX_UAIF_MIC_2RX_ENABLE_OFFSET;
                mask = ABOX_UAIF_MIC_2RX_ENABLE_MASK;
                }
            else
                {
                UAIF_DBG (DBG_ERR, "%s Invalid MIC mode. ctrl1:0x%08x\n", __func__, ctrl1);
                return;
                }
            }
        }

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL0, value, mask);
    UAIF_DBG (DBG_INFO, "%s stream:%d ctrl1:0x%08x mask:0x%08x value:0x%08x\n",
              __func__, stream, ctrl1, mask, value);
    }

//abox_uaif_cfg_gpio
LOCAL void aboxUaifGpioCfg
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData,
    uint32_t                    pinctrlId
    )
    {
    if (vxbPinMuxEnableById (pUaifData->pDev, pinctrlId) != OK)
        {
        UAIF_DBG (DBG_ERR, "%s: failed to set pinctrlId=%d\n", __func__);
        }
    }

//abox_uaif_get_route
LOCAL STATUS aboxUaifRouteGet
    (
    VX_SND_CONTROL *       kcontrol,
    VX_SND_CTRL_DATA_VAL * ucontrol
    )
    {
    return snd_soc_get_enum_double (kcontrol, ucontrol);
    }

//abox_uaif_put_route
LOCAL STATUS aboxUaifRoutePut
    (
    VX_SND_CONTROL *            kcontrol,
    VX_SND_CTRL_DATA_VAL *      ucontrol
    )
    {
    VX_SND_SOC_COMPONENT *      cmpt;
    VXB_DEV_ID                  pDev;
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    VX_SND_ENUM *               e = (VX_SND_ENUM *)kcontrol->privateValue;
    uint32_t                    item = ucontrol->value.enumerated.item[0];
    int32_t                     sd = 0;
    BOOL                        isSpk = FALSE;
    char                        frontName[SND_DEV_NAME_LEN];
    char                        backName[SND_DEV_NAME_LEN];

    if (kcontrol->privateData == NULL ||
        ((VX_SND_SOC_COMPONENT *) (kcontrol->privateData))->pDev == NULL)
        {
        UAIF_DBG (DBG_ERR, "cannot find VXB_DEV from kcontrol %s",
                  kcontrol->id.name);
        return ERROR;
        }
    else
        {
        cmpt = (VX_SND_SOC_COMPONENT *) kcontrol->privateData;
        pDev = cmpt->pDev;
        }

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (pDev);
    if (pUaifData == NULL)
        {
        UAIF_DBG (DBG_ERR, "cannot find ABOX_PCM_CTRL_DATA from kcontrol %s",
                  kcontrol->id.name);
        return ERROR;
        }

    switch (e->shiftLeft)
        {
        case ABOX_UAIF_ROUTE_SPK_4TX_L:
            sd = UAIF_SD3;
            break;
        case ABOX_UAIF_ROUTE_SPK_3TX_L:
            sd = UAIF_SD2;
            break;
        case ABOX_UAIF_ROUTE_SPK_2TX_L:
            sd = UAIF_SD1;
            break;
        case ABOX_UAIF_ROUTE_SPK_1TX_L:
            sd = UAIF_SD0;
            break;
        default:
            UAIF_DBG (DBG_ERR, "%s incorrect value : %x\n", __func__, e->shiftLeft);
            return ERROR;
    }
    isSpk = item > 0 ? TRUE : FALSE;
    vxbAboxUaifModeSet (pUaifData, sd, SNDRV_PCM_STREAM_PLAYBACK, isSpk);

    if (snd_soc_put_enum_double (kcontrol, ucontrol) != OK)
        {
        return ERROR;
        }

    /* set backend */

    (void) snprintf_s (frontName, SND_DEV_NAME_LEN, "RDMA%d",
                       item -1);
    if (item == 0)
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "NULL");
        }
    else
        {
        (void) snprintf_s (backName, SND_DEV_NAME_LEN, "UAIF%d", pUaifData->id);
        }

    if (sndSocBeConnectByName (cmpt->card, (const char *) frontName,
                               (const char *) backName,
                               SNDRV_PCM_STREAM_PLAYBACK) != OK)
        {
        UAIF_DBG (DBG_ERR, "failed to connect dynamic PCM\n");
        return ERROR;
        }

    return OK;
    }

LOCAL STATUS aboxUaifControlCreate
    (
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData
    )
    {
    int32_t                     i;
    VX_SND_ENUM *               route = NULL;
    VX_SND_CONTROL *            control = NULL;
    size_t                      size;
    char *                      name = NULL;
    uint32_t                    rdma = pUaifData->nRdma;
    static const char * const enumUaifStr[]   =
        {
        aboxUaifSIFS[0], aboxUaifSIFS[1],
        aboxUaifSIFS[2], aboxUaifSIFS[3],
        aboxUaifSIFS[4], aboxUaifSIFS[5],
        aboxUaifSIFS[6], aboxUaifSIFS[7],
        aboxUaifSIFS[8], aboxUaifSIFS[9],
        aboxUaifSIFS[10], aboxUaifSIFS[11],
        aboxUaifSIFS[12], aboxUaifSIFS[13],
        aboxUaifSIFS[14], aboxUaifSIFS[15],
        aboxUaifSIFS[16], aboxUaifSIFS[17],
        aboxUaifSIFS[18], aboxUaifSIFS[19],
        aboxUaifSIFS[20], aboxUaifSIFS[21],
        aboxUaifSIFS[22], aboxUaifSIFS[23],
        aboxUaifSIFS[24]
        };
    VX_SND_ENUM                 aboxUaifCtrlRoute[4] =
        {
        SOC_ENUM_DOUBLE (ABOX_UAIF_ROUTE_CTRL, ABOX_UAIF_ROUTE_SPK_1TX_L,
                         ABOX_UAIF_ROUTE_SPK_1TX_L, rdma + 1,
                         (const char *const *) enumUaifStr),
        SOC_ENUM_DOUBLE (ABOX_UAIF_ROUTE_CTRL, ABOX_UAIF_ROUTE_SPK_2TX_L,
                         ABOX_UAIF_ROUTE_SPK_2TX_L, rdma + 1,
                         (const char *const *) enumUaifStr),
        SOC_ENUM_DOUBLE (ABOX_UAIF_ROUTE_CTRL, ABOX_UAIF_ROUTE_SPK_3TX_L,
                         ABOX_UAIF_ROUTE_SPK_3TX_L, rdma + 1,
                         (const char *const *) enumUaifStr),
        SOC_ENUM_DOUBLE (ABOX_UAIF_ROUTE_CTRL, ABOX_UAIF_ROUTE_SPK_4TX_L,
                         ABOX_UAIF_ROUTE_SPK_4TX_L, rdma + 1,
                         (const char *const *) enumUaifStr),
        };
    VX_SND_CONTROL              uaifCtrlTemplate[4] =
        {
        SOC_ENUM_EXT ("UAIF%d SPK",     aboxUaifCtrlRoute[0], aboxUaifRouteGet,
                      aboxUaifRoutePut),
        SOC_ENUM_EXT ("UAIF%d 2TX SPK", aboxUaifCtrlRoute[1], aboxUaifRouteGet,
                      aboxUaifRoutePut),
        SOC_ENUM_EXT ("UAIF%d 3TX SPK", aboxUaifCtrlRoute[2], aboxUaifRouteGet,
                      aboxUaifRoutePut),
        SOC_ENUM_EXT ("UAIF%d 4TX SPK", aboxUaifCtrlRoute[3], aboxUaifRouteGet,
                      aboxUaifRoutePut),
        };

    /* create new enum */

    size = sizeof (aboxUaifCtrlRoute);
    route = vxbMemAlloc (size);
    if (route == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s: Fail to allocate %d route(s)\n",
                  __func__, pUaifData->dataLine);
        return ERROR;
        }
    (void) memcpy_s (route, size, aboxUaifCtrlRoute, size);

    /* create new snd control */

    size = sizeof (uaifCtrlTemplate);
    control = vxbMemAlloc (size);
    if (control == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s: Fail to allocate kcontrol\n", __func__);
        goto errOut;
        }
    (void) memcpy_s (control, size, uaifCtrlTemplate, size);

    /* allocate names memory*/

    name = vxbMemAlloc (SND_DEV_NAME_LEN * pUaifData->dataLine);
    if (name == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s: Fail to allocate control name\n", __func__);
        goto errOut;
        }

    /* initialize all controls */

    for (i = 0; i < pUaifData->dataLine; i++)
        {
        control[i].privateValue = (unsigned long) &route[i];
        control[i].id.name = name + (SND_DEV_NAME_LEN * i);
        (void) snprintf_s (control[i].id.name, SND_DEV_NAME_LEN,
                           uaifCtrlTemplate[i].id.name, pUaifData->id);
        }

    pUaifData->cmpntDrv->controls = control;
    pUaifData->cmpntDrv->num_controls = pUaifData->dataLine;

    return OK;

errOut:
    if (route != NULL)
        {
        vxbMemFree (route);
        }
    if (name != NULL)
        {
        vxbMemFree (name);
        }
    if (control != NULL)
        {
        vxbMemFree (control);
        }

    return ERROR;
    }

//abox_uaif_set_fmt
LOCAL STATUS aboxUaifFmtSet
    (
    VX_SND_SOC_DAI *            dai,
    uint32_t                    fmt
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet (dai->pDev);
    uint32_t                    value = 0;
    uint32_t                    mask = 0;
    int32_t                     muxType = 0;

    UAIF_DBG (DBG_INFO, "%s(%d) fmt:0x%08x\n", __func__, __LINE__, fmt);

    /* Check UAIF is enabled or not */

    aboxUaifRegmapRead (pUaifData, ABOX_UAIF_CTRL0, &value);

    mask = ABOX_UAIF_SPK_ENABLE_MASK |
           ABOX_UAIF_SPK_2TX_ENABLE_MASK |
           ABOX_UAIF_SPK_3TX_ENABLE_MASK |
           ABOX_UAIF_SPK_4TX_ENABLE_MASK |
           ABOX_UAIF_MIC_ENABLE_MASK |
           ABOX_UAIF_MIC_2RX_ENABLE_MASK |
           ABOX_UAIF_MIC_3RX_ENABLE_MASK |
           ABOX_UAIF_MIC_4RX_ENABLE_MASK;

    if ((value & mask) != 0)
        {
        UAIF_DBG (DBG_INFO, "%s(%d) UAIF%d running\n",
                  __func__, __LINE__, pUaifData->id);
        return OK;
        }

    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
        {
        case VX_SND_DAI_FORMAT_I2S:
            value = (0 << ABOX_UAIF_WS_MODE_OFFSET);
            break;
        case VX_SND_DAI_FORMAT_DSP_A:
            value = (1 << ABOX_UAIF_WS_MODE_OFFSET);
            break;
        default:
            return ERROR;
        }
    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                             value, ABOX_UAIF_WS_MODE_MASK);

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                             (1 << ABOX_UAIF_REAL_I2S_MODE_OFFSET),
                             ABOX_UAIF_REAL_I2S_MODE_MASK);

    switch (fmt & SND_SOC_DAIFMT_INV_MASK)
        {
        case SND_SOC_DAIFMT_NB_NF:
            if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_MASK);
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            else if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBM_CFM)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (1 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     (0 << ABOX_UAIF_WS_POLAR_OFFSET),
                                     ABOX_UAIF_WS_POLAR_MASK);
            break;
        case SND_SOC_DAIFMT_NB_IF:
            if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_MASK);
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            else if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBM_CFM)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (1 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     (1 << ABOX_UAIF_WS_POLAR_OFFSET),
                                     ABOX_UAIF_WS_POLAR_MASK);
            break;
        case SND_SOC_DAIFMT_IB_NF:
            if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (1 << ABOX_UAIF_BCLK_POLARITY_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_MASK);
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            else if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBM_CFM)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
            }
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     (0 << ABOX_UAIF_WS_POLAR_OFFSET),
                                     ABOX_UAIF_WS_POLAR_MASK);
            break;
        case SND_SOC_DAIFMT_IB_IF:
            if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (1 << ABOX_UAIF_BCLK_POLARITY_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_MASK);
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            else if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBM_CFM)
                {
                aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                         (0 << ABOX_UAIF_BCLK_POLARITY_S_OFFSET),
                                         ABOX_UAIF_BCLK_POLARITY_S_MASK);
                }
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     (1 << ABOX_UAIF_WS_POLAR_OFFSET),
                                     ABOX_UAIF_WS_POLAR_MASK);
            break;
        default:
            return ERROR;
        }

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
        {
        case SND_SOC_DAIFMT_CBM_CFM:
            value = (0 << ABOX_UAIF_MODE_OFFSET);
            muxType = MUX_UAIF_SLAVE;
            break;

        case SND_SOC_DAIFMT_CBS_CFS:
            value = (1 << ABOX_UAIF_MODE_OFFSET);
            muxType = MUX_UAIF_MASTER;
            break;
        default:
            return ERROR;
        }

    UAIF_DBG (DBG_INFO, "%s(%d) muxType:%x\n", __func__, __LINE__, muxType);

    if (muxType == MUX_UAIF_SLAVE)
        {
        (void) aboxClkRateSet (pUaifData->bclkMux, MUX_UAIF_SLAVE);
        }
    else
        {
        (void) aboxClkRateSet (pUaifData->bclkMux, vxbClkRateGet (pUaifData->bclk));
        }

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL0,
                             value, ABOX_UAIF_MODE_MASK);

    return OK;
    }

//abox_uaif_set_tristate
LOCAL STATUS aboxUaifTristateSet
    (
    VX_SND_SOC_DAI *            dai,
    int32_t                     tristate
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet (dai->pDev);
    ABOX_CTR_DATA *             abox_data = pUaifData->abox_data;

    UAIF_DBG (DBG_INFO, "%s, tristate %d\n", __func__, tristate);
    return abox_disable_qchannel (abox_data, ABOX_BCLK_UAIF0 + pUaifData->id, !tristate);
    }

//abox_uaif_startup
LOCAL STATUS aboxUaifStartup
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_SOC_DAI *            dai
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet(dai->pDev);
    uint32_t                    value = 0;
    uint32_t                    mask = 0;

    UAIF_DBG (DBG_INFO, "%s(%d) UAIF%d start up\n", __func__, __LINE__, pUaifData->id);

    pUaifData->substream = substream;

    if (vxbClkEnable (pUaifData->bclk) != OK)
        {
        UAIF_DBG (DBG_ERR, "Failed to enable bclk\n");
        return ERROR;
        }

    if (vxbClkEnable (pUaifData->bclkGate) != OK)
        {
        UAIF_DBG (DBG_ERR, "Failed to enable bclkGate\n");
        return ERROR;
        }

    if (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE)
        {
        value = ((1 << ABOX_UAIF_DATA_MODE_OFFSET) | (0 << ABOX_UAIF_IRQ_MODE_OFFSET));
        mask = ABOX_UAIF_DATA_MODE_MASK | ABOX_UAIF_IRQ_MODE_MASK;
        aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL0, value, mask);
        }

    return OK;
    }

//abox_uaif_shutdown
LOCAL void aboxUaifShutdown
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_SOC_DAI *            dai
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet (dai->pDev);

    UAIF_DBG (DBG_INFO, "%s(%d) shutdown\n", __func__, __LINE__);

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        aboxUaifDisable (pUaifData, substream->stream->direct, FALSE);
        }

    pUaifData->substream = NULL;
    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        UAIF_DBG (DBG_ERR, "%s[%d] uaif disconnect\n", __func__, pUaifData->id);
        return;
        }

    (void) vxbClkDisable (pUaifData->bclkGate);
    (void) vxbClkDisable (pUaifData->bclk);
    if (vxbClkRateSet (pUaifData->bclkMux,
                       vxbClkRateGet (pUaifData->bclk)) != OK)
        {
        UAIF_DBG (DBG_ERR, "%s: failed to set mux clock\n", __func__);
        }

    return;
    }

//abox_uaif_hw_params
LOCAL STATUS aboxUaifHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      hw_params,
    VX_SND_SOC_DAI *            dai
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    ABOX_CTR_DATA *             abox_data;
    uint32_t                    channels;
    uint32_t                    rate;
    uint32_t                    width;
    uint32_t                    format;
    uint32_t                    ctrl0;
    uint32_t                    ctrl1;

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (dai->pDev);
    abox_data = pUaifData->abox_data;

    UAIF_DBG (DBG_INFO, "%s[%s]\n", __func__,
        (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE) ? "Capture" : "Playback");

    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        UAIF_DBG (DBG_ERR, "%s[%d] DMA disconnect\n", __func__, pUaifData->id);
        return ERROR;
        }

    channels = PARAMS_CHANNELS (hw_params);
    rate = PARAMS_RATE (hw_params);
    width = PARAMS_WIDTH (hw_params);
    format = PARAMS_FORMAT (hw_params);

    if (abox_check_bclk_from_cmu_clock (abox_data, rate, channels, width) != OK)
        {
        UAIF_DBG (DBG_ERR, "bclk cmu clock check failed\n");
        }

    if (aboxClkRateSet (pUaifData->bclk, rate * channels * width) != OK)
        {
        UAIF_DBG (DBG_ERR, "bclk set error\n");
        return ERROR;
        }

    UAIF_DBG (DBG_INFO, "[%s] rate=%u, width=%d, channel=%u, bclk=%lu\n",
              __func__, rate, width, channels, vxbClkRateGet (pUaifData->bclk));

    switch (format)
        {
        case VX_SND_FMT_S16_LE:
        case VX_SND_FMT_S24_LE:
        case VX_SND_FMT_S32_LE:
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     (width - 1) << ABOX_UAIF_SBIT_MAX_OFFSET,
                                     ABOX_UAIF_SBIT_MAX_MASK);
            break;
        default:
            return ERROR;
        }

    switch (channels)
        {
        case 2:
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     0 << ABOX_UAIF_VALID_STR_OFFSET,
                                     ABOX_UAIF_VALID_STR_MASK);
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     0 << ABOX_UAIF_VALID_END_OFFSET,
                                     ABOX_UAIF_VALID_END_MASK);
            break;
        case 1:
        case 4:
        case 6:
        case 8:
        case 16:
        case 32:
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     ((width - 1) << ABOX_UAIF_VALID_STR_OFFSET),
                                     ABOX_UAIF_VALID_STR_MASK);
            aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                                     ((width - 1) << ABOX_UAIF_VALID_END_OFFSET),
                                     ABOX_UAIF_VALID_END_MASK);
            break;
        default:
            return ERROR;
        }
    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL1,
                             ((channels - 1) << ABOX_UAIF_SLOT_MAX_OFFSET),
                             ABOX_UAIF_SLOT_MAX_MASK);

    format = (channels - 1);
    switch (width)
        {
        case 16:
            format |= 1 << 5;
            break;
        case 24:
            format |= 2 << 5;
            break;
        case 32:
            format |= 3 << 5;
            break;
        default:
            break;
        }

    UAIF_DBG (DBG_INFO, "%s - (%d, %d): %u\n", __func__, width, channels, format);

    aboxUaifRegmapWriteBits (pUaifData, ABOX_UAIF_CTRL0,
                             (format << ABOX_UAIF_FORMAT_OFFSET),
                             ABOX_UAIF_FORMAT_MASK);

    aboxUaifRegmapRead (pUaifData, ABOX_UAIF_CTRL0, &ctrl0);
    aboxUaifRegmapRead (pUaifData, ABOX_UAIF_CTRL1, &ctrl1);
    UAIF_DBG (DBG_INFO, "[%s] UAIF(%d) ctrl0=0x%0x ctrl1=0x%0x\n", __func__,
              pUaifData->id, ctrl0, ctrl1);

    aboxUaifGpioCfg (pUaifData, pUaifData->resetPinctrlId);

    return OK;
    }

//abox_uaif_hw_free
LOCAL STATUS aboxUaifHwFree
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_SOC_DAI *            dai
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;

    UAIF_DBG (DBG_INFO, "%s - (%d)\n", __func__, __LINE__);

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (dai->pDev);

    aboxUaifGpioCfg (pUaifData, pUaifData->idlePinctrlId);
    return OK;
    }

//abox_uaif_trigger
LOCAL STATUS aboxUaifTrigger
    (
    SND_PCM_SUBSTREAM *         substream,
    int32_t                     trigger,
    VX_SND_SOC_DAI *            dai
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet (dai->pDev);
    STATUS                      ret = OK;

    UAIF_DBG (DBG_INFO, "%s[%s] trigger %d\n", __func__,
              (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE) ?
              "Carpture" : "Playback", trigger);

    switch (trigger)
        {
        case SNDRV_PCM_TRIGGER_START:
        //case SNDRV_PCM_TRIGGER_RESUME:
        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
            aboxUaifEnable (pUaifData, substream->stream->direct, TRUE);
            break;
        case SNDRV_PCM_TRIGGER_STOP:
        //case SNDRV_PCM_TRIGGER_SUSPEND:
        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
            if (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE)
                {
                aboxUaifDisable (pUaifData, substream->stream->direct, FALSE);
                }
            break;
        default:
            ret = ERROR;
        }

    UAIF_DBG (DBG_INFO, "%s(%d) - %d\n", __func__, __LINE__, ret);

    return ret;
    }

//abox_uaif_cmpnt_probe
LOCAL STATUS aboxUaifCmpntProbe
    (
    VX_SND_SOC_COMPONENT *      cmpnt
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData = vxbDevSoftcGet (cmpnt->pDev);

    UAIF_DBG (DBG_INFO, "%s:[%d]\n", __func__, pUaifData->id);

    pUaifData->cmpnt = cmpnt;
    //snd_soc_component_init_regmap(cmpnt, data->regmap);
    if (pUaifData->syncMode == UAIF_LINE_ASYNC)
        {
        aboxUaifRegmapWrite (pUaifData, ABOX_UAIF_CTRL2, 0x0);
        }

    return OK;
    }

//abox_uaif_cmpnt_remove
LOCAL void aboxUaifCmpntRemove
    (
    VX_SND_SOC_COMPONENT *  cmpnt
    )
    {
    UAIF_DBG (DBG_INFO,  "%s\n", __func__);
    }

//abox_uaif_register_component
LOCAL STATUS aboxUaifComponentRegister
    (
    VXB_DEV_ID                  pDev
    )
    {
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    size_t                      strLen;
    char *                      pTmpStr;

    UAIF_DBG (DBG_INFO,  "%s\n", __func__);

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet (pDev);

    /* build DAI drv */

    pUaifData->daiDrv =(VX_SND_SOC_DAI_DRIVER *)
                          vxbMemAlloc(sizeof (VX_SND_SOC_DAI_DRIVER));
    if (pUaifData->daiDrv == NULL)
        {
        return ERROR;
        }
    memcpy (pUaifData->daiDrv, &aboxUaifDaiDrv, sizeof (VX_SND_SOC_DAI_DRIVER));

    pUaifData->daiDrv->id = pUaifData->id;

    strLen = snprintf (NULL, 0, pUaifData->daiDrv->name, pUaifData->id);
    pTmpStr = vxbMemAlloc (strLen + 1);
    if (pTmpStr == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s(%d): Fail to create name string\n",
                  __func__, __LINE__);
        goto errOut;
        }
    else
        {
        snprintf (pTmpStr, strLen + 1, pUaifData->daiDrv->name, pUaifData->id);
        }
    pUaifData->daiDrv->name = pTmpStr;

    strLen = snprintf (NULL, 0,
                       pUaifData->daiDrv->playback.streamName, pUaifData->id);
    pTmpStr = vxbMemAlloc (strLen + 1);
    if (pTmpStr == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s(%d): Fail to create name string\n",
                  __func__, __LINE__);
        goto errOut;
        }
    else
        {
        snprintf (pTmpStr, strLen + 1,
                  pUaifData->daiDrv->playback.streamName, pUaifData->id);
        }
    pUaifData->daiDrv->playback.streamName = pTmpStr;

    strLen = snprintf (NULL, 0,
                       pUaifData->daiDrv->capture.streamName, pUaifData->id);
    pTmpStr = vxbMemAlloc (strLen + 1);
    if (pTmpStr == NULL)
        {
        UAIF_DBG (DBG_ERR, "%s(%d): Fail to create name string\n",
                  __func__, __LINE__);
        goto errOut;
        }
    else
        {
        snprintf (pTmpStr, strLen + 1,
                  pUaifData->daiDrv->capture.streamName, pUaifData->id);
        }
    pUaifData->daiDrv->capture.streamName = pTmpStr;

    /* build component drv */

    pUaifData->cmpntDrv =(VX_SND_SOC_CMPT_DRV *)
                            vxbMemAlloc (sizeof (VX_SND_SOC_CMPT_DRV));
    if (pUaifData->cmpntDrv == NULL)
        {
        goto errOut;
        }
    memcpy (pUaifData->cmpntDrv, &aboxUaifCmpnt, sizeof (VX_SND_SOC_CMPT_DRV));

    UAIF_DBG (DBG_INFO, "%s: pUaifData->type:%d\n", __func__, pUaifData->type);

    if (pUaifData->type != UAIF_SFT)
        {
        if (aboxUaifControlCreate (pUaifData) != OK)
            {
            UAIF_DBG (DBG_ERR, "%s: Fail to create controls\n", __func__);
            goto errOut;
            }
        }

    if (vxSndCpntRegister (pDev, pUaifData->cmpntDrv, pUaifData->daiDrv, 1) != OK)
        {
        UAIF_DBG (DBG_ERR, "%s: component register failed:%d\n", __func__);
        goto errOut;
        }

    return OK;

errOut:
    if (pUaifData->cmpntDrv != NULL)
        {
        vxbMemFree (pUaifData->cmpntDrv);
        }

    if (pUaifData->daiDrv->capture.streamName != NULL)
        {
        vxbMemFree ((void *) pUaifData->daiDrv->capture.streamName);
        }

    if (pUaifData->daiDrv->playback.streamName != NULL)
        {
        vxbMemFree ((void *) pUaifData->daiDrv->playback.streamName);
        }

    if (pUaifData->daiDrv->name != NULL)
        {
        vxbMemFree ((void *) pUaifData->daiDrv->name);
        }

    if (pUaifData->daiDrv != NULL)
        {
        vxbMemFree (pUaifData->daiDrv);
        }

    return ERROR;
    }

//void abox_uaif_set_mode(struct device *dev, struct abox_uaif_data *data,
//  int sd, int stream, int mode)
void vxbAboxUaifModeSet
    (
    VXB_ABOX_UAIF_DRV_CTRL * data,
    int32_t                  sd,
    int32_t                  stream,
    int32_t                  mode
    )
    {
    if (sd >= UAIF_SD_MAX)
        {
        UAIF_DBG (DBG_ERR, "%s: invalid sd: %d.\n", __func__, sd);
        return;
        }

    if (stream == SNDRV_PCM_STREAM_PLAYBACK)
        {
        if (mode == UAIF_LINE_MODE_SPK)
            {
            aboxUaifModePairSet (data, sd, 1, mode);
            }
        else
            {
            aboxUaifModePairSet (data, sd, 0, mode);
            }
        }
    else    //SNDRV_PCM_STREAM_CAPTURE
        {
        if (mode == UAIF_LINE_MODE_MIC)
            {
            aboxUaifModePairSet (data, sd, 0, mode);
            }
        else
            {
            aboxUaifModePairSet (data, sd, 1, mode);
            }
        }
    }

LOCAL STATUS aboxUaifAttach
    (
    VXB_DEV_ID                  pDev
    )
    {
    VXB_FDT_DEV *               pFdtDev;
    VXB_FDT_DEV *               pParFdtDev;
    VXB_ABOX_UAIF_DRV_CTRL *    pUaifData;
    VXB_RESOURCE *              pResMem = NULL;
    uint32_t *                  pValue;
    int32_t                     len;
    int32_t                     offset;
    uint32_t                    dmaType;
    uint32_t                    dmaId;
    char *                      nodeCompat;
    char *                      dmaStat;

    if (pDev == NULL)
        {
        UAIF_DBG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        return ERROR;
        }

    pUaifData = (VXB_ABOX_UAIF_DRV_CTRL *)
                    vxbMemAlloc (sizeof (VXB_ABOX_UAIF_DRV_CTRL));
    if (pUaifData == NULL)
        {
        UAIF_DBG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        return ERROR;
        }

    pUaifData->resetPinctrlId = vxbPinMuxGetIdByName (pDev, "reset");
    if (pUaifData->resetPinctrlId == PINMUX_ERROR_ID)
        {
        UAIF_DBG (DBG_ERR, "failed to get tas6424_reset pinctrl.\n");
        goto errOut;
        }

    pUaifData->idlePinctrlId = vxbPinMuxGetIdByName (pDev, "idle");
    if (pUaifData->idlePinctrlId == PINMUX_ERROR_ID)
        {
        UAIF_DBG (DBG_ERR, "failed to get tas6424_idle pinctrl.\n");
        goto errOut;
        }

    /* DTS configuration */

    pResMem = vxbResourceAlloc (pDev, VXB_RES_MEMORY, 0);
    if ((pResMem == NULL) || (pResMem->pRes == NULL))
        {
        UAIF_DBG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        goto errOut;
        }

    pUaifData->regBase = ((VXB_RESOURCE_ADR *)(pResMem->pRes))->virtAddr;
    pUaifData->regHandle = ((VXB_RESOURCE_ADR *)(pResMem->pRes))->pHandle;

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        UAIF_DBG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        goto errOut;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,id", &len);
    if ((pValue == NULL) || (len != sizeof (int32_t)))
        {
        UAIF_DBG (DBG_ERR, "failed to get uaif id.\n");
        goto errOut;
        }
    pUaifData->id = vxFdt32ToCpu (*pValue);
    UAIF_DBG (DBG_INFO, "id = %d\n", pUaifData->id);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,data-line", &len);
    if ((pValue == NULL) || (len != sizeof (int32_t)))
        {
        UAIF_DBG (DBG_ERR, "failed to get uaif data line.\n");
        goto errOut;
        }
    pUaifData->dataLine = vxFdt32ToCpu (*pValue);
    UAIF_DBG (DBG_INFO, "data line = %d\n", pUaifData->dataLine);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,type", &len);
    if ((pValue == NULL) || (len != sizeof (int32_t)))
        {
        UAIF_DBG (DBG_ERR, "failed to get uaif type.\n");
        goto errOut;
        }
    pUaifData->type = vxFdt32ToCpu (*pValue);
    UAIF_DBG (DBG_INFO, "type = %d\n", pUaifData->type);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "sync_mode", &len);
    if ((pValue == NULL) || (len != sizeof (int32_t)))
        {
        UAIF_DBG (DBG_ERR, "failed to get uaif sync mode.\n");
        goto errOut;
        }
    pUaifData->syncMode = vxFdt32ToCpu (*pValue);
    UAIF_DBG (DBG_INFO, "sync mode = %d\n", pUaifData->syncMode);

    /* clk get */

    pUaifData->bclk = vxbClkGet (pDev, "bclk");
    if (pUaifData->bclk == NULL)
        {
        UAIF_DBG (DBG_ERR, "cannot get bclk\n");
        goto errOut;
        }

    pUaifData->bclkMux = vxbClkGet (pDev, "bclk_mux");
    if (pUaifData->bclkMux == NULL)
        {
        UAIF_DBG (DBG_ERR, "cannot get bclk_mux\n");
        goto errOut;
        }

    pUaifData->bclkGate = vxbClkGet (pDev, "bclk_gate");
    if (pUaifData->bclkGate == NULL)
        {
        UAIF_DBG (DBG_ERR, "cannot get bclk_gate\n");
        goto errOut;
        }

    pUaifData->abox_data = ABOX_CTR_GLB_DATA;

    if (abox_register_uaif (pUaifData->abox_data->pDev,
                            pDev, pUaifData->id) != OK)
        {
        UAIF_DBG (DBG_ERR, "Failed to register UAIF pdev\n");
        goto errOut;
        }

    /* dma get */

    pUaifData->nRdma = 0;
    pUaifData->nWdma = 0;

    pParFdtDev = vxbFdtDevGet (vxbDevParent (pDev));
    if (pParFdtDev == NULL)
        {
        UAIF_DBG (DBG_INFO, "%s: failed to pParFdtDev\n", __func__);
        goto errOut;
        }

    offset = pParFdtDev->offset;
    for (offset = vxFdtFirstSubnode (offset); offset > 0;
         offset = vxFdtNextSubnode (offset))
        {
        nodeCompat = (char *) vxFdtPropGet (offset, "compatible", &len);
        if (nodeCompat == NULL)
            {
            UAIF_DBG (DBG_ERR, "%s: failed to get compatible.\n", __func__);
            continue;
            }

        UAIF_DBG (DBG_INFO, "nodeCompat = %s\n", nodeCompat);

        if (strncmp (nodeCompat,
                     "samsung,abox-rdma", strlen (nodeCompat)) == 0)
            {
            pValue = (uint32_t *) vxFdtPropGet (offset, "samsung,id", &len);
            if (pValue == NULL)
                {
                UAIF_DBG (DBG_ERR, "%s: failed to get dma id.\n", __func__);
                goto errOut;
                }
            dmaId = vxFdt32ToCpu (*pValue);
            UAIF_DBG (DBG_INFO, "dma id = %d\n", dmaId);

            pValue = (uint32_t *) vxFdtPropGet (offset, "samsung,type", &len);
            if (pValue == NULL)
                {
                UAIF_DBG (DBG_ERR, "%s: failed to get dma type.\n", __func__);
                goto errOut;
                }
            dmaType = vxFdt32ToCpu (*pValue);
            UAIF_DBG (DBG_INFO, "%s: dma type = %d\n", __func__, dmaType);

            dmaStat = (char *) vxFdtPropGet (offset, "status", &len);
            if (dmaStat == NULL)
                {
                UAIF_DBG (DBG_ERR, "%s: failed to get dma status.\n", __func__);
                goto errOut;
                }

            UAIF_DBG (DBG_INFO, "%s: dma status = %s\n", __func__, dmaStat);

            if (dmaType == DMA_TO_NON_SFT_UAIF)
                {
                if (strncmp (dmaStat, "okay", strlen (dmaStat)) == 0)
                    {
                    (void) snprintf (aboxUaifSIFS[dmaId + 1],
                                     CTRL_STR_SIZE - 1,
                                     "SIFS%d_SET", dmaId);
                    }
                else
                    {
                    (void) snprintf (aboxUaifSIFS[dmaId + 1],
                                     CTRL_STR_SIZE - 1,
                                     "NO_CONNECT");
                    }

                pUaifData->nRdma++;
                }
            }
        else if (strncmp (nodeCompat,
                          "samsung,abox-wdma", strlen (nodeCompat)) == 0)
            {
            pValue = (uint32_t *) vxFdtPropGet (offset, "samsung,type", &len);
            if (pValue == NULL)
                {
                UAIF_DBG (DBG_ERR, "%s: failed to get dma type.\n", __func__);
                goto errOut;
                }
            dmaType = vxFdt32ToCpu (*pValue);
            UAIF_DBG (DBG_INFO, "%s: dma type = %d\n", __func__, dmaType);

            if (dmaType == DMA_TO_NON_SFT_UAIF)
                {
                pUaifData->nWdma++;
                }
            }
        }

    SPIN_LOCK_ISR_INIT (&pUaifData->regmapSpinlockIsr, 0);

    /* save pUaifData in VXB_DEVICE structure */

    pUaifData->pDev = pDev;
    vxbDevSoftcSet (pDev, pUaifData);

    /* register uaif component */

    if (aboxUaifComponentRegister (pDev) != OK)
        {
        goto errOut;
        }

    return OK;

errOut:
    if (pResMem != NULL)
        {
        (void) vxbResourceFree (pDev, pResMem);
        }

    if (pUaifData != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pUaifData);
        }

    return ERROR;
    }

