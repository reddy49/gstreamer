/* vxbFdtTas6424.c - Tas6424 codec driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/buslib/vxbI2cLib.h>
#include <subsys/gpio/vxbGpioLib.h>
#include <soc.h>
#include <vxbFdtTca6416aTas6424.h>
#include <vxbFdtTas6424.h>

#undef TAS6424_DEBUG
#ifdef TAS6424_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t tas6424DbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((tas6424DbgMask & (mask)) || ((mask) == DBG_ALL))           \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("%s(%d) ", __func__, __LINE__);       \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* TAS6424_DEBUG */

/* defines */

/* DTS value defines */

#define ABOX_V920 920

/* DTS value defines end */

#define MS_TO_TICKS(ms) \
    ((_Vx_ticks_t)(((sysClkRateGet() * (ms)) + 999) / 1000))

#define TAS6424_FAULT_CHECK_INTERVAL 200
#define TAS6424_RESET_TIMEOUT 40000
#define CH_VOL 0x8F

#define TAS6424_GPIO_MUTE   GPIO_VALUE_HIGH
#define TAS6424_GPIO_UNMUTE GPIO_VALUE_LOW

/* forward declarations */

IMPORT void vxbUsDelay (int delayTime);
IMPORT void vxbMsDelay (int delayTime);

LOCAL STATUS tas6424Probe (VXB_DEV_ID pDev);
LOCAL STATUS tas6424Attach (VXB_DEV_ID pDev);
LOCAL uint32_t tas6424CmpntRead (VX_SND_SOC_COMPONENT * cmpnt,
                                 uint32_t regOfs);
LOCAL STATUS tas6424CmpntWrite (VX_SND_SOC_COMPONENT * cmpnt,
                                 uint32_t regOfs, uint32_t val);
LOCAL int32_t tas6424PowerOn (VX_SND_SOC_COMPONENT * component);
LOCAL int32_t tas6424PowerOff (VX_SND_SOC_COMPONENT * component);
LOCAL int32_t tas6424DacEvent (VX_SND_SOC_COMPONENT * component,
                               int32_t event);
LOCAL uint32_t tas6424CmpntRead (VX_SND_SOC_COMPONENT * cmpnt,
                                 uint32_t regOfs);
LOCAL STATUS tas6424ComponentPrepare (struct vxSndSocComponent * component,
                                      SND_PCM_SUBSTREAM * substream);
LOCAL STATUS tas6424ComponentHwFree (struct vxSndSocComponent * component,
                                     SND_PCM_SUBSTREAM * substream);
LOCAL STATUS tas6424HwParams (SND_PCM_SUBSTREAM * substream,
                               VX_SND_PCM_HW_PARAMS * params,
                               VX_SND_SOC_DAI * dai);
LOCAL STATUS tas6424SetDaiFmt (VX_SND_SOC_DAI * dai, uint32_t fmt);
LOCAL STATUS tas6424SetDaiTdmSlot (VX_SND_SOC_DAI * dai, uint32_t txMask,
                                    uint32_t rxMask, int32_t slots,
                                    int32_t slotWidth);
LOCAL STATUS tas6424Mute (VX_SND_SOC_DAI * dai, int32_t mute, int32_t stream);

LOCAL void tas6424RegDump
    (
    VXB_DEV_ID pDev
    );
/* locals */

LOCAL VXB_DRV_METHOD tas6424MethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), tas6424Probe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), tas6424Attach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY tas6424Match[] =
    {
    {
    "ti,tas6424",
    NULL
    },
    {}                              /* empty terminated list */
    };

//LOCAL DECLARE_TLV_DB_SCALE(dac_tlv, -10350, 50, 0);
LOCAL unsigned int dac_tlv[] = {SNDRV_CTL_TLVT_DB_SCALE, 8, -10350, 0|50};

