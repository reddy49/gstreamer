
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/* vxbFdtAboxUaif.h - Abox UAIF driver header file */

/*
DESCRIPTION
This file provides Abox Unified Audio Interface(UAIF) driver specific definitions.
*/

#ifndef __INCvxbFdtAboxUaifh
#define __INCvxbFdtAboxUaifh

#include <vxWorks.h>
#include <vxSoundCore.h>
#include <soc.h>
#include <aboxIpc.h>
#include <vxbFdtAboxCtr.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DTS value defines */

#define UAIF_LINE_ASYNC                 0
#define UAIF_LINE_SYNC                  1

/* DTS value defines end */

#define ABOX_UAIF_DRV_NAME              "aboxUaif"

/* register offset */

#define ABOX_UAIF_CTRL0                 (0x00)
#define ABOX_UAIF_CTRL1                 (0x04)
#define ABOX_UAIF_CTRL2                 (0x08)
#define ABOX_UAIF_STATUS                (0x0C)
#define ABOX_UAIF_ROUTE_CTRL            (0x40)

/* ABOX_UAIF_CTRL0 */

#define ABOX_UAIF_START_FIFO_DIFF_MIC_OFFSET       (26)
#define ABOX_UAIF_START_FIFO_DIFF_MIC_MASK (0xFC000000)

#define ABOX_UAIF_START_FIFO_DIFF_SPK_OFFSET       (20)
#define ABOX_UAIF_START_FIFO_DIFF_SPK_MASK (0x3F000000)

#define ABOX_UAIF_MIC_4RX_ENABLE_OFFSET            (19)
#define ABOX_UAIF_MIC_4RX_ENABLE_MASK         (0x80000)

#define ABOX_UAIF_SPK_4TX_ENABLE_OFFSET            (18)
#define ABOX_UAIF_SPK_4TX_ENABLE_MASK         (0x40000)

#define ABOX_UAIF_MIC_3RX_ENABLE_OFFSET            (17)
#define ABOX_UAIF_MIC_3RX_ENABLE_MASK         (0x20000)

#define ABOX_UAIF_SPK_3TX_ENABLE_OFFSET            (16)
#define ABOX_UAIF_SPK_3TX_ENABLE_MASK         (0x10000)

#define ABOX_UAIF_FORMAT_OFFSET                     (8)
#define ABOX_UAIF_FORMAT_MASK                  (0x7F00)

#define ABOX_UAIF_MIC_2RX_ENABLE_OFFSET             (6)
#define ABOX_UAIF_MIC_2RX_ENABLE_MASK            (0x40)

#define ABOX_UAIF_SPK_2TX_ENABLE_OFFSET             (5)
#define ABOX_UAIF_SPK_2TX_ENABLE_MASK            (0x20)

#define ABOX_UAIF_DATA_MODE_OFFSET                  (4)
#define ABOX_UAIF_DATA_MODE_MASK                 (0x10)

#define ABOX_UAIF_IRQ_MODE_OFFSET                   (3)
#define ABOX_UAIF_IRQ_MODE_MASK                   (0x8)

#define ABOX_UAIF_MODE_OFFSET                       (2)
#define ABOX_UAIF_MODE_MASK                       (0x4)

#define ABOX_UAIF_MIC_ENABLE_OFFSET                (1)
#define ABOX_UAIF_MIC_ENABLE_MASK                (0x2)

#define ABOX_UAIF_SPK_ENABLE_OFFSET                (0)
#define ABOX_UAIF_SPK_ENABLE_MASK                (0x1)

/* ABOX_UAIF_CTRL1 */

#define ABOX_UAIF_REAL_I2S_MODE_OFFSET             (31)
#define ABOX_UAIF_REAL_I2S_MODE_MASK       (0x80000000)

#define ABOX_UAIF_BCLK_POLARITY_S_OFFSET           (30)
#define ABOX_UAIF_BCLK_POLARITY_S_MASK     (0x40000000)

#define ABOX_UAIF_SD3_MODE_OFFSET                  (29)
#define ABOX_UAIF_SD3_MODE_MASK            (0x20000000)

#define ABOX_UAIF_SD2_MODE_OFFSET                  (28)
#define ABOX_UAIF_SD2_MODE_MASK            (0x10000000)

