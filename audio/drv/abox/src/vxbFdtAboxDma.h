/* vxbFdtAboxDma.h - Samsung Abox Dma driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

#ifndef __INFdtAboxDmah
#define __INFdtAboxDmah

#include <vxWorks.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <soc.h>
#include <aboxIpc.h>
#include <control.h>
#include <vxbAboxPcm.h>
#include <vxbFdtAboxCtr.h>


#if __cplusplus
extern "C" {
#endif

/* includes */

//abox_rdma_id
typedef enum aboxRdmaId
    {
    ABOX_RDMA_NO_CONNECT = -1,
    ABOX_RDMA0 = 0,
    ABOX_RDMA1,
    ABOX_RDMA2,
    ABOX_RDMA3,
    ABOX_RDMA4,
    ABOX_RDMA5,
    ABOX_RDMA6,
    ABOX_RDMA7,
    ABOX_RDMA8,
    ABOX_RDMA9,
    ABOX_RDMA10,
    ABOX_RDMA11,
    ABOX_RDMA12,
    ABOX_RDMA13,
    ABOX_RDMA14,
    ABOX_RDMA15,
    ABOX_RDMA16,
    ABOX_RDMA17,
    ABOX_RDMA18,
    ABOX_RDMA19,
    ABOX_RDMA20,
    ABOX_RDMA21
    } ABOX_RDMA_ID;

typedef enum aboxWdmaId {
    ABOX_WDMA_NO_CONNECT = -1,
    ABOX_WDMA0 = 0,
    ABOX_WDMA1,
    ABOX_WDMA2,
    ABOX_WDMA3,
    ABOX_WDMA4,
    ABOX_WDMA5,
    ABOX_WDMA6,
    ABOX_WDMA7,
    ABOX_WDMA8,
    ABOX_WDMA9,
    ABOX_WDMA10,
    ABOX_WDMA11,
    ABOX_WDMA12,
    ABOX_WDMA13,
    ABOX_WDMA14,
    ABOX_WDMA15,
    ABOX_WDMA16,
    ABOX_WDMA17,
    ABOX_WDMA18,
    ABOX_WDMA19,
    ABOX_WDMA20,
    ABOX_WDMA21
    } ABOX_WDMA_ID;


struct abox_dma_data {
    void                  * sfr_base;
    void                  * mailbox_base;
    unsigned int            id;
    unsigned int            type;
    unsigned int            stream_type;

    VXB_DEV_ID              pdev;
    VXB_DEV_ID              pdev_abox;
    ABOX_CTR_DATA *         abox_data;

    VX_SND_SOC_COMPONENT *  cmpnt;
    VX_SND_SOC_DAI_DRIVER * dai_drv;

    VX_SND_CONTROL *plat_kcontrol;
};

/* typedefs */

typedef struct abox_dma_ctrl {
    VXB_DEV_ID               pdev;
    void                   * handle;
    spinlockIsr_t            spinlockIsr;
    struct abox_dma_data   * pDmaData;
} ABOX_DMA_CTRL;


/* defines */

#ifndef BIT
#define BIT(n)                              (1u << (n))
#endif

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

#define ABOX_RDMA_DRIVER_NAME     "samsung,abox-rdma"
#define ABOX_WDMA_DRIVER_NAME     "samsung,abox-wdma"
#define MAX_ABOX_DMA_DAI_NAME_LEN (64)
#define MAX_PCM_STREAM_NAME_LEN   (64)
#define MAX_WIDGET_NAME_LEN       (32)

/* RDMA */

#define ABOX_RDMA_CTRL0             (0x0000)
#define ABOX_RDMA_CTRL1             (0x0004)
#define ABOX_RDMA_BUF_STR           (0x0008)
#define ABOX_RDMA_BUF_END           (0x000C)
#define ABOX_RDMA_BUF_OFFSET        (0x0010)
#define ABOX_RDMA_STR_POINT         (0x0014)
#define ABOX_RDMA_VOL_FACTOR        (0x0018)
#define ABOX_RDMA_VOL_CHANGE        (0x001C)
#define ABOX_RDMA_SBANK_LIMIT       (0x0020)
#define ABOX_RDMA_STATUS            (0x0030)
#define ABOX_RDMA_SPUS_CTRL0        (0x0040)
#define ABOX_RDMA_SPUS_CTRL1        (0x0044)
#define ABOX_RDMA_SPUS_SBANK        (0x0048)
#define ABOX_RDMA_SIFS_CNT_VAL      (0x004C)

