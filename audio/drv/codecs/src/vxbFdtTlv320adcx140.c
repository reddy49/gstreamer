/* vxbFdtTlv320adcx140.c - Tlv320adcx140 codec driver */

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
#include <vxbFdtTlv320adcx140.h>

#undef TLV320ADCX140_DEBUG
#ifdef TLV320ADCX140_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t tlv320adcx140DbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((tlv320adcx140DbgMask & (mask)) || ((mask) == DBG_ALL))     \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* TLV320ADCX140_DEBUG */

/* defines */

#define TLV320_RESET_DEACTIVE   GPIO_VALUE_LOW
#define TLV320_RESET_ACTIVE     GPIO_VALUE_HIGH

/* forward declarations */

IMPORT void vxbUsDelay (int delayTime);

LOCAL STATUS tlv320adcx140Probe (VXB_DEV_ID pDev);
LOCAL STATUS tlv320adcx140Attach (VXB_DEV_ID pDev);
LOCAL STATUS adcx140CodecCmpntProbe (VX_SND_SOC_COMPONENT * component);
LOCAL uint32_t tlv320adcx140CmpntRead (VX_SND_SOC_COMPONENT * cmpnt,
                                       uint32_t regOfs);
LOCAL STATUS tlv320adcx140CmpntWrite (VX_SND_SOC_COMPONENT * cmpnt,
                                       uint32_t regOfs, uint32_t val);
LOCAL STATUS adcx140HwParams (SND_PCM_SUBSTREAM * substream,
                               VX_SND_PCM_HW_PARAMS * params,
                               VX_SND_SOC_DAI * dai);
LOCAL STATUS adcx140SetDaiFmt (VX_SND_SOC_DAI * codecDai, uint32_t fmt);
LOCAL STATUS adcx140Mute (VX_SND_SOC_DAI * codecDai, int32_t mute,
                           int32_t stream);
LOCAL STATUS tlv320adcx140RegWrite (VXB_DEV_ID pDev, uint8_t regAddr,
                                    uint8_t pRegVal);
LOCAL void tlv320RegDump (VXB_DEV_ID pDev);

/* locals */

LOCAL const TLV320_REG_DEFAULT adcx140_reg_defaults[] =
    {
    { ADCX140_PAGE_SELECT, 0x00 },
    { ADCX140_SW_RESET, 0x00 },
    { ADCX140_SLEEP_CFG, 0x00 },
    { ADCX140_SHDN_CFG, 0x05 },
    { ADCX140_ASI_CFG0, 0x30 },
    { ADCX140_ASI_CFG1, 0x00 },
    { ADCX140_ASI_CFG2, 0x00 },
    { ADCX140_ASI_CH1, 0x00 },
    { ADCX140_ASI_CH2, 0x01 },
    { ADCX140_ASI_CH3, 0x02 },
    { ADCX140_ASI_CH4, 0x03 },
    { ADCX140_ASI_CH5, 0x04 },
    { ADCX140_ASI_CH6, 0x05 },
    { ADCX140_ASI_CH7, 0x06 },
    { ADCX140_ASI_CH8, 0x07 },
    { ADCX140_MST_CFG0, 0x02 },
    { ADCX140_MST_CFG1, 0x48 },
    { ADCX140_ASI_STS, 0xff },
    { ADCX140_CLK_SRC, 0x10 },
    { ADCX140_PDMCLK_CFG, 0x40 },
    { ADCX140_PDM_CFG, 0x00 },
    { ADCX140_GPIO_CFG0, 0x22 },
    { ADCX140_GPO_CFG0, 0x00 },
    { ADCX140_GPO_CFG1, 0x00 },
    { ADCX140_GPO_CFG2, 0x00 },
    { ADCX140_GPO_CFG3, 0x00 },
    { ADCX140_GPO_VAL, 0x00 },
    { ADCX140_GPIO_MON, 0x00 },
    { ADCX140_GPI_CFG0, 0x00 },
    { ADCX140_GPI_CFG1, 0x11 },
    { ADCX140_GPI_MON, 0x00 },
    { ADCX140_INT_CFG, 0x00 },
    { ADCX140_INT_MASK0, 0xff },
    { ADCX140_INT_LTCH0, 0x00 },
    { ADCX140_BIAS_CFG, 0x00 },
    { ADCX140_CH1_CFG0, 0x00 },
    { ADCX140_CH1_CFG1, 0x00 },
    { ADCX140_CH1_CFG2, 0xc9 },
    { ADCX140_CH1_CFG3, 0x80 },
    { ADCX140_CH1_CFG4, 0x00 },
    { ADCX140_CH2_CFG0, 0x00 },
    { ADCX140_CH2_CFG1, 0x00 },
    { ADCX140_CH2_CFG2, 0xc9 },
    { ADCX140_CH2_CFG3, 0x80 },
    { ADCX140_CH2_CFG4, 0x00 },
    { ADCX140_CH3_CFG0, 0x00 },
    { ADCX140_CH3_CFG1, 0x00 },
    { ADCX140_CH3_CFG2, 0xc9 },
    { ADCX140_CH3_CFG3, 0x80 },
    { ADCX140_CH3_CFG4, 0x00 },
    { ADCX140_CH4_CFG0, 0x00 },
    { ADCX140_CH4_CFG1, 0x00 },
    { ADCX140_CH4_CFG2, 0xc9 },
    { ADCX140_CH4_CFG3, 0x80 },
    { ADCX140_CH4_CFG4, 0x00 },
    { ADCX140_CH5_CFG2, 0xc9 },
    { ADCX140_CH5_CFG3, 0x80 },
    { ADCX140_CH5_CFG4, 0x00 },
    { ADCX140_CH6_CFG2, 0xc9 },
    { ADCX140_CH6_CFG3, 0x80 },
    { ADCX140_CH6_CFG4, 0x00 },
    { ADCX140_CH7_CFG2, 0xc9 },
    { ADCX140_CH7_CFG3, 0x80 },
    { ADCX140_CH7_CFG4, 0x00 },
    { ADCX140_CH8_CFG2, 0xc9 },
    { ADCX140_CH8_CFG3, 0x80 },
    { ADCX140_CH8_CFG4, 0x00 },
    { ADCX140_DSP_CFG0, 0x01 },
    { ADCX140_DSP_CFG1, 0x40 },
    { ADCX140_DRE_CFG0, 0x7b },
    { ADCX140_IN_CH_EN, 0xf0 },
    { ADCX140_ASI_OUT_CH_EN, 0x00 },
    { ADCX140_PWR_CFG, 0x00 },
    { ADCX140_DEV_STS0, 0x00 },
    { ADCX140_DEV_STS1, 0x80 },
    };