#define ABOX_UAIF_SD1_MODE_OFFSET                  (27)
#define ABOX_UAIF_SD1_MODE_MASK             (0x8000000)

#define ABOX_UAIF_SD0_MODE_OFFSET                  (26)
#define ABOX_UAIF_SD0_MODE_MASK             (0x4000000)

#define ABOX_UAIF_BCLK_POLARITY_OFFSET             (25)
#define ABOX_UAIF_BCLK_POLARITY_MASK        (0x2000000)

#define ABOX_UAIF_WS_MODE_OFFSET                   (24)
#define ABOX_UAIF_WS_MODE_MASK              (0x1000000)

#define ABOX_UAIF_WS_POLAR_OFFSET                  (23)
#define ABOX_UAIF_WS_POLAR_MASK              (0x800000)

#define ABOX_UAIF_SLOT_MAX_OFFSET                  (18)
#define ABOX_UAIF_SLOT_MAX_MASK              (0x7C0000)

#define ABOX_UAIF_SBIT_MAX_OFFSET                  (12)
#define ABOX_UAIF_SBIT_MAX_MASK               (0x3F000)

#define ABOX_UAIF_VALID_STR_OFFSET                 (6)
#define ABOX_UAIF_VALID_STR_MASK               (0xFC0)

#define ABOX_UAIF_VALID_END_OFFSET                 (0)
#define ABOX_UAIF_VALID_END_MASK                (0x3F)

#define ABOX_UAIF_SYNC_4TX_OFFSET               (29)
#define ABOX_UAIF_SYNC_4TX_MASK             (0x800000)



/* ABOX_UAIF_CTRL2 */

#define ABOX_UAIF_SYNC_4RX_OFFSET                 (28)
#define ABOX_UAIF_SYNC_4RX_MASK           (0x10000000)

#define ABOX_UAIF_SYNC_3TX_OFFSET                 (27)
#define ABOX_UAIF_SYNC_3TX_MASK            (0x8000000)

#define ABOX_UAIF_SYNC_3RX_OFFSET                 (26)
#define ABOX_UAIF_SYNC_3RX_MASK            (0x4000000)

#define ABOX_UAIF_V920_SYNC_2TX_OFFSET            (25)
#define ABOX_UAIF_V920_SYNC_2TX_MASK       (0x2000000)

#define ABOX_UAIF_V920_SYNC_2RX_OFFSET            (24)
#define ABOX_UAIF_V920_SYNC_2RX_MASK       (0x1000000)

#define ABOX_UAIF_CK_VALID_MIC_OFFSET             (20)
#define ABOX_UAIF_CK_VALID_MIC_MASK         (0x100000)

#define ABOX_UAIF_VALID_STR_MIC_OFFSET            (14)
#define ABOX_UAIF_VALID_STR_MIC_MASK         (0xFC000)

#define ABOX_UAIF_VALID_END_MIC_OFFSET             (8)
#define ABOX_UAIF_VALID_END_MIC_MASK          (0x3F00)

#define ABOX_UAIF_FILTER_CNT_OFFSET                (0)
#define ABOX_UAIF_FILTER_CNT_MASK               (0x7F)

/* ABOX_UAIF_STATUS */

#define ABOX_UAIF_ERROR_OF_MIC_4RX_OFFSET           (3)
#define ABOX_UAIF_ERROR_OF_MIC_4RX_MASK           (0x8)
#define ABOX_UAIF_ERROR_OF_SPK_4TX_OFFSET           (2)
#define ABOX_UAIF_ERROR_OF_SPK_4TX_MASK           (0x4)

#define ABOX_UAIF_ERROR_OF_MIC_3RX_OFFSET           (3)
#define ABOX_UAIF_ERROR_OF_MIC_3RX_MASK           (0x8)
#define ABOX_UAIF_ERROR_OF_SPK_3TX_OFFSET           (2)
#define ABOX_UAIF_ERROR_OF_SPK_3TX_MASK           (0x4)