//tas6424_snd_controls
LOCAL const VX_SND_CONTROL tas6424SndControls[] =
    {
    SOC_SINGLE_TLV ("Speaker Driver CH1 Playback Volume",
                    TAS6424_CH1_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    SOC_SINGLE_TLV ("Speaker Driver CH2 Playback Volume",
                    TAS6424_CH2_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    SOC_SINGLE_TLV ("Speaker Driver CH3 Playback Volume",
                    TAS6424_CH3_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    SOC_SINGLE_TLV ("Speaker Driver CH4 Playback Volume",
                    TAS6424_CH4_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    };

LOCAL STATUS tas6424ComponentPrepare
    (
    struct vxSndSocComponent *  component,
    SND_PCM_SUBSTREAM *         substream
    )
    {
    (void) tas6424PowerOn (component);

    return tas6424DacEvent (component, SND_SOC_CODEC_PMU);
    }

LOCAL STATUS tas6424ComponentHwFree
    (
    struct vxSndSocComponent *  component,
    SND_PCM_SUBSTREAM *         substream
    )
    {
    (void) tas6424PowerOff (component);

    return tas6424DacEvent (component, SND_SOC_CODEC_PMD);
    }

//soc_codec_dev_tas6424
LOCAL VX_SND_SOC_CMPT_DRV socCodecDevTas6424 =
    {
    .controls               = tas6424SndControls,
    .num_controls           = NELEMENTS (tas6424SndControls),
    .use_pmdown_time        = 1,
    .endianness             = 1,
    .read                   = tas6424CmpntRead,
    .write                  = tas6424CmpntWrite,
    .prepare                = tas6424ComponentPrepare,
    .hwFree                 = tas6424ComponentHwFree,
    };

//tas6424_speaker_dai_ops
LOCAL const VX_SND_SOC_DAI_OPS tas6424SpeakerDaiOps =
    {
    .hwParams      = tas6424HwParams,
    .setFmt        = tas6424SetDaiFmt,
    .set_tdm_slot  = tas6424SetDaiTdmSlot,
    .muteStream    = tas6424Mute,
    };

//tas6424_dai
LOCAL VX_SND_SOC_DAI_DRIVER tas6424Dai[] =
    {
    {
    .name = "tas6424-amplifier",
    .playback =
        {
        .streamName    = "Playback",
        .channelsMin   = 1,
        .channelsMax   = 4,
        .rates         = TAS6424_RATES,
        .formats       = TAS6424_FORMATS,
        },
    .ops = &tas6424SpeakerDaiOps,
    },
    };

/* globals */

VXB_DRV tas6424Drv =
    {
    {NULL},
    TAS6424_DRV_NAME,                   /* Name */
    "Tas6424 FDT driver",               /* Description */
    VXB_BUSID_FDT,                      /* Class */
    0,                                  /* Flags */
    0,                                  /* Reference count */
    tas6424MethodList                   /* Method table */
    };

VXB_DRV_DEF(tas6424Drv)

LOCAL STATUS tas6424RegRead
    (
    VXB_DEV_ID      pDev,
    uint8_t         regAddr,
    uint8_t *       pRegVal
    )
    {
    I2C_MSG         msgs[2];
    int32_t         num;
    TAS6424_DATA *  pTas6424Data;
    STATUS          ret;

    pTas6424Data = (TAS6424_DATA *) vxbDevSoftcGet (pDev);
    if (pTas6424Data == NULL)
        {
        return ERROR;
        }

    (void) semTake (pTas6424Data->i2cMutex, WAIT_FOREVER);

    memset (msgs, 0, sizeof (I2C_MSG) * 2);
    num = 0;

    /* write reg address */

    msgs[num].addr  = pTas6424Data->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_WR;
    msgs[num].buf   = &regAddr;
    msgs[num].len   = 1;
    num++;

    /* read reg data */

    msgs[num].addr  = pTas6424Data->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_RD;
    msgs[num].buf   = pRegVal;
    msgs[num].len   = 1;
    num++;

    ret = vxbI2cDevXfer (pDev, &msgs[0], num);

    (void) semGive (pTas6424Data->i2cMutex);

    return ret;
    }

LOCAL STATUS tas6424RegWrite
    (
    VXB_DEV_ID      pDev,
    uint8_t         regAddr,
    uint8_t         pRegVal
    )
    {
    I2C_MSG         msg;
    uint8_t         msgBuf[2];
    TAS6424_DATA *  pTas6424Data;
    STATUS          ret;

    pTas6424Data = (TAS6424_DATA *) vxbDevSoftcGet (pDev);
    if (pTas6424Data == NULL)
        {
        return ERROR;
        }

    (void) semTake (pTas6424Data->i2cMutex, WAIT_FOREVER);

    memset (&msg, 0, sizeof (I2C_MSG));

    msgBuf[0] = regAddr;
    msgBuf[1] = pRegVal;

    /* write reg address and data */

    msg.addr    = pTas6424Data->i2cDevAddr;
    msg.scl     = 0;        /* use controller's default scl */
    msg.flags   = I2C_M_WR;
    msg.buf     = msgBuf;
    msg.len     = 2;

    ret = vxbI2cDevXfer (pDev, &msg, 1);

    (void) semGive (pTas6424Data->i2cMutex);

    return ret;
    }

LOCAL uint32_t tas6424CmpntRead
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs
    )
    {
    uint8_t                 regVal = 0;

    if (tas6424RegRead (cmpnt->pDev, (uint8_t) regOfs, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read reg [%02x]\n", (uint8_t) regOfs);
        }
    return regVal;
    }

LOCAL STATUS tas6424CmpntWrite
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs,
    uint32_t                val
    )
    {
    if (tas6424RegWrite (cmpnt->pDev, (uint8_t) regOfs, (uint8_t) val) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to write reg [%02x] = 0x%02x\n",
                 (uint8_t) regOfs, (uint8_t) val);
        return ERROR;
        }

    return 0;
    }

//tas6424_power_on
LOCAL int32_t tas6424PowerOn
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    TAS6424_DATA *          pTas6424Data;
    uint8_t                 noAutoDiags = 0;
    uint8_t                 regVal;

    pTas6424Data = (TAS6424_DATA *) vxbDevSoftcGet (component->pDev);

    if (tas6424RegRead (pTas6424Data->pDev,
                        TAS6424_DC_DIAG_CTRL1, &regVal) == OK)
        {
        noAutoDiags = regVal & TAS6424_LDGBYPASS_MASK;
        }

    if (pTas6424Data->muteGpio != ERROR)
        {
        DBG_MSG (DBG_INFO, "set muteGpio[%d] to unmute\n", pTas6424Data->muteGpio);
        (void) vxbGpioSetValue (pTas6424Data->muteGpio, TAS6424_GPIO_UNMUTE);
        }
    else
        {
        (void) tca6416aConfigAmpMute (pTas6424Data->id, TCA6416A_UNMUTE);
        }

    (void) vxSndSocComponentWrite (component, TAS6424_CH_STATE_CTRL,
                                   TAS6424_ALL_STATE_PLAY);

    /*
     * any time we come out of HIZ, the output channels automatically run DC
     * load diagnostics if autodiagnotics are enabled. wait here until this
     * completes.
     */

    if (noAutoDiags == 0)
        {
        vxbMsDelay (230);
        }

    return 0;
    }

//tas6424_power_off
LOCAL int32_t tas6424PowerOff
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    (void) vxSndSocComponentWrite (component, TAS6424_CH_STATE_CTRL,
                                   TAS6424_ALL_STATE_HIZ);

    return 0;
    }