LOCAL VXB_DRV_METHOD tlv320adcx140MethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), tlv320adcx140Probe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), tlv320adcx140Attach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY tlv320adcx140Match[] =
    {
    {
    "ti,tlv320adc3140",
    NULL
    },
    {}                              /* empty terminated list */
    };

/* Digital Volume control. From -100 to 27 dB in 0.5 dB steps */
//LOCAL DECLARE_TLV_DB_SCALE (dig_vol_tlv, -10000, 50, 0);
LOCAL uint32_t dig_vol_tlv[] = {SNDRV_CTL_TLVT_DB_SCALE, 8, -10000, 0|50};

/* ADC gain. From 0 to 42 dB in 1 dB steps */
//LOCAL DECLARE_TLV_DB_SCALE (adc_tlv, 0, 100, 0);
LOCAL uint32_t adc_tlv[] = {SNDRV_CTL_TLVT_DB_SCALE, 8, 0, 0|100};
//resistor_text
LOCAL const char * const resistorText[] =
    {
    "2.5 kOhm", "10 kOhm", "20 kOhm"
    };

LOCAL SOC_ENUM_SINGLE_DECL (in1_resistor_enum, ADCX140_CH1_CFG0, 2,
                            resistorText);
LOCAL SOC_ENUM_SINGLE_DECL (in2_resistor_enum, ADCX140_CH2_CFG0, 2,
                            resistorText);
LOCAL SOC_ENUM_SINGLE_DECL (in3_resistor_enum, ADCX140_CH3_CFG0, 2,
                            resistorText);
LOCAL SOC_ENUM_SINGLE_DECL (in4_resistor_enum, ADCX140_CH4_CFG0, 2,
                            resistorText);

/* Analog/Digital Selection */
//adcx140_mic_sel_text
LOCAL const char * const adcx140MicSelText[] =
    {
    "Analog", "Line In", "Digital"
    };

//adcx140_analog_sel_text
LOCAL const char * const adcx140AnalogSelText[] =
    {
    "Analog", "Line In"
    };

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic1p_enum,
                            ADCX140_CH1_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic1_analog_enum,
                            ADCX140_CH1_CFG0, 7,
                            adcx140AnalogSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic1m_enum,
                            ADCX140_CH1_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic2p_enum,
                            ADCX140_CH2_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic2_analog_enum,
                            ADCX140_CH2_CFG0, 7,
                            adcx140AnalogSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic2m_enum,
                            ADCX140_CH2_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic3p_enum,
                            ADCX140_CH3_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic3_analog_enum,
                            ADCX140_CH3_CFG0, 7,
                            adcx140AnalogSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic3m_enum,
                            ADCX140_CH3_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic4p_enum,
                            ADCX140_CH4_CFG0, 5,
                            adcx140MicSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic4_analog_enum,
                            ADCX140_CH4_CFG0, 7,
                            adcx140AnalogSelText);

LOCAL SOC_ENUM_SINGLE_DECL (adcx140_mic4m_enum,
                            ADCX140_CH4_CFG0, 5,
                            adcx140MicSelText);