#define ABOX_UAIF_ERROR_OF_MIC_2RX_OFFSET           (3)
#define ABOX_UAIF_ERROR_OF_MIC_2RX_MASK           (0x8)
#define ABOX_UAIF_ERROR_OF_SPK_2TX_OFFSET           (2)
#define ABOX_UAIF_ERROR_OF_SPK_2TX_MASK           (0x4)

#define ABOX_UAIF_ERROR_OF_MIC_OFFSET               (1)
#define ABOX_UAIF_ERROR_OF_MIC_MASK               (0x2)

#define ABOX_UAIF_ERROR_OF_SPK_OFFSET               (0)
#define ABOX_UAIF_ERROR_OF_SPK_MASK               (0x1)

/* ABOX_UAIF_ROUTE_CTRL */

#define ABOX_UAIF_ROUTE_SPK_4TX_H                  (28)
#define ABOX_UAIF_ROUTE_SPK_4TX_L                  (24)
#define ABOX_UAIF_ROUTE_SPK_4TX_MASK       (0x1F000000)

#define ABOX_UAIF_ROUTE_SPK_3TX_H                  (20)
#define ABOX_UAIF_ROUTE_SPK_3TX_L                  (16)
#define ABOX_UAIF_ROUTE_SPK_3TX_MASK         (0x1F0000)

#define ABOX_UAIF_ROUTE_SPK_2TX_H                  (12)
#define ABOX_UAIF_ROUTE_SPK_2TX_L                  (8)
#define ABOX_UAIF_ROUTE_SPK_2TX_MASK           (0x1F00)

#define ABOX_UAIF_ROUTE_SPK_1TX_H                   (4)
#define ABOX_UAIF_ROUTE_SPK_1TX_L                   (0)
#define ABOX_UAIF_ROUTE_SPK_1TX_MASK             (0x1F)

enum abox_uaif_id {
    ABOX_UAIF0 = 0,
    ABOX_UAIF1,
    ABOX_UAIF2,
    ABOX_UAIF3,
    ABOX_UAIF4,
    ABOX_UAIF5,
    ABOX_UAIF6,
    ABOX_UAIF7,
    ABOX_UAIF8,
    ABOX_UAIF9,
};

typedef enum uaifMux
    {
    MUX_UAIF_MASTER = 0,
    MUX_UAIF_SLAVE = 100000000,
    } ABOX_UAIF_MUX;

typedef enum uaifLine
    {
    UAIF_SD0 = 0,
    UAIF_SD1,
    UAIF_SD2,
    UAIF_SD3,
    UAIF_SD_MAX,
    } ABOX_UAIF_LINE;

typedef enum uaifLineMode
    {
    UAIF_LINE_MODE_MIC = 0,
    UAIF_LINE_MODE_SPK,
    UAIF_LINE_MODE_MAX
    }ABOX_UAIF_LINE_MODE;

typedef enum uaifGpioOpt
    {
    UAIF_GPIO_RESET = 0,
    UAIF_GPIO_IDLE,
    }ABOX_UAIF_GPIO_OPT;

typedef struct vxbAboxUaifDrvCtrl
    {
    VXB_DEV_ID              pDev;
    VIRT_ADDR               regBase;
    void *                  regHandle;
    spinlockIsr_t           regmapSpinlockIsr;
    uint32_t                id;
    uint32_t                type;
    uint32_t                syncMode;
    uint32_t                dataLine;
    uint32_t                nRdma;
    uint32_t                nWdma;
    uint32_t                resetPinctrlId;
    uint32_t                idlePinctrlId;
    VXB_CLK_ID              bclk;
    VXB_CLK_ID              bclkMux;
    VXB_CLK_ID              bclkGate;
    VX_SND_SOC_COMPONENT *  cmpnt;
    SND_PCM_SUBSTREAM *     substream;
    VX_SND_SOC_CMPT_DRV *   cmpntDrv;
    VX_SND_SOC_DAI_DRIVER * daiDrv; //dai_drv
//  struct snd_soc_dapm_widget * widget;
//  struct snd_soc_dapm_route * route;
    ABOX_CTR_DATA *         abox_data;
    } VXB_ABOX_UAIF_DRV_CTRL;

extern void vxbAboxUaifModeSet(VXB_ABOX_UAIF_DRV_CTRL * data, int32_t sd, int32_t stream, int32_t mode);

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtAboxUaifh */

