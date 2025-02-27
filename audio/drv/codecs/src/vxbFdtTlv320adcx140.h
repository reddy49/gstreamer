/* vxbFdtTlv320adcx140.h - Tlv320adcx140 codec driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Tlv320adcx140 codec driver specific definitions.
*/

#ifndef __INCvxbFdtTlv320adcx140h
#define __INCvxbFdtTlv320adcx140h

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TLV320ADCX140_DRV_NAME   "tlv320adcx140"

#define ADCX140_MAX_CHANNELS    (8)

#define ADCX140_RATES           (SNDRV_PCM_RATE_44100 | \
                                 SNDRV_PCM_RATE_48000)

#define ADCX140_FORMATS         (VX_SND_FMTBIT_S16_LE | \
                                 VX_SND_FMTBIT_S20_3LE | \
                                 VX_SND_FMTBIT_S24_3LE | \
                                 VX_SND_FMTBIT_S24_LE | \
                                 VX_SND_FMTBIT_S32_LE)

/* registers */

#define ADCX140_PAGE_SELECT     (0x00)
#define ADCX140_SW_RESET        (0x01)
#define ADCX140_SLEEP_CFG       (0x02)
#define ADCX140_SHDN_CFG        (0x05)
#define ADCX140_ASI_CFG0        (0x07)
#define ADCX140_ASI_CFG1        (0x08)
#define ADCX140_ASI_CFG2        (0x09)
#define ADCX140_ASI_CH1         (0x0b)
#define ADCX140_ASI_CH2         (0x0c)
#define ADCX140_ASI_CH3         (0x0d)
#define ADCX140_ASI_CH4         (0x0e)
#define ADCX140_ASI_CH5         (0x0f)
#define ADCX140_ASI_CH6         (0x10)
#define ADCX140_ASI_CH7         (0x11)
#define ADCX140_ASI_CH8         (0x12)
#define ADCX140_MST_CFG0        (0x13)
#define ADCX140_MST_CFG1        (0x14)
#define ADCX140_ASI_STS         (0x15)
#define ADCX140_CLK_SRC         (0x16)
#define ADCX140_PDMCLK_CFG      (0x1f)
#define ADCX140_PDM_CFG         (0x20)
#define ADCX140_GPIO_CFG0       (0x21)
#define ADCX140_GPO_CFG0        (0x22)
#define ADCX140_GPO_CFG1        (0x23)
#define ADCX140_GPO_CFG2        (0x24)
#define ADCX140_GPO_CFG3        (0x25)
#define ADCX140_GPO_VAL         (0x29)
#define ADCX140_GPIO_MON        (0x2a)
#define ADCX140_GPI_CFG0        (0x2b)
#define ADCX140_GPI_CFG1        (0x2c)
#define ADCX140_GPI_MON         (0x2f)
#define ADCX140_INT_CFG         (0x32)
#define ADCX140_INT_MASK0       (0x33)
#define ADCX140_INT_LTCH0       (0x36)
#define ADCX140_BIAS_CFG        (0x3b)
#define ADCX140_CH1_CFG0        (0x3c)
#define ADCX140_CH1_CFG1        (0x3d)
#define ADCX140_CH1_CFG2        (0x3e)
#define ADCX140_CH1_CFG3        (0x3f)
#define ADCX140_CH1_CFG4        (0x40)
#define ADCX140_CH2_CFG0        (0x41)
#define ADCX140_CH2_CFG1        (0x42)
#define ADCX140_CH2_CFG2        (0x43)
#define ADCX140_CH2_CFG3        (0x44)
#define ADCX140_CH2_CFG4        (0x45)
#define ADCX140_CH3_CFG0        (0x46)
#define ADCX140_CH3_CFG1        (0x47)
#define ADCX140_CH3_CFG2        (0x48)
#define ADCX140_CH3_CFG3        (0x49)
#define ADCX140_CH3_CFG4        (0x4a)
#define ADCX140_CH4_CFG0        (0x4b)
#define ADCX140_CH4_CFG1        (0x4c)
#define ADCX140_CH4_CFG2        (0x4d)
#define ADCX140_CH4_CFG3        (0x4e)
#define ADCX140_CH4_CFG4        (0x4f)
#define ADCX140_CH5_CFG2        (0x52)
#define ADCX140_CH5_CFG3        (0x53)
#define ADCX140_CH5_CFG4        (0x54)
#define ADCX140_CH6_CFG2        (0x57)
#define ADCX140_CH6_CFG3        (0x58)
#define ADCX140_CH6_CFG4        (0x59)
#define ADCX140_CH7_CFG2        (0x5c)
#define ADCX140_CH7_CFG3        (0x5d)
#define ADCX140_CH7_CFG4        (0x5e)
#define ADCX140_CH8_CFG2        (0x61)
#define ADCX140_CH8_CFG3        (0x62)
#define ADCX140_CH8_CFG4        (0x63)
#define ADCX140_DSP_CFG0        (0x6b)
#define ADCX140_DSP_CFG1        (0x6c)
#define ADCX140_DRE_CFG0        (0x6d)
#define ADCX140_IN_CH_EN        (0x73)
#define ADCX140_ASI_OUT_CH_EN   (0x74)
#define ADCX140_PWR_CFG         (0x75)
#define ADCX140_DEV_STS0        (0x76)
#define ADCX140_DEV_STS1        (0x77)