//adcx140_snd_controls
LOCAL const VX_SND_CONTROL adcx140SndControls[] =
    {
    SOC_SINGLE_TLV ("Analog CH1 Mic Gain Volume", ADCX140_CH1_CFG1, 2, 42, 0,
                    adc_tlv),
    SOC_SINGLE_TLV ("Analog CH2 Mic Gain Volume", ADCX140_CH1_CFG2, 2, 42, 0,
                    adc_tlv),
    SOC_SINGLE_TLV ("Analog CH3 Mic Gain Volume", ADCX140_CH1_CFG3, 2, 42, 0,
                    adc_tlv),
    SOC_SINGLE_TLV ("Analog CH4 Mic Gain Volume", ADCX140_CH1_CFG4, 2, 42, 0,
                    adc_tlv),

    SOC_SINGLE_TLV ("Digital CH1 Out Volume", ADCX140_CH1_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH2 Out Volume", ADCX140_CH2_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH3 Out Volume", ADCX140_CH3_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH4 Out Volume", ADCX140_CH4_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH5 Out Volume", ADCX140_CH5_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH6 Out Volume", ADCX140_CH6_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH7 Out Volume", ADCX140_CH7_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),
    SOC_SINGLE_TLV ("Digital CH8 Out Volume", ADCX140_CH8_CFG2,
                    0, 0xFF, 0, dig_vol_tlv),

    SOC_ENUM ("IN1 Analog Mic Resistor", in1_resistor_enum),
    SOC_ENUM ("IN2 Analog Mic Resistor", in2_resistor_enum),
    SOC_ENUM ("IN3 Analog Mic Resistor", in3_resistor_enum),
    SOC_ENUM ("IN4 Analog Mic Resistor", in4_resistor_enum),
    SOC_ENUM ("MIC1P Input Mux", adcx140_mic1p_enum),
    SOC_ENUM ("MIC1 Analog Mux", adcx140_mic1_analog_enum),
    SOC_ENUM ("MIC1M Input Mux", adcx140_mic1m_enum),
    SOC_ENUM ("MIC2P Input Mux", adcx140_mic2p_enum),
    SOC_ENUM ("MIC2 Analog MUX", adcx140_mic2_analog_enum),
    SOC_ENUM ("MIC2M Input Mux", adcx140_mic2m_enum),
    SOC_ENUM ("MIC3P Input Mux", adcx140_mic3p_enum),
    SOC_ENUM ("MIC3 Analog MUX", adcx140_mic3_analog_enum),
    SOC_ENUM ("MIC3M Input Mux", adcx140_mic3m_enum),
    SOC_ENUM ("MIC4P Input Mux", adcx140_mic4p_enum),
    SOC_ENUM ("MIC4 Analog MUX", adcx140_mic4_analog_enum),
    SOC_ENUM ("MIC4M Input Mux", adcx140_mic4m_enum),

    SOC_SINGLE ("CH1_ASI_EN", ADCX140_ASI_OUT_CH_EN, 7, 1, 0),
    SOC_SINGLE ("CH2_ASI_EN", ADCX140_ASI_OUT_CH_EN, 6, 1, 0),
    SOC_SINGLE ("CH3_ASI_EN", ADCX140_ASI_OUT_CH_EN, 5, 1, 0),
    SOC_SINGLE ("CH4_ASI_EN", ADCX140_ASI_OUT_CH_EN, 4, 1, 0),
    };

LOCAL STATUS adcx140ComponentPrepare
    (
    struct vxSndSocComponent *  component,
    SND_PCM_SUBSTREAM *         substream
    )
    {
    if (tlv320adcx140RegWrite (component->pDev,
                               (uint8_t) ADCX140_IN_CH_EN, 0xf0) != OK)
        {
        return ERROR;
        }
    return OK;
    }

LOCAL STATUS adcx140ComponentHwFree
    (
    struct vxSndSocComponent *  component,
    SND_PCM_SUBSTREAM *         substream
    )
    {
    if (tlv320adcx140RegWrite (component->pDev,
                               (uint8_t) ADCX140_IN_CH_EN, 0x00) != OK)
        {
        return ERROR;
        }
    return OK;
    }

//soc_codec_driver_adcx140
LOCAL const VX_SND_SOC_CMPT_DRV socCodecDriverAdcx140 =
    {
    .probe              = adcx140CodecCmpntProbe,
    .controls           = adcx140SndControls,
    .num_controls       = NELEMENTS (adcx140SndControls),
    .read               = tlv320adcx140CmpntRead,
    .write              = tlv320adcx140CmpntWrite,
    .prepare            = adcx140ComponentPrepare,
    .hwFree             = adcx140ComponentHwFree,
    };

//adcx140_dai_ops
LOCAL const VX_SND_SOC_DAI_OPS adcx140DaiOps =
    {
    .hwParams      = adcx140HwParams,
    .setFmt        = adcx140SetDaiFmt,
    .muteStream    = adcx140Mute,
    };

//adcx140_dai_driver
LOCAL VX_SND_SOC_DAI_DRIVER adcx140DaiDriver[] =
    {
    {
    .name = "tlv320adcx140-codec",
    .capture =
        {
        .streamName    = "Capture",
        .channelsMin   = 2,
        .channelsMax   = ADCX140_MAX_CHANNELS,
        .rates         = ADCX140_RATES,
        .formats       = ADCX140_FORMATS,
        },
    .ops = &adcx140DaiOps,
    }
    };

/* globals */

VXB_DRV tlv320adcx140Drv =
    {
    {NULL},
    TLV320ADCX140_DRV_NAME,             /* Name */
    "Tlv320adcx140 FDT driver",         /* Description */
    VXB_BUSID_FDT,                      /* Class */
    0,                                  /* Flags */
    0,                                  /* Reference count */
    tlv320adcx140MethodList             /* Method table */
    };

VXB_DRV_DEF(tlv320adcx140Drv);

LOCAL STATUS tlv320adcx140RegRead
    (
    VXB_DEV_ID              pDev,
    uint8_t                 regAddr,
    uint8_t *               pRegVal
    )
    {
    I2C_MSG                 msgs[2];
    int32_t                 num;
    TLV320ADCX140_DATA *    pTlv320adcx140Data;
    STATUS                  ret;

    pTlv320adcx140Data = (TLV320ADCX140_DATA *) vxbDevSoftcGet (pDev);
    if (pTlv320adcx140Data == NULL)
        {
        return ERROR;
        }

    (void) semTake (pTlv320adcx140Data->i2cMutex, WAIT_FOREVER);

    memset (msgs, 0, sizeof (I2C_MSG) * 2);
    num = 0;

    /* write reg address */

    msgs[num].addr  = pTlv320adcx140Data->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_WR;
    msgs[num].buf   = &regAddr;
    msgs[num].len   = 1;
    num++;

    /* read reg data */

    msgs[num].addr  = pTlv320adcx140Data->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_RD;
    msgs[num].buf   = pRegVal;
    msgs[num].len   = 1;
    num++;

    ret = vxbI2cDevXfer (pDev, &msgs[0], num);

    (void) semGive (pTlv320adcx140Data->i2cMutex);

    return ret;
    }