/* ABOX_RDMA_CTRL0 */
#define ABOX_RDMA_BURST_LEN_L       (19)
#define ABOX_RDMA_BURST_LEN_H       (22)
#define ABOX_RDMA_BURST_LEN_MASK    (0x780000)

#define ABOX_RDMA_FUNC_L        (16)
#define ABOX_RDMA_FUNC_H        (18)
#define ABOX_RDMA_FUNC_MASK     (0x70000)

#define ABOX_RDMA_AUTO_FADE_IN_L    (15)
#define ABOX_RDMA_AUTO_FADE_IN_H    (15)
#define ABOX_RDMA_AUTO_FADE_IN_MASK (0x8000)

#define ABOX_RDMA_EXPEND_L      (13)
#define ABOX_RDMA_EXPEND_H      (13)
#define ABOX_RDMA_EXPEND_MASK   (0x2000)

#define ABOX_RDMA_FORMAT_L      (6)
#define ABOX_RDMA_FORMAT_H      (12)
#define ABOX_RDMA_FORMAT_MASK   (0x1fc0)

#define ABOX_RDMA_BUF_UPDATE_L      (5)
#define ABOX_RDMA_BUF_UPDATE_H      (5)
#define ABOX_RDMA_BUF_UPDATE_MASK   (0x20)

#define ABOX_RDMA_BUF_TYPE_L        (4)
#define ABOX_RDMA_BUF_TYPE_H        (4)
#define ABOX_RDMA_BUF_TYPE_MASK     (0x10)

#define ABOX_RDMA_ENABLE_L      (0)
#define ABOX_RDMA_ENABLE_H      (0)
#define ABOX_RDMA_ENABLE_MASK   (0x1)

/* ABOX_RDMA_CTRL1 */

#define ABOX_RDMA_RESTART_L     (0)
#define ABOX_RDMA_RESTART_H     (0)
#define ABOX_RDMA_RESTART_MASK  (0x1)

/* ABOX_RDMA_BUF_STR */

#define ABOX_RDMA_BUF_STR_L     (4)
#define ABOX_RDMA_BUF_STR_H     (31)
#define ABOX_RDMA_BUF_STR_MASK  (0xfffffff0)

/* ABOX_RDMA_BUF_END */

#define ABOX_RDMA_BUF_END_L     (4)
#define ABOX_RDMA_BUF_END_H     (31)
#define ABOX_RDMA_BUF_END_MASK  (0xfffffff0)

/* ABOX_RDMA_BUF_OFFSET */

#define ABOX_RDMA_BUF_OFFSET_L      (4)
#define ABOX_RDMA_BUF_OFFSET_H      (15)
#define ABOX_RDMA_BUF_OFFSET_MASK   (0xfff0)

/* ABOX_RDMA_STR_POINT */

#define ABOX_RDMA_STR_POINT_L       (4)
#define ABOX_RDMA_STR_POINT_H       (31)
#define ABOX_RDMA_STR_POINT_MASK    (0xfffffff0)

/* ABOX_RDMA_VOL_FACTOR */

#define ABOX_RDMA_VOL_FACTOR_L      (0)
#define ABOX_RDMA_VOL_FACTOR_H      (23)
#define ABOX_RDMA_VOL_FACTOR_MASK   (0xffffff)

/* ABOX_RDMA_VOL_CHANGE */

#define ABOX_RDMA_VOL_CHANGE_L      (0)
#define ABOX_RDMA_VOL_CHANGE_H      (23)
#define ABOX_RDMA_VOL_CHANGE_MASK   (0xffffff)

/* ABOX_RDMA_SBANK_LIMIT */

#define ABOX_RDMA_SBANK_LIMIT_L             (4)
#define ABOX_RDMA_V920_SBANK_LIMIT_H        (14)
#define ABOX_RDMA_SBANK_LIMIT_H             (ABOX_RDMA_V920_SBANK_LIMIT_H)
#define ABOX_RDMA_BANK_LIMIT_MASK           (0x7ff0)

