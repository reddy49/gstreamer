/* vxbFdtTas6424.h - Tas6424 codec driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Tas6424 codec driver specific definitions.
*/

#ifndef __INCvxbFdtTas6424h
#define __INCvxbFdtTas6424h

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAS6424_DRV_NAME            "tas6424"

#define TAS6424_FAULT_CHECK_TASK_NAME   "tas6424FaultCheckTask"
#define TAS6424_FAULT_CHECK_TASK_PRI    (99)
#define TAS6424_FAULT_CHECK_TASK_STACK  (32 * 1024)

#define TAS6424_RATES (SNDRV_PCM_RATE_44100 | \
                       SNDRV_PCM_RATE_48000 | \
                       SNDRV_PCM_RATE_96000)

#define TAS6424_FORMATS (VX_SND_FMTBIT_S16_LE | VX_SND_FMTBIT_S16_BE | \
                         VX_SND_FMTBIT_S24_LE | VX_SND_FMTBIT_S24_BE | \
                         VX_SND_FMTBIT_S32_LE | VX_SND_FMTBIT_S32_BE)

/* registers */

#define TAS6424_MODE_CTRL           (0x00)
#define TAS6424_RESET               (0x1 << 7)
#define TAS6424_CH1_LO_MODE         (0x1 << 3)
#define TAS6424_CH2_LO_MODE         (0x1 << 2)
#define TAS6424_CH3_LO_MODE         (0x1 << 1)
#define TAS6424_CH4_LO_MODE         (0x1 << 0)

#define TAS6424_SAP_CTRL            (0x03)
#define TAS6424_SAP_RATE_MASK       (0xC0)
#define TAS6424_SAP_RATE_44100      (0x00 << 6)
#define TAS6424_SAP_RATE_48000      (0x01 << 6)
#define TAS6424_SAP_RATE_96000      (0x02 << 6)
#define TAS6424_SAP_TDM_SLOT_SZ_16  (0x1 << 4)
#define TAS6424_SAP_TDM_SLOT_LAST   (0x1 << 5)
#define TAS6424_SAP_FMT_MASK        (0x7)
#define TAS6424_SAP_I2S             (0x04 << 0)
#define TAS6424_SAP_LEFTJ           (0x05 << 0)
#define TAS6424_SAP_DSP             (0x06 << 0)

#define TAS6424_CH_STATE_CTRL       (0x04)
#define TAS6424_CH1_STATE_PLAY      (0x00 << 6)
#define TAS6424_CH2_STATE_PLAY      (0x00 << 4)
#define TAS6424_CH3_STATE_PLAY      (0x00 << 2)
#define TAS6424_CH4_STATE_PLAY      (0x00 << 0)
#define TAS6424_ALL_STATE_PLAY      (TAS6424_CH1_STATE_PLAY | \
                                     TAS6424_CH2_STATE_PLAY | \
                                     TAS6424_CH3_STATE_PLAY | \
                                     TAS6424_CH4_STATE_PLAY)
#define TAS6424_CH1_STATE_HIZ       (0x01 << 6)
#define TAS6424_CH2_STATE_HIZ       (0x01 << 4)
#define TAS6424_CH3_STATE_HIZ       (0x01 << 2)
#define TAS6424_CH4_STATE_HIZ       (0x01 << 0)
#define TAS6424_ALL_STATE_HIZ       (TAS6424_CH1_STATE_HIZ | \
                                     TAS6424_CH2_STATE_HIZ | \
                                     TAS6424_CH3_STATE_HIZ | \
                                     TAS6424_CH4_STATE_HIZ)
#define TAS6424_CH1_STATE_MUTE      (0x02 << 6)
#define TAS6424_CH2_STATE_MUTE      (0x02 << 4)
#define TAS6424_CH3_STATE_MUTE      (0x02 << 2)
#define TAS6424_CH4_STATE_MUTE      (0x02 << 0)
#define TAS6424_ALL_STATE_MUTE      (TAS6424_CH1_STATE_MUTE | \
                                     TAS6424_CH2_STATE_MUTE | \
                                     TAS6424_CH3_STATE_MUTE | \
                                     TAS6424_CH4_STATE_MUTE)