LOCAL STATUS tlv320adcx140RegWrite
    (
    VXB_DEV_ID              pDev,
    uint8_t                 regAddr,
    uint8_t                 pRegVal
    )
    {
    I2C_MSG                 msg;
    uint8_t                 msgBuf[2];
    TLV320ADCX140_DATA *    pTlv320adcx140Data;
    STATUS                  ret;

    pTlv320adcx140Data = (TLV320ADCX140_DATA *) vxbDevSoftcGet (pDev);
    if (pTlv320adcx140Data == NULL)
        {
        DBG_MSG (DBG_ERR, "pTlv320adcx140Data is NULL\n");
        return ERROR;
        }

    (void) semTake (pTlv320adcx140Data->i2cMutex, WAIT_FOREVER);

    memset (&msg, 0, sizeof (I2C_MSG));

    msgBuf[0] = regAddr;
    msgBuf[1] = pRegVal;

    /* write reg address and data */

    msg.addr    = pTlv320adcx140Data->i2cDevAddr;
    msg.scl     = 0;        /* use controller's default scl */
    msg.flags   = I2C_M_WR;
    msg.buf     = msgBuf;
    msg.len     = 2;

    ret = vxbI2cDevXfer (pDev, &msg, 1);

    (void) semGive (pTlv320adcx140Data->i2cMutex);

    return ret;
    }

LOCAL uint32_t tlv320adcx140CmpntRead
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs
    )
    {
    uint8_t                 regVal = 0;

    if (tlv320adcx140RegRead (cmpnt->pDev, (uint8_t) regOfs, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s read reg[%02xh] error\n", cmpnt->name, regOfs);
        }

    return regVal;
    }

LOCAL STATUS tlv320adcx140CmpntWrite
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs,
    uint32_t                val
    )
    {
    if (tlv320adcx140RegWrite (cmpnt->pDev, (uint8_t) regOfs,
                               (uint8_t) val) != OK)
        {
        DBG_MSG (DBG_ERR, "%s write reg[%02xh] = 0x%02x error\n", cmpnt->name,
                (uint8_t) regOfs, (uint8_t) val);
        return ERROR;
        }

    return OK;
    }

//adcx140_enable_outch
LOCAL STATUS adcx140EnaOutch
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data,
    uint32_t                chnum,
    BOOL                    enabled
    )
    {
    uint8_t                 regVal = 0;

    /* since there are only 4 D-MIC in discovery board */

    if (enabled)
        {
        if (chnum == 2)
            {
            regVal = ASI_OUT_CH1_EN | ASI_OUT_CH2_EN;
            }
        else if (chnum == 4)
            {
            regVal = ASI_OUT_CH1_EN | ASI_OUT_CH2_EN
                     | ASI_OUT_CH3_EN | ASI_OUT_CH4_EN;
            }
        else
            {
            DBG_MSG (DBG_ERR, "[%s] unsupport out channel num\n", __func__);
            return ERROR;
            }
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_ASI_OUT_CH_EN, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_ASI_OUT_CH_EN failed\n",
                 __func__);
        return ERROR;
        }

    return OK;
    }

//adcx410_on
LOCAL STATUS adcx410On
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data,
    BOOL                    enabled
    )
    {
    uint8_t                 chEnRegVal = 0;
    uint8_t                 pwdRegVal = 0;

    DBG_MSG (DBG_ERR, "[%s]\n", __func__);

    if (enabled)
        {
        chEnRegVal = ADCX140_IN_CH1_EN | ADCX140_IN_CH2_EN
                     | ADCX140_IN_CH3_EN | ADCX140_IN_CH4_EN;
        pwdRegVal = ADCx140_PWR_CFG_PLL_PDZ | ADCx140_PWR_CFG_ADC_PDZ;
        }

    /* Enable input channels : Input Channel 1~4 */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_IN_CH_EN, chEnRegVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_IN_CH_EN failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_PWR_CFG, pwdRegVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_PWR_CFG failed\n", __func__);
        return ERROR;
        }

    return OK;
    }

//adcx140_bclk_config
LOCAL STATUS adcx140BclkConfig
    (
    SND_PCM_SUBSTREAM *     substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai)
    {
    VX_SND_SOC_COMPONENT *  component;
    TLV320ADCX140_DATA *    pTlv320adcx140Data;
    uint32_t                rate;
    int32_t                 width;
    uint32_t                ch;
    uint32_t                fs_bclk_ratio;
    uint8_t                 data;

    component = dai->component;
    pTlv320adcx140Data = (TLV320ADCX140_DATA *)
                             vxbDevSoftcGet (component->pDev);
    rate = PARAMS_RATE (params);
    width = PARAMS_WIDTH (params);
    ch = PARAMS_CHANNELS (params);
    fs_bclk_ratio = width * ch;

    switch (rate)
        {
        case 8000:
            data = FS_RATE_8KHZ;
            break;

        case 16000:
            data = FS_RATE_16KHZ;
            break;

        case 24000:
            data = FS_RATE_24KHZ;
            break;

        case 32000:
            data = FS_RATE_32KHZ;
            break;

        case 48000:
            data = FS_RATE_48KHZ;
            break;

        case 96000:
            data = FS_RATE_96KHZ;
            break;

        case 192000:
            data = FS_RATE_192KHZ;
            break;

        case 384000:
            data = FS_RATE_384KHZ;
            break;

        case 768000:
            data = FS_RATE_768KHZ;
            break;

        default:
            DBG_MSG (DBG_ERR, "unsupported sample rate: %u\n", rate);
            return ERROR;
        }

    switch (fs_bclk_ratio)
        {
        case 16:
            data |= RATIO_OF_16;
            break;

        case 24:
            data |= RATIO_OF_24;
            break;

        case 32:
            data |= RATIO_OF_32;
            break;

        case 48:
            data |= RATIO_OF_48;
            break;

        case 64:
            data |= RATIO_OF_64;
            break;

        case 96:
            data |= RATIO_OF_96;
            break;

        case 128:
            data |= RATIO_OF_128;
            break;

        case 192:
            data |= RATIO_OF_192;
            break;

        case 256:
            data |= RATIO_OF_256;
            break;

        case 384:
            data |= RATIO_OF_384;
            break;

        case 512:
            data |= RATIO_OF_512;
            break;

        case 1024:
            data |= RATIO_OF_1024;
            break;

        case 2048:
            data |= RATIO_OF_2048;
            break;

        default:
            DBG_MSG (DBG_ERR, "%s: Unsupported ratio %d\n",
                     __func__, fs_bclk_ratio);
            return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_MST_CFG1, data) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_MST_CFG1 failed\n", __func__);
        return ERROR;
        }

    if (adcx140EnaOutch (pTlv320adcx140Data, ch, TRUE) != OK)
        {
        return ERROR;
        }

    if (adcx410On (pTlv320adcx140Data, TRUE) != OK)
        {
        return ERROR;
        }

    return OK;
    }