/* ABOX_RDMA_STATUS */

#define ABOX_RDMA_PROGRESS_L        (31)
#define ABOX_RDMA_PROGRESS_H        (31)
#define ABOX_RDMA_PROGRESS_MASK     (0x80000000)

#define ABOX_RDMA_RBUF_OFFSET_L     (12)
#define ABOX_RDMA_RBUF_OFFSET_H     (27)
#define ABOX_RDMA_RBUF_OFFSET_MASK  (0xffff000)

#define ABOX_RDMA_RBUF_CNT_L        (0)
#define ABOX_RDMA_RBUF_CNT_H        (11)
#define ABOX_RDMA_RBUF_CNT_MASK     (0xfff)

/* ABOX_RDMA_SPUS_CTRL0 */

#define ABOX_RDMA_SPUS_ASRC_ID_L        (8)
#define ABOX_RDMA_V920_SPUS_ASRC_ID_H   (12)
#define ABOX_RDMA_SPUS_ASRC_ID_H        (ABOX_RDMA_V920_SPUS_ASRC_ID_H)
#define ABOX_RDMA_SPUS_ASRC_ID_MASK     (0x1f00)

#define ABOX_RDMA_SPUS_FUNC_CHAIN_L     (0)
#define ABOX_RDMA_SPUS_FUNC_CHAIN_H     (0)
#define ABOX_RDMA_SPUS_FUNC_CHAIN_MASK  (0x01)

/* ABOX_RDMA_SPUS_CTRL1 */

#define ABOX_RDMA_SIFS_FLUSH_L      (0)
#define ABOX_RDMA_SIFS_FLUSH_H      (0)
#define ABOX_RDMA_SIFS_FLUSH_MASK   (0x01)

/* ABOX_RDMA_SPUS_SBANK */

#define ABOX_RDMA_SBANK_SIZE_L          (20)
#define ABOX_RDMA_V920_SBANK_SIZE_H     (30)
#define ABOX_RDMA_SBANK_SIZE_H          (ABOX_RDMA_V920_SBANK_SIZE_H)
#define ABOX_RDMA_SBANK_SIZE_MASK       (0x7ff00000)

#define ABOX_RDMA_SBANK_STR_L       (4)
#define ABOX_RDMA_SBANK_STR_H       (14)
#define ABOX_RDMA_SBANK_STR_MASK    (0x7ff0)

/* ABOX_RDMA_SIFS_CNT_VAL */

#define ABOX_RDMA_SIFS_CNT_VAL_L        (0)
#define ABOX_RDMA_SIFS_CNT_VAL_H        (23)
#define ABOX_RDMA_SIFS_CNT_VAL_MASK     (0xffffff)


#define ABOX_RDMA_CTRL0_ENABLE      BIT(0)
#define ABOX_RDMA_SPUS_CTRL1_FLUSH  BIT(0)


#define ABOX_RDMA_SPUS_ASRC_ID          (8)
#define ABOX_RDMA_SPUS_FUNC_CHAIN_ASRC  (1)
#define ABOX_RDMA_RBUF_STATUS_MASK      (0xffff000)


/* WDMA */

#define ABOX_WDMA_CTRL              (0x0000)
#define ABOX_WDMA_BUF_STR           (0x0008)
#define ABOX_WDMA_BUF_END           (0x000C)
#define ABOX_WDMA_BUF_OFFSET        (0x0010)
#define ABOX_WDMA_STR_POINT         (0x0014)
#define ABOX_WDMA_VOL_FACTOR        (0x0018)
#define ABOX_WDMA_VOL_CHANGE        (0x001C)
#define ABOX_WDMA_SBANK_LIMIT       (0x0020)
#define ABOX_WDMA_STATUS            (0x0030)
#define ABOX_WDMA_SPUM_CTRL0        (0x0040)
#define ABOX_WDMA_SPUM_SIFM_SBANK   (0x0048)

/* ABOX_WDMA_SPUM_CTRL0 */

#define ABOX_SPUM_FUNC_CHAIN_L      (0)
#define ABOX_SPUM_FUNC_CHAIN_H      (0)
#define ABOX_SPUM_FUNC_CHAIN_MASK   (0x1)