#define TAS6424_CH1_VOL_CTRL        0x05
#define TAS6424_CH2_VOL_CTRL        0x06
#define TAS6424_CH3_VOL_CTRL        0x07
#define TAS6424_CH4_VOL_CTRL        0x08

#define TAS6424_DC_DIAG_CTRL1       (0x09)
#define TAS6424_LDGBYPASS_MASK      (0x1 << 0)
#define TAS6424_LDGBYPASS_DISABLE   (0x1 << 0)

#define TAS6424_DC_DIAG_CTRL2       (0x0A)
#define TAS6424_DC_DIAG_CTRL3       (0x0B)

#define TAS6424_CHANNEL_FAULT       (0x10)
#define TAS6424_FAULT_OC_CH1        (0x1 << 7)
#define TAS6424_FAULT_OC_CH2        (0x1 << 6)
#define TAS6424_FAULT_OC_CH3        (0x1 << 5)
#define TAS6424_FAULT_OC_CH4        (0x1 << 4)
#define TAS6424_FAULT_DC_CH1        (0x1 << 3)
#define TAS6424_FAULT_DC_CH2        (0x1 << 2)
#define TAS6424_FAULT_DC_CH3        (0x1 << 1)
#define TAS6424_FAULT_DC_CH4        (0x1 << 0)

#define TAS6424_GLOB_FAULT1         (0x11)
#define TAS6424_FAULT_PVDD_OV       (0x1 << 3)
#define TAS6424_FAULT_VBAT_OV       (0x1 << 2)
#define TAS6424_FAULT_PVDD_UV       (0x1 << 1)
#define TAS6424_FAULT_VBAT_UV       (0x1 << 0)

#define TAS6424_GLOB_FAULT2         (0x12)
#define TAS6424_FAULT_OTSD          (0x1 << 4)
#define TAS6424_FAULT_OTSD_CH1      (0x1 << 3)
#define TAS6424_FAULT_OTSD_CH2      (0x1 << 2)
#define TAS6424_FAULT_OTSD_CH3      (0x1 << 1)
#define TAS6424_FAULT_OTSD_CH4      (0x1 << 0)

#define TAS6424_WARN                (0x13)
#define TAS6424_WARN_VDD_UV         (0x1 << 6)
#define TAS6424_WARN_VDD_POR        (0x1 << 5)
#define TAS6424_WARN_VDD_OTW        (0x1 << 4)
#define TAS6424_WARN_VDD_OTW_CH1    (0x1 << 3)
#define TAS6424_WARN_VDD_OTW_CH2    (0x1 << 2)
#define TAS6424_WARN_VDD_OTW_CH3    (0x1 << 1)
#define TAS6424_WARN_VDD_OTW_CH4    (0x1 << 0)

#define TAS6424_PIN_CTRL            (0x14)

#define TAS6424_MISC_CTRL3          (0x21)
#define TAS6424_CLEAR_FAULT         (0x1 << 7)

typedef enum workState
    {
    TAS6424_W_INIT,
    TAS6424_W_RUNNING,
    TAS6424_W_DONE
    } TAS6424_WORK_STATE;

typedef enum pmState
    {
    SND_SOC_CODEC_PMU,
    SND_SOC_CODEC_PMD,
    } TAS6424_PM_STATE;

//tas6424_data
typedef struct tas6424Data
    {
    VXB_DEV_ID          pDev;   //dev
    UINT16              i2cDevAddr;
    SEM_ID              i2cMutex;
    TASK_ID             faultCheckWorkTask;
    SEM_ID              faultCheckSem;
    spinlockIsr_t       workSpinlockIsr;
    BOOL                isCancel;
    TAS6424_WORK_STATE  workState;
    uint32_t            lastCfault;    //last_cfault
    uint32_t            lastFault1;    //last_fault1
    uint32_t            lastFault2;    //last_fault2
    uint32_t            lastWarn;      //last_warn
    int32_t             standbyGpio;   //standby_gpio
    int32_t             muteGpio;  //mute_gpio
    uint32_t            id;
    } TAS6424_DATA;

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtTas6424h */