//adcx410_amic_cfg
LOCAL STATUS adcx410AmicCfg
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {
    uint8_t                 regVal;

    /*
     * 1. Channel configuration
     * PDM 8-Channel : PDMDIN1 - Ch1 and Ch2, PDMDIN2 - Ch3 and Ch4
     * PDMDIN3 - Ch5 and Ch6, PDMDIN4 - Ch7 and Ch8
     */

    /* PDMDIN1 */

    regVal = LINE_INPUT | AMIC_PDM_INPUT;
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG0 failed\n", __func__);
        return ERROR;
        }

    /* PDMDIN2 */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG0 failed\n", __func__);
        return ERROR;
        }

    /* Gain set to 25db(0x64) for Ch1/Ch2/Ch3/Ch4 */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG1, 0x64) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG1 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG1, 0x64) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG1 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG1, 0x64) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG1 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG1, 0x64) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG1 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG2, 0xC9) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG2, 0xC9) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG2, 0xC9) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG2, 0xC9) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG2 failed\n", __func__);
        return ERROR;
        }

    /*
     * 2. PDM clock configuration and GPO/GPI
     */

    /* PDM clock generation configuraiton */

    regVal = 0x40 | DMIC_PDMCLK_1536KHZ;
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_PDMCLK_CFG, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_PDMCLK_CFG failed\n", __func__);
        return ERROR;
        }

    /* Configure GPO_CFG0(0x21)/GPO_CFG1(0x22) */

    regVal = 0x01; //PDMCLK, Drive active low and active high
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPO_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPO_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPO_CFG1, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPO_CFG1 failed\n", __func__);
        return ERROR;
        }

    /* configure GPI_CFG0(0x00) */

    regVal = 0x00; // disable PDM
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPI_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPI_CFG0 failed\n", __func__);
        return ERROR;
        }

    /* configure ADC_FSCALE and MBIAS */

    regVal = (ADCX140_MIC_BIAS_VAL_AVDD << 4) | ADCX140_MIC_BIAS_VAL_ADC_FS;
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_BIAS_CFG, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_BIAS_CFG failed\n", __func__);
        return ERROR;
        }

    return OK;
    }

//adcx410_dmic_cfg
LOCAL STATUS adcx410DmicCfg
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {
    uint8_t                 regVal;

    /*
     * 1. Channel configuration
     * PDM 8-Channel : PDMDIN1 - Ch1 and Ch2, PDMDIN2 - Ch3 and Ch4
     * PDMDIN3 - Ch5 and Ch6, PDMDIN4 - Ch7 and Ch8
     */

    /* PDMDIN1 */

    regVal = LINE_INPUT | DMIC_PDM_INPUT;
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG0 failed\n", __func__);
        return ERROR;
        }

    /* PDMDIN2 */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG0 failed\n", __func__);
        return ERROR;
        }

    /* Digital volume control is set to 21db(0xF3) for Ch1/Ch2/Ch3/Ch4 */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG2, 0xF3) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG2, 0xF3) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG2, 0xF3) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG2 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG2, 0xF3) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG2 failed\n", __func__);
        return ERROR;
        }

    /*
     * 2. PDM clock configuration and GPO/GPI
     */

    /* PDM clock generation configuraiton */

    regVal = 0x40 | DMIC_PDMCLK_1536KHZ;
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_PDMCLK_CFG, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_PDMCLK_CFG failed\n", __func__);
        return ERROR;
        }

    /* Configure GPO_CFG0(0x21)/GPO_CFG1(0x22) */

    regVal = 0x41; //PDMCLK, Drive active low and active high
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPO_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPO_CFG0 failed\n", __func__);
        return ERROR;
        }
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPO_CFG1, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPO_CFG1 failed\n", __func__);
        return ERROR;
        }

    /* configure GPI_CFG0(0x2B) */

    regVal = 0x45; //PDMDIN1, PDMDIN2
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPI_CFG0, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPI_CFG0 failed\n", __func__);
        return ERROR;
        }

    return OK;
    }

//adcx410_mictype_cfg
LOCAL STATUS adcx410MictypeCfg
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {
    uint8_t                 regVal;

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPI_CFG1, 0x11) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPI_CFG1 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegRead (pTlv320adcx140Data->pDev,
                              ADCX140_GPI_MON, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: read ADCX140_GPI_MON failed\n", __func__);
        return ERROR;
        }

    if (((regVal >> 4) & 0x01) == A_MIC)
        {
        DBG_MSG (DBG_INFO, "A_MIC detected\n");

        return adcx410AmicCfg (pTlv320adcx140Data);
        }
    else
        {
        DBG_MSG (DBG_INFO, "D_MIC detected\n");

        return adcx410DmicCfg (pTlv320adcx140Data);
        }
    }