//tas6424_hw_params
LOCAL STATUS tas6424HwParams
    (
    SND_PCM_SUBSTREAM *     substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    uint32_t                rate;
    uint32_t                width;
    uint8_t                 sapCtrl = 0;

    component = dai->component;
    rate = PARAMS_RATE (params);
    width = PARAMS_WIDTH (params);

    DBG_MSG (DBG_INFO, "rate=%u width=%u\n", rate, width);

    switch (rate)
        {
        case 44100:
            sapCtrl |= TAS6424_SAP_RATE_44100;
            break;

        case 48000:
            sapCtrl |= TAS6424_SAP_RATE_48000;
            break;

        case 96000:
            sapCtrl |= TAS6424_SAP_RATE_96000;
            break;

        default:
            DBG_MSG (DBG_ERR, "unsupported sample rate: %u\n", rate);
            return ERROR;
        }

    switch (width)
        {
        case 16:
            sapCtrl |= TAS6424_SAP_TDM_SLOT_SZ_16;
            break;

        case 24:
        case 32:
            break;

        default:
            DBG_MSG (DBG_ERR, "unsupported sample width: %u\n", width);
            return ERROR;
        }

    if (vxSndSocComponentUpdate (component, TAS6424_SAP_CTRL,
                                 (TAS6424_SAP_RATE_MASK |
                                  TAS6424_SAP_TDM_SLOT_SZ_16),
                                 sapCtrl) != OK)
        {
        return ERROR;
        }

    return OK;
    }

//tas6424_set_dai_fmt
LOCAL STATUS tas6424SetDaiFmt
    (
    VX_SND_SOC_DAI *        dai,
    uint32_t                fmt
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    uint8_t                 serialFormat = 0;

    DBG_MSG (DBG_INFO, "fmt=0x%0x\n", fmt);

    component = dai->component;

    /* clock masters */

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
        {
        case SND_SOC_DAIFMT_CBS_CFS:
            break;

        default:
            DBG_MSG (DBG_ERR, "Invalid DAI master/slave interface\n");
            return ERROR;
        }

    /* signal polarity */

    switch (fmt & SND_SOC_DAIFMT_INV_MASK)
        {
        case SND_SOC_DAIFMT_NB_NF:
            break;

        default:
            DBG_MSG (DBG_ERR, "Invalid DAI clock signal polarity\n");
            return ERROR;
        }

    /* interface format */
    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
        {
        case VX_SND_DAI_FORMAT_I2S:
            serialFormat |= TAS6424_SAP_I2S;
            break;

        case VX_SND_DAI_FORMAT_DSP_A:
            serialFormat |= TAS6424_SAP_DSP;
            break;

        case VX_SND_DAI_FORMAT_DSP_B:
            /*
             * We can use the fact that the TAS6424 does not care about the
             * LRCLK duty cycle during TDM to receive DSP_B formatted data
             * in LEFTJ mode (no delaying of the 1st data bit).
             */
            serialFormat |= TAS6424_SAP_LEFTJ;
            break;

        case SND_SOC_DAIFMT_LEFT_J:
            serialFormat |= TAS6424_SAP_LEFTJ;
            break;

        default:
            DBG_MSG (DBG_ERR, "Invalid DAI interface format\n");
            return ERROR;
        }

    if (vxSndSocComponentUpdate (component, TAS6424_SAP_CTRL,
                                 TAS6424_SAP_FMT_MASK, serialFormat) != OK)
        {
        return ERROR;
        }

    return OK;
    }