#define ADCX140_16_BIT_WORD     (0x0)
#define ADCX140_20_BIT_WORD     (0x1 << 4)
#define ADCX140_24_BIT_WORD     (0x1 << 5)
#define ADCX140_32_BIT_WORD     ((0x1 << 4) | (0x1 << 5))
#define ADCX140_WORD_LEN_MSK    (0x30)

#define FS_RATE_8KHZ            (0 << 4)
#define FS_RATE_16KHZ           (1 << 4)
#define FS_RATE_24KHZ           (2 << 4)
#define FS_RATE_32KHZ           (3 << 4)
#define FS_RATE_48KHZ           (4 << 4)
#define FS_RATE_96KHZ           (5 << 4)
#define FS_RATE_192KHZ          (6 << 4)
#define FS_RATE_384KHZ          (7 << 4)
#define FS_RATE_768KHZ          (8 << 4)

#define RATIO_OF_16             (0)
#define RATIO_OF_24             (1)
#define RATIO_OF_32             (2)
#define RATIO_OF_48             (3)
#define RATIO_OF_64             (4)
#define RATIO_OF_96             (5)
#define RATIO_OF_128            (6)
#define RATIO_OF_192            (7)
#define RATIO_OF_256            (8)
#define RATIO_OF_384            (9)
#define RATIO_OF_512            (10)
#define RATIO_OF_1024           (11)
#define RATIO_OF_2048           (12)

#define ASI_OUT_CH1_EN          (0x1 << 7)
#define ASI_OUT_CH2_EN          (0x1 << 6)
#define ASI_OUT_CH3_EN          (0x1 << 5)
#define ASI_OUT_CH4_EN          (0x1 << 4)
#define ASI_OUT_CH5_EN          (0x1 << 3)
#define ASI_OUT_CH6_EN          (0x1 << 2)
#define ASI_OUT_CH7_EN          (0x1 << 1)
#define ASI_OUT_CH8_EN          (0x1 << 0)

#define ADCX140_IN_CH1_EN       (0x1 << 7)
#define ADCX140_IN_CH2_EN       (0x1 << 6)
#define ADCX140_IN_CH3_EN       (0x1 << 5)
#define ADCX140_IN_CH4_EN       (0x1 << 4)

#define ADCx140_PWR_CFG_ADC_PDZ (0x1 << 6)
#define ADCx140_PWR_CFG_PLL_PDZ (0x1 << 5)

#define LINE_INPUT              (0x1 << 7)
#define DMIC_PDM_INPUT          (0x1 << 6)
#define AMIC_PDM_INPUT          (0x1 << 5)

#define DMIC_PDMCLK_1536KHZ     (0x01)

#define ADCX140_BCLKINV_BIT     (0x1 << 2)
#define ADCX140_FSYNCINV_BIT    (0x1 << 3)
#define ADCX140_INV_MSK         (ADCX140_BCLKINV_BIT | ADCX140_FSYNCINV_BIT)
#define ADCX140_BCLK_FSYNC_MASTER   (0x1 << 7)
#define ADCX140_I2S_MODE_BIT    (0x1 << 6)
#define ADCX140_LEFT_JUST_BIT   (0x1 << 7)
#define ADCX140_ASI_FORMAT_MSK  (ADCX140_I2S_MODE_BIT | ADCX140_LEFT_JUST_BIT)

#define ASI_CH_MASK             (0x3F)

#define CH_SUM_MASK             (0xC)
#define FOUR_CH_SUM             (0x2)

#define ADCX140_WAKE_DEV        (0x1 << 0)
#define ADCX140_AREG_INTERNAL   (0x1 << 7)

#define ADCX140_MIC_BIAS_VAL_VREF   (0)
#define ADCX140_MIC_BIAS_VAL_VREF_1096  (1)
#define ADCX140_MIC_BIAS_VAL_AVDD   (6)
#define ADCX140_MIC_BIAS_VAL_MSK    (0x70)
#define ADCX140_MIC_BIAS_VAL_ADC_FS (0x1 << 1)
#define ADCX140_MIC_BIAS_VAL_SHIFT  (4)

#define ADCX140_RESET           (0x1 << 0)

typedef enum tlv320adcx140MicType
    {
    D_MIC = 0,
    A_MIC,
    } TLV320ADCX140_MIC_TYPE;

typedef struct reg_default
    {
    uint32_t reg;
    uint32_t def;
    } TLV320_REG_DEFAULT;

typedef struct tlv320adcx140Data
    {
    VXB_DEV_ID          pDev;
    uint16_t            i2cDevAddr;
    SEM_ID              i2cMutex;
    int32_t             resetGpio;
    } TLV320ADCX140_DATA;

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtTlv320adcx140h */