//adcx140_hw_params
LOCAL STATUS adcx140HwParams
    (
    SND_PCM_SUBSTREAM *     substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    TLV320ADCX140_DATA *    pTlv320adcx140Data;
    uint8_t                 data;

    DBG_MSG (DBG_ERR, "%s\n", __func__);

    component = dai->component;
    pTlv320adcx140Data = (TLV320ADCX140_DATA *)
                             vxbDevSoftcGet (component->pDev);

    switch (PARAMS_WIDTH (params))
        {
        case 16:
            data = ADCX140_16_BIT_WORD;
            break;

        case 20:
            data = ADCX140_20_BIT_WORD;
            break;

        case 24:
            data = ADCX140_24_BIT_WORD;
            break;

        case 32:
            data = ADCX140_32_BIT_WORD;
            break;

        default:
            DBG_MSG (DBG_ERR, "%s: Unsupported width %d\n",
                     __func__, PARAMS_WIDTH (params));
            return ERROR;
        }

    if (vxSndSocComponentUpdate (component, ADCX140_ASI_CFG0,
                                    ADCX140_WORD_LEN_MSK, data) != OK)
        {
        DBG_MSG (DBG_ERR, "write ADCX140_ASI_CFG0 failed\n");
        return ERROR;
        }

    if (adcx140BclkConfig (substream, params, dai) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to config bclk\n");
        return ERROR;
        }

    if (adcx410MictypeCfg (pTlv320adcx140Data) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to config mic type\n");
        return ERROR;
        }

    return OK;
    }

//adcx140_set_dai_fmt
LOCAL STATUS adcx140SetDaiFmt
    (
    VX_SND_SOC_DAI *        codecDai,
    uint32_t                fmt
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    uint8_t                 ifaceReg1 = 0;
    uint8_t                 ifaceReg2 = 0;

    DBG_MSG (DBG_INFO, "%s\n", __func__);

    component = codecDai->component;

    /* set master/slave audio interface */
    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK)
        {
        case SND_SOC_DAIFMT_CBM_CFM:
            ifaceReg2 |= ADCX140_BCLK_FSYNC_MASTER;
            break;

        case SND_SOC_DAIFMT_CBS_CFS:
            break;

        case SND_SOC_DAIFMT_CBS_CFM:
        case SND_SOC_DAIFMT_CBM_CFS:
        default:
            DBG_MSG (DBG_ERR, "Invalid DAI master/slave interface\n");
            return ERROR;
        }

    /* signal polarity */
    switch (fmt & SND_SOC_DAIFMT_INV_MASK)
        {
        case SND_SOC_DAIFMT_NB_IF:
            ifaceReg1 |= ADCX140_FSYNCINV_BIT;
            break;

        case SND_SOC_DAIFMT_IB_IF:
            ifaceReg1 |= ADCX140_BCLKINV_BIT;
            ifaceReg1 |= ADCX140_FSYNCINV_BIT;
            break;

        case SND_SOC_DAIFMT_IB_NF:
            ifaceReg1 |= ADCX140_BCLKINV_BIT;
            break;

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
            ifaceReg1 |= ADCX140_I2S_MODE_BIT;
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH1,
                                            ASI_CH_MASK, 0x00);
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH2,
                                            ASI_CH_MASK, 0x20);
            break;

        case SND_SOC_DAIFMT_LEFT_J:
            ifaceReg1 |= ADCX140_LEFT_JUST_BIT;
            break;

        case VX_SND_DAI_FORMAT_DSP_A:
        case VX_SND_DAI_FORMAT_DSP_B:
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH1,
                                            ASI_CH_MASK, 0x00);
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH2,
                                            ASI_CH_MASK, 0x01);
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH3,
                                            ASI_CH_MASK, 0x02);
            (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CH4,
                                            ASI_CH_MASK, 0x03);
            break;

        default:
            DBG_MSG (DBG_ERR, "Invalid DAI interface format\n");
            return ERROR;
        }

    //Channel summation for higher SNR
    (void) vxSndSocComponentUpdate (component, ADCX140_DSP_CFG0,
                                    CH_SUM_MASK, FOUR_CH_SUM);
    (void) vxSndSocComponentUpdate (component, ADCX140_ASI_CFG0,
                                    ADCX140_INV_MSK | ADCX140_ASI_FORMAT_MSK,
                                    ifaceReg1);
    (void) vxSndSocComponentUpdate (component, ADCX140_MST_CFG0,
                                    ADCX140_BCLK_FSYNC_MASTER, ifaceReg2);

    return OK;
    }

//adcx140_mute
LOCAL STATUS adcx140Mute
    (
    VX_SND_SOC_DAI *        codecDai,
    int32_t                 mute,
    int32_t                 stream
    )
    {
    VX_SND_SOC_COMPONENT *  component;
    uint32_t                configReg;
    uint32_t                micEnable;
    uint32_t                i;

    component = codecDai->component;

    micEnable = vxSndSocComponentRead (component, ADCX140_IN_CH_EN);
    if (micEnable == 0)
        {
        return OK;
        }

    /* mute the channel if it was enabled */

    for (i = 0; i < ADCX140_MAX_CHANNELS; i++)
        {
        configReg = ADCX140_CH8_CFG2 - (5 * i);
        if ((micEnable & (0x1 << i)) == 0)
            {
            continue;
            }

        if (mute)
            {
            (void) vxSndSocComponentWrite (component, configReg, 0);
            }
        }

    return OK;
    }