//tas6424_set_dai_tdm_slot
LOCAL STATUS tas6424SetDaiTdmSlot
    (
    VX_SND_SOC_DAI *        dai,
    uint32_t                txMask,
    uint32_t                rxMask,
    int32_t                 slots,
    int32_t                 slotWidth
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    uint32_t                firstSlot;
    uint32_t                lastSlot;
    BOOL                    sapTdmSlotLast;

    DBG_MSG (DBG_INFO, "tx_mask=%d rx_mask=%d\n", txMask, rxMask);

    component = dai->component;

    if (txMask == 0 || rxMask == 0)
        {
        return 0; /* nothing needed to disable TDM mode */
        }

    firstSlot = (uint32_t) ffs32Lsb (txMask);
    lastSlot = (uint32_t) ffs32Msb (rxMask);

    if (lastSlot - firstSlot != 4)
        {
        DBG_MSG (DBG_ERR, "tdm mask must cover 4 contiguous slots\n");
        return ERROR;
        }

    switch (firstSlot-1)
        {
        case 0:
            sapTdmSlotLast = FALSE;
            break;

        case 4:
            sapTdmSlotLast = TRUE;
            break;

        default:
            DBG_MSG (DBG_ERR, "tdm mask must start at slot 0 or 4\n");
            return ERROR;
        }

    (void) vxSndSocComponentUpdate (component,
                                    TAS6424_SAP_CTRL,
                                    TAS6424_SAP_TDM_SLOT_LAST,
                                    (sapTdmSlotLast ?
                                     TAS6424_SAP_TDM_SLOT_LAST : 0));

    return OK;
    }

//tas6424_mute
LOCAL STATUS tas6424Mute
    (
    VX_SND_SOC_DAI *        dai,
    int32_t                 mute,
    int32_t                 stream
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    TAS6424_DATA *          pTas6424Data;

    DBG_MSG (DBG_INFO, "mute=%d\n", mute);

    component = dai->component;
    pTas6424Data = (TAS6424_DATA *) vxbDevSoftcGet (component->pDev);

    if (stream == SNDRV_PCM_STREAM_CAPTURE)
        {
        return OK;
        }

    if (pTas6424Data->muteGpio != ERROR)
        {
        DBG_MSG (DBG_INFO, "set muteGpio[%d] to %s\n",pTas6424Data->muteGpio, mute ? "mute" : "unmute");
        (void) vxbGpioSetValue (pTas6424Data->muteGpio,
                                mute ? TAS6424_GPIO_MUTE : TAS6424_GPIO_UNMUTE);
        return OK;
        }
    else
        {
        tca6416aConfigAmpMute (pTas6424Data->id,
                               mute ? TCA6416A_MUTE : TCA6416A_UNMUTE);
        }

    if (mute)
        {
        (void) vxSndSocComponentWrite (component, TAS6424_CH_STATE_CTRL,
                                       TAS6424_ALL_STATE_MUTE);
        }
    else
        {
        (void) vxSndSocComponentWrite (component, TAS6424_CH_STATE_CTRL,
                                       TAS6424_ALL_STATE_PLAY);
        }

    return OK;
    }

LOCAL void startDelayWork
    (
    TAS6424_DATA *  pTas6424Data
    )
    {
    SPIN_LOCK_ISR_TAKE (&pTas6424Data->workSpinlockIsr);

    pTas6424Data->isCancel = FALSE;

    SPIN_LOCK_ISR_GIVE (&pTas6424Data->workSpinlockIsr);

    (void) semGive (pTas6424Data->faultCheckSem);
    }

LOCAL void cancelDelayWork
    (
    TAS6424_DATA *  pTas6424Data
    )
    {
    BOOL            needWait;

    /* cancel work and check if need wait the last work done */

    SPIN_LOCK_ISR_TAKE (&pTas6424Data->workSpinlockIsr);

    pTas6424Data->isCancel = TRUE;

    if (pTas6424Data->workState == TAS6424_W_RUNNING)
        {
        needWait = TRUE;
        }
    else
        {
        needWait = FALSE;
        }

    SPIN_LOCK_ISR_GIVE (&pTas6424Data->workSpinlockIsr);

    if (needWait)
        {
        while (1)
            {
            if (pTas6424Data->workState == TAS6424_W_DONE)
                {
                break;
                }
            }
        }
    }