#define ABOX_SPUM_ASRC_ID_L         (8)
#define ABOX_SPUM_ASRC_ID_H         (11)
#define ABOX_SPUM_ASRC_ID_MASK      (0xf00)

/* ABOX_WDMA_CTRL */

#define ABOX_WDMA_ENABLE_L          (0)
#define ABOX_WDMA_ENABLE_H          (0)
#define ABOX_WDMA_ENABLE_MASK       (0x1)

#define ABOX_WDMA_BUF_UPDATE_L      (5)
#define ABOX_WDMA_BUF_UPDATE_H      (5)
#define ABOX_WDMA_BUF_UPDATE_MASK   (0x20)

#define ABOX_WDMA_FORMAT_L          (6)
#define ABOX_WDMA_FORMAT_H          (12)
#define ABOX_WDMA_FORMAT_MASK       (0x1fc0)

#define ABOX_WDMA_REDUCE_L          (13)
#define ABOX_WDMA_REDUCE_H          (13)
#define ABOX_WDMA_REDUCE_MASK       (0x2000)

#define ABOX_WDMA_AUTO_FADE_IN_L    (15)
#define ABOX_WDMA_AUTO_FADE_IN_H    (15)
#define ABOX_WDMA_AUTO_FADE_IN_MASK (0x8000)

#define ABOX_WDMA_FUNC_L            (16)
#define ABOX_WDMA_FUNC_H            (18)
#define ABOX_WDMA_FUNC_MASK         (0x70000)

#define ABOX_WDMA_BURST_LEN_L       (19)
#define ABOX_WDMA_BURST_LEN_H       (22)
#define ABOX_WDMA_BURST_LEN_MASK    (0x780000)

/* ABOX_WDMA_BUF_STR */

#define ABOX_WDMA_BUF_STR_L         (4)
#define ABOX_WDMA_BUF_STR_H         (31)
#define ABOX_WDMA_BUF_STR_MASK      (0xfffffff0)

/* ABOX_WDMA_BUF_END */

#define ABOX_WDMA_BUF_END_L         (4)
#define ABOX_WDMA_BUF_END_H         (31)
#define ABOX_WDMA_BUF_END_MASK      (0xfffffff0)

/* ABOX_WDMA_BUF_OFFSET */

#define ABOX_WDMA_BUF_OFFSET_L      (4)
#define ABOX_WDMA_BUF_OFFSET_H      (15)
#define ABOX_WDMA_BUF_OFFSET_MASK   (0xfff0)

/* ABOX_WDMA_STR_POINT */

#define ABOX_WDMA_STR_POINT_L       (4)
#define ABOX_WDMA_STR_POINT_H       (31)
#define ABOX_WDMA_STR_POINT_MASK    (0xfffffff0)

/* ABOX_WDMA_VOL_FACTOR */

#define ABOX_WDMA_VOL_FACTOR_L      (0)
#define ABOX_WDMA_VOL_FACTOR_H      (23)
#define ABOX_WDMA_VOL_FACTOR_MASK   (0xffffff)

/* ABOX_WDMA_VOL_CHANGE */

#define ABOX_WDMA_VOL_CHANGE_L      (0)
#define ABOX_WDMA_VOL_CHANGE_H      (23)
#define ABOX_WDMA_VOL_CHANGE_MASK   (0xffffff)

/* ABOX_WDMA_SBANK_LIMIT */

#define ABOX_WDMA_SBANK_LIMIT_L         (4)
#define ABOX_WDMA_SBANK_LIMIT_H_V920    (14)
#define ABOX_WDMA_SBANK_LIMIT_H         (ABOX_WDMA_SBANK_LIMIT_H_V920)
#define ABOX_WDMA_SBANK_LIMIT_MASK      (0x7ff0)

/* ABOX_WDMA_STATUS */

#define ABOX_WDMA_PROGRESS_L            (31)
#define ABOX_WDMA_PROGRESS_H            (31)
#define ABOX_WDMA_PROGRESS_SHIFT        (31)
#define ABOX_WDMA_PROGRESS_MASK         (0x1 << ABOX_WDMA_PROGRESS_SHIFT)