//adcx140_reset
LOCAL STATUS adcx140Reset
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {
    if (pTlv320adcx140Data->resetGpio != ERROR)
        {
        /* RESET pin is active-low, set HIGH to enter reset mode */

        (void) vxbGpioSetValue (pTlv320adcx140Data->resetGpio,
                                TLV320_RESET_ACTIVE);

        vxbUsDelay (100000);

        /* RESET pin is active-low, set LOW to quit reset mode */

        (void) vxbGpioSetValue (pTlv320adcx140Data->resetGpio,
                                TLV320_RESET_DEACTIVE);
        }
    else
        {
        if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                                   ADCX140_SW_RESET, ADCX140_RESET) != OK)
            {
            DBG_MSG (DBG_ERR, "%s: write ADCX140_SW_RESET failed\n", __func__);
            return ERROR;
            }
        }

    vxbUsDelay (100000);

    return OK;
    }

//adcx410_master
LOCAL STATUS adcx410Master
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data,
    BOOL                    enabled
    )
    {
    uint8_t                 gpioRegVal = 0;
    uint8_t                 mstCfg0RegVal = 0;
    uint8_t                 mstCfg1RegVal = 0;

    DBG_MSG (DBG_INFO, "%s enabled:%d\n", __func__, enabled);

    if (enabled)
        {

        /*
         * 1. configure GPIO1 as MCLK input
         * ADCX140_GPIO_CFG0(0x21)
         * GPIO1_CFG[7:4] : 0xA - GPIO1 is configued as a master clock input
         * GPIO1_DRV[2:0] : 0x2 - Driver active low and weak high
         */

        gpioRegVal = 0xA2;

        /*
         * 2. configure device as master with MCLK = 24.576 MHz
         * ADCX140_MST_CFG0(0x13)
         * MST_SLV_CFG[7] : 1b - master mode
         * FS_MODE[3] : 0b - fs is a multiple of 48KHz
         * MCLK_FREQ_SEL[2:0] : 111b - 24.576MHz
         *
         */

        mstCfg0RegVal = 0x87;

        /*
         * 3. configure device's master config1
         * ADCX140_MST_CFG1(0x14)
         * FS_RATE[7:4] : 0x4 - 44.1KHz or 48KHz
         * FS_BCLK_RATIO[3:0] 0x2 - Ratio of 32
         */

        mstCfg1RegVal = 0x42;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_GPIO_CFG0, gpioRegVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_GPIO_CFG0 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_MST_CFG0, mstCfg0RegVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_MST_CFG0 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_MST_CFG1, mstCfg1RegVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_MST_CFG1 failed\n", __func__);
        return ERROR;
        }

    return OK;
    }

//adcx140_pga_cfg
LOCAL STATUS adcx140PgaCfg
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {

    DBG_MSG (DBG_INFO, "[%s]\n", __func__);

    /* Set each pga of channels with 12db */

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH1_CFG1, 0x78) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH1_CFG1 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH2_CFG1, 0x78) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH2_CFG1 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH3_CFG1, 0x78) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH3_CFG1 failed\n", __func__);
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_CH4_CFG1, 0x78) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_CH4_CFG1 failed\n", __func__);
        return ERROR;
        }

    return OK;
    }

//adcx410_init
LOCAL STATUS adcx410Init
    (
    TLV320ADCX140_DATA *    pTlv320adcx140Data
    )
    {
    uint8_t                 sleepCfgVal;
    uint8_t                 biasSource;
    VXB_FDT_DEV *           pFdtDev;
    uint32_t *              pValue;
    int32_t                 len;
    uint8_t                 regVal;

    DBG_MSG (DBG_INFO, "%s\n", __func__);

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pTlv320adcx140Data->pDev));
    if (pFdtDev == NULL)
        {
        return ERROR;
        }

    sleepCfgVal = ADCX140_WAKE_DEV;
    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "ti,use-internal-areg",
                                        &len);
    if (pValue != NULL)
        {
        sleepCfgVal |= ADCX140_AREG_INTERNAL;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "ti,mic-bias-source",
                                        &len);
    if (pValue != NULL)
        {
        biasSource = (uint8_t) vxFdt32ToCpu (*pValue);
        }
    else
        {
        biasSource = ADCX140_MIC_BIAS_VAL_VREF;
        }

    if (biasSource != ADCX140_MIC_BIAS_VAL_VREF
        && biasSource != ADCX140_MIC_BIAS_VAL_VREF_1096
        && biasSource != ADCX140_MIC_BIAS_VAL_AVDD)
        {
        DBG_MSG (DBG_ERR, "Mic Bias source value is invalid\n");
        return ERROR;
        }

    if (adcx140Reset (pTlv320adcx140Data) != OK)
        {
        return ERROR;
        }

    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_SLEEP_CFG, sleepCfgVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_SLEEP_CFG failed\n", __func__);
        return ERROR;
        }

    vxbUsDelay (10000);

    if (tlv320adcx140RegRead (pTlv320adcx140Data->pDev,
                              ADCX140_BIAS_CFG, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: read ADCX140_BIAS_CFG failed\n", __func__);
        return ERROR;
        }
    regVal &= ~ADCX140_MIC_BIAS_VAL_MSK;
    regVal |= (biasSource << ADCX140_MIC_BIAS_VAL_SHIFT);
    if (tlv320adcx140RegWrite (pTlv320adcx140Data->pDev,
                               ADCX140_BIAS_CFG, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "%s: write ADCX140_BIAS_CFG failed\n", __func__);
        return ERROR;
        }

    if (adcx410Master (pTlv320adcx140Data, TRUE) != OK)
        {
        return ERROR;
        }

    if (adcx410MictypeCfg (pTlv320adcx140Data) != OK)
        {
        return ERROR;
        }

    if (adcx140PgaCfg (pTlv320adcx140Data) != OK)
        {
        return ERROR;
        }

    return OK;
    }