//tas6424_fault_check_work
LOCAL void tas6424_fault_check_work
    (
    TAS6424_DATA *  pTas6424Data
    )
    {
    uint8_t         regVal;
    VXB_DEV_ID      pDev;

    pDev = pTas6424Data->pDev;

    if (tas6424RegRead (pDev, TAS6424_CHANNEL_FAULT, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read CHANNEL_FAULT register\n");
        goto out;
        }

    if (regVal == 0)
        {
        pTas6424Data->lastCfault = regVal;
        goto check_global_fault1_reg;
        }

    /*
     * Only flag errors once for a given occurrence. This is needed as
     * the TAS6424 will take time clearing the fault condition internally
     * during which we don't want to bombard the system with the same
     * error message over and over.
     */

    if ((regVal & TAS6424_FAULT_OC_CH1)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_OC_CH1))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 1 overcurrent fault\n");
        }

    if ((regVal & TAS6424_FAULT_OC_CH2)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_OC_CH2))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 2 overcurrent fault\n");
        }

    if ((regVal & TAS6424_FAULT_OC_CH3)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_OC_CH3))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 3 overcurrent fault\n");
        }

    if ((regVal & TAS6424_FAULT_OC_CH4)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_OC_CH4))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 4 overcurrent fault\n");
        }

    if ((regVal & TAS6424_FAULT_DC_CH1)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_DC_CH1))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 1 DC fault\n");
        }

    if ((regVal & TAS6424_FAULT_DC_CH2)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_DC_CH2))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 2 DC fault\n");
        }

    if ((regVal & TAS6424_FAULT_DC_CH3)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_DC_CH3))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 3 DC fault\n");
        }

    if ((regVal & TAS6424_FAULT_DC_CH4)
        && !(pTas6424Data->lastCfault & TAS6424_FAULT_DC_CH4))
        {
        DBG_MSG (DBG_ERR, "experienced a channel 4 DC fault\n");
        }

    /* Store current fault1 value so we can detect any changes next time */
    pTas6424Data->lastCfault = regVal;