#define ABOX_WDMA_WBUF_OFFSET_L         (12)
#define ABOX_WDMA_WBUF_OFFSET_H         (27)
#define ABOX_WDMA_WBUF_OFFSET_MASK      (0xffff000)

#define ABOX_WDMA_WBUF_CNT_L            (0)
#define ABOX_WDMA_WBUF_CNT_H            (11)
#define ABOX_WDMA_WBUF_CNT_MASK         (0xfff)

/* ABOX_WDMA_SPUM_CTRL0 */

#define ABOX_WDMA_FUNC_CHAIN_L          (0)
#define ABOX_WDMA_FUNC_CHAIN_H          (0)
#define ABOX_WDMA_FUNC_CHAIN_MASK       (0x01)

#define ABOX_WDMA_ASRC_L                (8)
#define ABOX_WDMA_ASRC_H_V920           (12)
#define ABOX_WDMA_ASRC_H                (ABOX_WDMA_ASRC_H_V920)
#define ABOX_WDMA_ASRC_MASK             (0x1f00)

#define ABOX_WDMA_ROUTE_NSRC_L          (16)
#define ABOX_WDMA_ROUTE_NSRC_H          (21)
#define ABOX_WDMA_ROUTE_NSRC_MASK       (0x3f0000)

/* ABOX_WDMA_SPUM_SIFM_SBANK  */

#define ABOX_WDMA_SBANK_STR_L           (4)
#define ABOX_WDMA_SBANK_STR_H_V920      (14)
#define ABOX_WDMA_SBANK_STR_H           (ABOX_WDMA_SBANK_STR_H_V920)
#define ABOX_WDMA_SBANK_STR_MASK        (0x7ff0)

#define ABOX_WDMA_SBANK_SIZE_L          (20)
#define ABOX_WDMA_SBANK_SIZE_H          (29)
#define ABOX_WDMA_SBANK_SIZE_MASK       (0x3ff00000)

#define ABOX_WDMA_WBUF_STATUS_MASK      (0xf)
#define ABOX_SPUM_FUNC_CHAIN_ASRC       (1)
#define ABOX_SPUM_ASRC_ID               (8)



/* UAIF Type */
#define UAIF_NON_SFT        0
#define UAIF_SFT            1

/* DMA Type */
#define DMA_TO_NON_SFT_UAIF     0
#define DMA_TO_DMA              1
#define DMA_TO_SFT_UAIF         2

enum widget_register_order {
	WDMA_ASRC,
	WDMA_Reduce_Route,
	WDMA_Reduce,
	SIFM,
	WDMA,
	V910_WIDGET_CNT,
	NSRC = V910_WIDGET_CNT,
	V920_WIDGET_CNT
};

IMPORT int __abox_rdma_flush_sifs(struct abox_dma_data *data);
IMPORT void aboxRdmaDisable(struct abox_dma_data *data);
IMPORT void aboxRdmaEnable(struct abox_dma_data *data);
IMPORT uint32_t rdmaSifsCntValSet(struct abox_dma_data * data, uint32_t val);
IMPORT uint32_t aboxRdmaBufferStatusGet(struct abox_dma_data *data);
IMPORT uint32_t aboxRdmaStatusGet(struct abox_dma_data *data);
IMPORT uint32_t aboxRdmaIsEnabled(struct abox_dma_data *data);
IMPORT int aboxRdmaIpcRequest(struct abox_dma_data *data, ABOX_IPC_MSG *msg,
                              int atomic, int sync);
/* wdma */
IMPORT void aboxWdmaDisable(struct abox_dma_data   * pDmaData);
IMPORT void aboxWdmaEnable(struct abox_dma_data   * pDmaData);
IMPORT uint32_t aboxWdmaBufferStatuGet(struct abox_dma_data * pDmaData);
IMPORT uint32_t wdmaIsEnable(struct abox_dma_data  * pDmaData);
IMPORT uint32_t wdmaReduceEnable(struct abox_dma_data  * pDmaData);
IMPORT int wdmaRequestIpc(struct abox_dma_data *data, ABOX_IPC_MSG *msg,
                          int atomic, int sync);
IMPORT uint32_t aboxWdmaGetStatus(struct abox_dma_data *  pDmaData);
#if __cplusplus
}
#endif /* __cplusplus */

#endif /* __INFdtAboxDmah */