//adcx140_codec_probe
LOCAL STATUS adcx140CodecCmpntProbe
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    TLV320ADCX140_DATA *    pTlv320adcx140Data;

    DBG_MSG (DBG_INFO, "%s() is called\n", __func__);

    pTlv320adcx140Data = (TLV320ADCX140_DATA *)
                             vxbDevSoftcGet (component->pDev);

    if (adcx410Init (pTlv320adcx140Data) != OK)
        {
        return ERROR;
        }

    return OK;
    }

/******************************************************************************
*
* tlv320adcx140Probe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS tlv320adcx140Probe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, tlv320adcx140Match, NULL);
    }

/******************************************************************************
*
* tlv320adcx140Attach - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

LOCAL STATUS tlv320adcx140Attach
    (
    VXB_DEV_ID              pDev
    )
    {
    TLV320ADCX140_DATA *    pTlv320adcx140Data;
    VXB_RESOURCE *          pResMem = NULL;
    uint32_t                idx;

    if (pDev == NULL)
        {
        return ERROR;
        }

    pTlv320adcx140Data=
        (TLV320ADCX140_DATA *) vxbMemAlloc (sizeof (TLV320ADCX140_DATA));
    if (pTlv320adcx140Data == NULL)
        {
        return ERROR;
        }

    vxbDevSoftcSet (pDev, pTlv320adcx140Data);
    pTlv320adcx140Data->pDev = pDev;

    pTlv320adcx140Data->i2cMutex = semMCreate (SEM_Q_PRIORITY
                                               | SEM_DELETE_SAFE
                                               | SEM_INVERSION_SAFE);
    if (pTlv320adcx140Data->i2cMutex == SEM_ID_NULL)
        {
        goto errOut;
        }

    pResMem = vxbResourceAlloc (pDev, VXB_RES_MEMORY, 0);
    if ((pResMem == NULL) || (pResMem->pRes == NULL))
        {
        goto errOut;
        }
    pTlv320adcx140Data->i2cDevAddr =
        (UINT16)(((VXB_RESOURCE_ADR *)(pResMem->pRes))->virtAddr);

    DBG_MSG (DBG_INFO, "i2cDevAddr is 0x%02x\n", pTlv320adcx140Data->i2cDevAddr);

    pTlv320adcx140Data->resetGpio = vxbGpioGetByFdtIndex (pDev,
                                                          "reset",
                                                          0);
    if (pTlv320adcx140Data->resetGpio == ERROR)
        {
        DBG_MSG (DBG_ERR, "failed to get <reset> GPIO from dtb\n");
        goto errOut;
        }
    else
        {

        /* RESET pin is active-low, set LOW to quit reset mode */

        if (vxbGpioAlloc (pTlv320adcx140Data->resetGpio) != OK ||
            vxbGpioSetDir (pTlv320adcx140Data->resetGpio,
                           GPIO_DIR_OUTPUT) != OK ||
            vxbGpioSetValue (pTlv320adcx140Data->resetGpio,
                             TLV320_RESET_DEACTIVE) != OK)
            {
            DBG_MSG (DBG_ERR, "failed to configure reset GPIO\n");
            goto errOut;
            }
        }

    vxbUsDelay (100000);

    /* write initial values to registers */

    for (idx = 0; idx < NELEMENTS (adcx140_reg_defaults); idx++)
        {
        if (tlv320adcx140RegWrite (pDev,
                                   (uint8_t) adcx140_reg_defaults[idx].reg,
                                   (uint8_t) adcx140_reg_defaults[idx].def)
                                   != OK)
            {
            DBG_MSG (DBG_ERR, "failed to write reg[%02xh] = 0x%02x\n",
                     (uint8_t) adcx140_reg_defaults[idx].reg,
                     (uint8_t) adcx140_reg_defaults[idx].def);
            goto errOut;
            }
        }

    tlv320RegDump (pDev);

    if (vxSndCpntRegister (pDev, &socCodecDriverAdcx140, adcx140DaiDriver,
                           NELEMENTS (adcx140DaiDriver)) != OK)
        {
        DBG_MSG (DBG_ERR, "unable to register codec component\n");
        goto errOut;
        }

    return OK;

errOut:
    if (pTlv320adcx140Data->resetGpio != ERROR)
        {
        (void) vxbGpioFree (pTlv320adcx140Data->resetGpio);
        }

    if (pResMem != NULL)
        {
        (void) vxbResourceFree (pDev, pResMem);
        }

    if (pTlv320adcx140Data->i2cMutex != SEM_ID_NULL)
        {
        (void) semDelete (pTlv320adcx140Data->i2cMutex);
        }

    if (pTlv320adcx140Data != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pTlv320adcx140Data);
        }

    return ERROR;
    }

LOCAL void tlv320RegDump
    (
    VXB_DEV_ID pDev
    )
    {
#ifdef TLV320ADCX140_DEBUG
    uint8_t idx;
    uint8_t regVal;

    for (idx = 0; idx <= ADCX140_DEV_STS1; idx++)
        {
        (void) tlv320adcx140RegRead (pDev, idx, &regVal);
        DBG_MSG (DBG_INFO, "reg[%02xh] = 0x%02x\n", idx, regVal);
        vxbUsDelay (1);
        }
#endif
    return;
    }