check_global_fault1_reg:
    if (tas6424RegRead (pDev, TAS6424_GLOB_FAULT1, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read GLOB_FAULT1 register\n");
        goto out;
        }

    /*
     * Ignore any clock faults as there is no clean way to check for them.
     * We would need to start checking for those faults *after* the SAIF
     * stream has been setup, and stop checking *before* the stream is
     * stopped to avoid any false-positives. However there are no
     * appropriate hooks to monitor these events.
     */

    regVal &= (TAS6424_FAULT_PVDD_OV
               | TAS6424_FAULT_VBAT_OV
               | TAS6424_FAULT_PVDD_UV
               | TAS6424_FAULT_VBAT_UV);

    if (regVal == 0)
        {
        pTas6424Data->lastFault1 = regVal;
        goto check_global_fault2_reg;
        }

    if ((regVal & TAS6424_FAULT_PVDD_OV)
        && !(pTas6424Data->lastFault1 & TAS6424_FAULT_PVDD_OV))
        {
        DBG_MSG (DBG_ERR, "experienced a PVDD overvoltage fault\n");
        }

    if ((regVal & TAS6424_FAULT_VBAT_OV)
        && !(pTas6424Data->lastFault1 & TAS6424_FAULT_VBAT_OV))
        {
        DBG_MSG (DBG_ERR, "experienced a VBAT overvoltage fault\n");
        }

    if ((regVal & TAS6424_FAULT_PVDD_UV)
        && !(pTas6424Data->lastFault1 & TAS6424_FAULT_PVDD_UV))
        {
        DBG_MSG (DBG_ERR, "experienced a PVDD undervoltage fault\n");
        }

    if ((regVal & TAS6424_FAULT_VBAT_UV)
        && !(pTas6424Data->lastFault1 & TAS6424_FAULT_VBAT_UV))
        {
        DBG_MSG (DBG_ERR, "experienced a VBAT undervoltage fault\n");
        }

    /* Store current fault1 value so we can detect any changes next time */
    pTas6424Data->lastFault1 = regVal;

check_global_fault2_reg:
    if (tas6424RegRead (pDev, TAS6424_GLOB_FAULT2, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read GLOB_FAULT2 register\n");
        goto out;
        }

    regVal &= (TAS6424_FAULT_OTSD
               | TAS6424_FAULT_OTSD_CH1
               | TAS6424_FAULT_OTSD_CH2
               | TAS6424_FAULT_OTSD_CH3
               | TAS6424_FAULT_OTSD_CH4);

    if (regVal == 0)
        {
        pTas6424Data->lastFault2 = regVal;
        goto check_warn_reg;
        }

    if ((regVal & TAS6424_FAULT_OTSD)
        && !(pTas6424Data->lastFault2 & TAS6424_FAULT_OTSD))
        {
        DBG_MSG (DBG_ERR, "experienced a global overtemp shutdown\n");
        }

    if ((regVal & TAS6424_FAULT_OTSD_CH1)
        && !(pTas6424Data->lastFault2 & TAS6424_FAULT_OTSD_CH1))
        {
        DBG_MSG (DBG_ERR, "experienced an overtemp shutdown on CH1\n");
        }

    if ((regVal & TAS6424_FAULT_OTSD_CH2)
        && !(pTas6424Data->lastFault2 & TAS6424_FAULT_OTSD_CH2))
        {
        DBG_MSG (DBG_ERR, "experienced an overtemp shutdown on CH2\n");
        }

    if ((regVal & TAS6424_FAULT_OTSD_CH3)
        && !(pTas6424Data->lastFault2 & TAS6424_FAULT_OTSD_CH3))
        {
        DBG_MSG (DBG_ERR, "experienced an overtemp shutdown on CH3\n");
        }

    if ((regVal & TAS6424_FAULT_OTSD_CH4)
        && !(pTas6424Data->lastFault2 & TAS6424_FAULT_OTSD_CH4))
        {
        DBG_MSG (DBG_ERR, "experienced an overtemp shutdown on CH4\n");
        }

    /* Store current fault2 value so we can detect any changes next time */
    pTas6424Data->lastFault2 = regVal;

check_warn_reg:
    if (tas6424RegRead (pDev, TAS6424_WARN, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read WARN register\n");
        goto out;
        }

    regVal &= (TAS6424_WARN_VDD_UV
               | TAS6424_WARN_VDD_POR
               | TAS6424_WARN_VDD_OTW
               | TAS6424_WARN_VDD_OTW_CH1
               | TAS6424_WARN_VDD_OTW_CH2
               | TAS6424_WARN_VDD_OTW_CH3
               | TAS6424_WARN_VDD_OTW_CH4);

    if (regVal == 0)
        {
        pTas6424Data->lastWarn = regVal;
        goto out;
        }

    if ((regVal & TAS6424_WARN_VDD_UV)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_UV))
        {
        DBG_MSG (DBG_WARN, "experienced a VDD under voltage condition\n");
        }

    if ((regVal & TAS6424_WARN_VDD_POR)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_POR))
        {
        DBG_MSG (DBG_WARN, "experienced a VDD POR condition\n");
        }

    if ((regVal & TAS6424_WARN_VDD_OTW)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_OTW))
        {
        DBG_MSG (DBG_WARN, "experienced a global overtemp warning\n");
        }

    if ((regVal & TAS6424_WARN_VDD_OTW_CH1)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_OTW_CH1))
        {
        DBG_MSG (DBG_WARN, "experienced an overtemp warning on CH1\n");
        }

    if ((regVal & TAS6424_WARN_VDD_OTW_CH2)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_OTW_CH2))
        {
        DBG_MSG (DBG_WARN, "experienced an overtemp warning on CH2\n");
        }

    if ((regVal & TAS6424_WARN_VDD_OTW_CH3)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_OTW_CH3))
        {
        DBG_MSG (DBG_WARN, "experienced an overtemp warning on CH3\n");
        }

    if ((regVal & TAS6424_WARN_VDD_OTW_CH4)
        && !(pTas6424Data->lastWarn & TAS6424_WARN_VDD_OTW_CH4))
        {
        DBG_MSG (DBG_WARN, "experienced an overtemp warning on CH4\n");
        }

    /* Store current warn value so we can detect any changes next time */
    pTas6424Data->lastWarn = regVal;

    /* Clear any warnings by toggling the CLEAR_FAULT control bit */

    if (tas6424RegRead (pDev, TAS6424_MISC_CTRL3, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to read MISC_CTRL3 register\n");
        goto out;
        }

    regVal |= TAS6424_CLEAR_FAULT;
    if (tas6424RegWrite (pDev, TAS6424_MISC_CTRL3, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to write MISC_CTRL3 register\n");
        goto out;
        }

    regVal &= ~TAS6424_CLEAR_FAULT;
    if (tas6424RegWrite (pDev, TAS6424_MISC_CTRL3, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to write MISC_CTRL3 register\n");
        goto out;
        }

out:
    /* Schedule the next fault check at the specified interval */

    startDelayWork (pTas6424Data);
    }

//tas6424_fault_check_work
LOCAL void tas6424FaultCheckTask
    (
    TAS6424_DATA *  pTas6424Data
    )
    {
    BOOL            canRun;

    while (1)
        {
        (void) semTake (pTas6424Data->faultCheckSem, WAIT_FOREVER);

        (void) taskDelay (MS_TO_TICKS (TAS6424_FAULT_CHECK_INTERVAL));

        /* check cancel state and update work state */

        SPIN_LOCK_ISR_TAKE (&pTas6424Data->workSpinlockIsr);

        if (pTas6424Data->isCancel == TRUE)
            {
            canRun = FALSE;
            }
        else
            {
            pTas6424Data->workState = TAS6424_W_RUNNING;
            canRun = TRUE;
            }

        SPIN_LOCK_ISR_GIVE (&pTas6424Data->workSpinlockIsr);

        if (canRun)
            {
            tas6424_fault_check_work (pTas6424Data);

            /* work done, update work state */

            SPIN_LOCK_ISR_TAKE (&pTas6424Data->workSpinlockIsr);

            pTas6424Data->workState = TAS6424_W_DONE;

            SPIN_LOCK_ISR_GIVE (&pTas6424Data->workSpinlockIsr);
            }
        }
    }

//tas6424_dac_event
LOCAL int32_t tas6424DacEvent
    (
    VX_SND_SOC_COMPONENT *  component,
    int32_t                 event
    )
    {
    TAS6424_DATA *          pTas6424Data;

    DBG_MSG (DBG_INFO, "event=0x%0x\n", event);

    pTas6424Data = (TAS6424_DATA *) vxbDevSoftcGet (component->pDev);

    if (event == SND_SOC_CODEC_PMU)
        {

        /* Observe codec shutdown-to-active time */

        (void) taskDelay (MS_TO_TICKS (12));

        /* Turn on TAS6424 periodic fault checking/handling */

        pTas6424Data->lastFault1 = 0;
        pTas6424Data->lastFault2 = 0;
        pTas6424Data->lastWarn = 0;

        startDelayWork (pTas6424Data);
        }
    else if (event == SND_SOC_CODEC_PMD)
        {

        /* Disable TAS6424 periodic fault checking/handling */

        cancelDelayWork (pTas6424Data);
        }

    return 0;
    }

/******************************************************************************
*
* aboxMailboxProbe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS tas6424Probe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, tas6424Match, NULL);
    }

/******************************************************************************
*
* aboxMailboxAttach - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

LOCAL STATUS tas6424Attach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;
    TAS6424_DATA *  pTas6424Data;
    VXB_RESOURCE *  pResMem = NULL;
    uint32_t        socVer;
    uint32_t *      pValue;
    int32_t         len;
    uint8_t         regVal;
    uint32_t        i;
    BOOL            rstDone;

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

    pTas6424Data= (TAS6424_DATA *) vxbMemAlloc (sizeof (TAS6424_DATA));
    if (pTas6424Data == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    vxbDevSoftcSet (pDev, pTas6424Data);
    pTas6424Data->pDev = pDev;

    pTas6424Data->i2cMutex = semMCreate (SEM_Q_PRIORITY
                                         | SEM_DELETE_SAFE
                                         | SEM_INVERSION_SAFE);
    if (pTas6424Data->i2cMutex == SEM_ID_NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pResMem = vxbResourceAlloc (pDev, VXB_RES_MEMORY, 0);
    if ((pResMem == NULL) || (pResMem->pRes == NULL))
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }
    pTas6424Data->i2cDevAddr =
        (UINT16)(((VXB_RESOURCE_ADR *)(pResMem->pRes))->virtAddr);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,abox_target",
                                      &len);
    if (pValue != NULL)
        {
        socVer = vxFdt32ToCpu (*pValue);
        if (socVer != ABOX_V920)
            {
            DBG_MSG (DBG_ERR, "\n");
            goto errOut;
            }
        }
    else
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "id", &len);
    if (pValue != NULL)
        {
        pTas6424Data->id = vxFdt32ToCpu (*pValue);
        }
    else
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pTas6424Data->standbyGpio = vxbGpioGetByFdtIndex (pDev,
                                                      "standby-gpios",
                                                      0);
    if (pTas6424Data->standbyGpio == ERROR)
        {
        (void) tca6416aConfigAmpStandby (pTas6424Data->id, 1);
        }
    else
        {

        /* STANDBY pin is active-low, set LOW to quit standby mode */

        (void) vxbGpioAlloc (pTas6424Data->standbyGpio);
        (void) vxbGpioSetDir (pTas6424Data->standbyGpio, GPIO_DIR_OUTPUT);
        (void) vxbGpioSetValue (pTas6424Data->standbyGpio, GPIO_VALUE_LOW);
        }

    pTas6424Data->muteGpio = vxbGpioGetByFdtIndex (pDev,
                                                   "mute-gpios",
                                                   0);
    if (pTas6424Data->muteGpio == ERROR)
        {
        tca6416aConfigAmpMute (pTas6424Data->id, TCA6416A_MUTE);
        }
    else
        {

        /* MUTE pin is active-low, set HIGH to mute */

        if (vxbGpioAlloc (pTas6424Data->muteGpio) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to alloc mute GPIO\n");
            goto errOut;
            }
        if (vxbGpioSetDir (pTas6424Data->muteGpio, GPIO_DIR_OUTPUT) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to set mute GPIO OUTPUT\n");
            goto errOut;
            }
        DBG_MSG (DBG_INFO, "set muteGpio[%d] to mute\n", pTas6424Data->muteGpio);
        if (vxbGpioSetValue (pTas6424Data->muteGpio, TAS6424_GPIO_MUTE) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to mute GPIO\n");
            goto errOut;
            }
        }

    /* reset device */

    if (tas6424RegRead (pDev, TAS6424_MODE_CTRL, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }
    regVal |= TAS6424_RESET;
    if (tas6424RegWrite (pDev, TAS6424_MODE_CTRL, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* disable diagnostics */

    if (tas6424RegRead (pDev, TAS6424_DC_DIAG_CTRL1, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }
    regVal |= TAS6424_LDGBYPASS_DISABLE;
    if (tas6424RegWrite (pDev, TAS6424_DC_DIAG_CTRL1, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    /* wait reset done */

    rstDone = FALSE;
    for (i = 0; i <= TAS6424_RESET_TIMEOUT; i++)
        {
        if (tas6424RegRead (pDev, TAS6424_MODE_CTRL, &regVal) == OK)
            {
            if ((regVal & TAS6424_RESET) == 0x0)
                {
                rstDone = TRUE;
                break;
                }
            }

        vxbUsDelay (1);
        }

    if (!rstDone)
        {
        DBG_MSG (DBG_ERR, "unable to reset codec\n");
        goto errOut;
        }

    /* disable diagnostics */

    for (i = TAS6424_DC_DIAG_CTRL1; i <= TAS6424_DC_DIAG_CTRL3; i++)
        {
        if (tas6424RegWrite (pDev, (uint8_t) i, 0x1) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to disable diagnostics %d\n", i);
            goto errOut;
            }
        }

    /* set channel volume */

    for (i = TAS6424_CH1_VOL_CTRL; i <= TAS6424_CH4_VOL_CTRL; i++)
        {
        if (tas6424RegWrite (pDev, (uint8_t)i, CH_VOL) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to set volume channel %d\n", i);
            goto errOut;
            }
        }

    /* set channels to line out mode */

    if (tas6424RegRead (pDev, TAS6424_MODE_CTRL, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }
    regVal |= (TAS6424_CH1_LO_MODE | TAS6424_CH2_LO_MODE |
               TAS6424_CH3_LO_MODE | TAS6424_CH4_LO_MODE);
    if (tas6424RegWrite (pDev, TAS6424_MODE_CTRL, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (tas6424RegWrite (pDev, TAS6424_PIN_CTRL, 0xff) != OK)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pTas6424Data->workState = TAS6424_W_INIT;
    pTas6424Data->isCancel = FALSE;
    SPIN_LOCK_ISR_INIT (&pTas6424Data->workSpinlockIsr, 0);
    pTas6424Data->faultCheckSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pTas6424Data->faultCheckSem == SEM_ID_NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    pTas6424Data->faultCheckWorkTask = taskSpawn (
                                                 TAS6424_FAULT_CHECK_TASK_NAME,
                                                 TAS6424_FAULT_CHECK_TASK_PRI,
                                                 0,
                                                 TAS6424_FAULT_CHECK_TASK_STACK,
                                                 (FUNCPTR) tas6424FaultCheckTask,
                                                 (_Vx_usr_arg_t) pTas6424Data,
                                                 0, 0, 0, 0, 0, 0, 0, 0, 0
                                                 );
    if (pTas6424Data->faultCheckWorkTask == TASK_ID_ERROR)
        {
        DBG_MSG (DBG_ERR, "\n");
        goto errOut;
        }

    if (vxSndCpntRegister (pDev,
                           &socCodecDevTas6424,
                           tas6424Dai,
                           NELEMENTS (tas6424Dai)) != OK)
        {
        DBG_MSG (DBG_ERR, "unable to register codec component\n");
        goto errOut;
        }

    DBG_MSG (DBG_INFO, "attach tas6424 successfully\n");
    tas6424RegDump (pDev);

    return OK;

errOut:
    if (pTas6424Data->faultCheckSem != SEM_ID_NULL)
        {
        (void) semDelete (pTas6424Data->faultCheckSem);
        }

    if (pTas6424Data->muteGpio != ERROR)
        {
        (void) vxbGpioFree (pTas6424Data->muteGpio);
        }

    if (pTas6424Data->standbyGpio != ERROR)
        {
        (void) vxbGpioFree (pTas6424Data->standbyGpio);
        }

    if (pResMem != NULL)
        {
        (void) vxbResourceFree (pDev, pResMem);
        }

    if (pTas6424Data->i2cMutex != SEM_ID_NULL)
        {
        (void) semDelete (pTas6424Data->i2cMutex);
        }

    if (pTas6424Data != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pTas6424Data);
        }

    return ERROR;
    }

LOCAL void tas6424RegDump
    (
    VXB_DEV_ID pDev
    )
    {
#ifdef TAS6424_DEBUG
    uint8_t idx;
    uint8_t regVal;

    for (idx = 0; idx <= 0x28; idx++)
        {
        (void) tas6424RegRead (pDev, (uint8_t) idx, &regVal);
        DBG_MSG (DBG_INFO, "reg[%02x] = 0x%02x\n", idx, regVal);
        vxbUsDelay (1);
        }

#endif
    return;
    }
