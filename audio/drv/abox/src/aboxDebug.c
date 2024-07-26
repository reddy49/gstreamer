/* vxbFdtAboxDebug.c - Samsung Abox Debug driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

This is the VxBus driver for the Abox Debug.
*/

#include <vxWorks.h>
#include <iosLib.h>
#include <sysLib.h>
#include <soc.h>
#include <errno.h>
#include <hwif/vxbus/vxbLib.h>
#include <private/semLibP.h>
#include <vxSoundCore.h>
#include <vxbFdtAboxGic.h>
#include <vxbFdtAboxMailbox.h>
#include <vxbFdtAboxUaif.h>
#include <vxbFdtAboxDma.h>
#include <vxbAboxPcm.h>
#include <vxbFdtAboxCtr.h>

#define DBG_MSG   printf

extern VX_SND_SOC_CARD * vxSndCardGet(void);
extern DL_LIST * vxSndCmpGet (void);

void vxAboxBufCmp
    (
    const void * src,
    const void * dst,
    unsigned int bufSize,
    unsigned int frameLen
    )
    {
    unsigned int i, j;
    unsigned int frameNum = bufSize/frameLen;
    const unsigned char *p1 = src;
    const unsigned char *p2 = dst;
    for (i = 0; i < frameNum; ++i)
        {
        if (memcmp (p1, p2, frameLen) != 0)
            {
            DBG_MSG ("frame[%d]:\n", i);
            DBG_MSG("src: ");
            for (j = 0; j < frameLen; ++j)
                {
                DBG_MSG("0x%x ", *p1++);
                if (j + 1 == frameLen)
                    {
                    DBG_MSG("\n");
                    }
                }
            DBG_MSG("dst: ");
            for (j = 0; j < frameLen; ++j)
                {
                DBG_MSG("0x%x ", *p2++);
                if (j + 1 == frameLen)
                    {
                    DBG_MSG("\n");
                    }
                }
            return;
            }
        p1 += frameLen;
        p2 += frameLen;
        }
    return;
    }

void vxAboxUaifShow(VXB_DEV_ID dev)
    {
    VXB_ABOX_UAIF_DRV_CTRL * data = (VXB_ABOX_UAIF_DRV_CTRL *) vxbDevSoftcGet(dev);
	unsigned int ctrl0 = 0;
	unsigned int ctrl1 = 0;
	unsigned int ctrl2 = 0;
	unsigned int status = 0;
	unsigned int route = 0;
	const char *ctrl0_format_bit_str[] = {"None", "16bit", "24bit", "32bit"};

	ctrl0 = vxSndSocComponentRead (data->cmpnt, ABOX_UAIF_CTRL0);
	ctrl1 = vxSndSocComponentRead (data->cmpnt, ABOX_UAIF_CTRL1);
	ctrl2 = vxSndSocComponentRead (data->cmpnt, ABOX_UAIF_CTRL2);
	status = vxSndSocComponentRead (data->cmpnt, ABOX_UAIF_STATUS);
	route = vxSndSocComponentRead (data->cmpnt, ABOX_UAIF_ROUTE_CTRL);

	DBG_MSG ("ABOX_UAIF%d_CTRL0[0x%08X]=========================================\n",
		data->id, ctrl0);
	DBG_MSG ("[START_FIFO_DIFF_MIC] : 0x%lX\n",
		(ctrl0 & ABOX_UAIF_START_FIFO_DIFF_MIC_MASK) >> ABOX_UAIF_START_FIFO_DIFF_MIC_OFFSET);
	DBG_MSG ("[START_FIFO_DIFF_SPK] : 0x%lX\n",
		(ctrl0 & ABOX_UAIF_START_FIFO_DIFF_SPK_MASK) >> ABOX_UAIF_START_FIFO_DIFF_SPK_OFFSET);
	DBG_MSG ("[MIC_4RX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_MIC_4RX_ENABLE_MASK) >> ABOX_UAIF_MIC_4RX_ENABLE_OFFSET) ?
		"Enable":"Disable");
	DBG_MSG ("[SPK_4TX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_SPK_4TX_ENABLE_MASK) >> ABOX_UAIF_SPK_4TX_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("[MIC_3RX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_MIC_3RX_ENABLE_MASK) >> ABOX_UAIF_MIC_3RX_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("[SPK_3TX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_SPK_3TX_ENABLE_MASK) >> ABOX_UAIF_SPK_3TX_ENABLE_OFFSET) ?
		"Enable" : "Disable");
    DBG_MSG ("[FORMAT]              : %s %ld Channel\n",
        ctrl0_format_bit_str[((ctrl0 & ABOX_UAIF_FORMAT_MASK) >> ABOX_UAIF_FORMAT_OFFSET) >> 5],
        (((ctrl0 & ABOX_UAIF_FORMAT_MASK) >> ABOX_UAIF_FORMAT_OFFSET) & 0x1F) + 1);
	DBG_MSG ("[MIC_2RX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_MIC_2RX_ENABLE_MASK) >> ABOX_UAIF_MIC_2RX_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("[SPK_2TX_ENABLE]      : %s\n",
		((ctrl0 & ABOX_UAIF_SPK_2TX_ENABLE_MASK) >> ABOX_UAIF_SPK_2TX_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("[DATA_MODE]           : %s\n",
		((ctrl0 & ABOX_UAIF_DATA_MODE_MASK) >> ABOX_UAIF_DATA_MODE_OFFSET) ?
		"Data Mode Enable" : "Data Mode Disable");
	DBG_MSG ("[IRQ_MODE]            : %s\n",
		((ctrl0 & ABOX_UAIF_IRQ_MODE_MASK) >> ABOX_UAIF_IRQ_MODE_OFFSET) ?
		"IRQ Mode Enable" : "IRQ Mode Disable");
	DBG_MSG ("[MODE]                : %s\n",
		((ctrl0 & ABOX_UAIF_MODE_MASK) >> ABOX_UAIF_MODE_OFFSET) ?
		"Master mode" : "Slave mode");
	DBG_MSG ("[MIC_ENABLE]          : %s\n",
		((ctrl0 & ABOX_UAIF_MIC_ENABLE_MASK) >> ABOX_UAIF_MIC_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("[SPK_ENABLE]          : %s\n",
		((ctrl0 & ABOX_UAIF_SPK_ENABLE_MASK) >> ABOX_UAIF_SPK_ENABLE_OFFSET) ?
		"Enable" : "Disable");
	DBG_MSG ("ABOX_UAIF%d_CTRL1[0x%08X]=========================================\n",
		data->id, ctrl1);
	DBG_MSG ("[REAL_I2S_MODE]       : %s\n",
		((ctrl1 & ABOX_UAIF_REAL_I2S_MODE_MASK) >> ABOX_UAIF_REAL_I2S_MODE_OFFSET) ?
		"Different Edge Launch":"Same Edge Launch");
	DBG_MSG ("[BCLK_POLARITY_Slave] : %s\n",
		((ctrl1 & ABOX_UAIF_BCLK_POLARITY_S_MASK) >> ABOX_UAIF_BCLK_POLARITY_S_OFFSET) ?
		"Falling Edge":"Rising Edge");
	DBG_MSG ("[SD3_MODE]            : %s\n",
		((ctrl1 & ABOX_UAIF_SD3_MODE_MASK) >> ABOX_UAIF_SD3_MODE_OFFSET) ?
		"SPK(4Tx)" : "MIC(1Rx)");
	DBG_MSG ("[SD2_MODE]            : %s\n",
		((ctrl1 & ABOX_UAIF_SD2_MODE_MASK) >> ABOX_UAIF_SD2_MODE_OFFSET) ?
		"SPK(3Tx)" : "MIC(2Rx)");
	DBG_MSG ("[SD1_MODE]            : %s\n",
		((ctrl1 & ABOX_UAIF_SD1_MODE_MASK) >> ABOX_UAIF_SD1_MODE_OFFSET) ?
		"SPK(2Tx)" : "MIC(3Rx)");
	DBG_MSG ("[SD0_MODE]            : %s\n",
		((ctrl1 & ABOX_UAIF_SD0_MODE_MASK) >> ABOX_UAIF_SD0_MODE_OFFSET) ?
		"SPK(1Tx)" : "MIC(4Rx)");
	DBG_MSG ("[BCLK_POLARITY_Master]: %s\n",
		((ctrl1 & ABOX_UAIF_BCLK_POLARITY_MASK) >> ABOX_UAIF_BCLK_POLARITY_OFFSET) ?
		"Falling Edge":"Rising Edge");
	DBG_MSG ("[WS_MODE]             : %s\n",
		((ctrl1 & ABOX_UAIF_WS_MODE_MASK) >> ABOX_UAIF_WS_MODE_OFFSET) ? "TDM":"I2S");
	DBG_MSG ("[WS_POLAR]            : %s\n",
		((ctrl1 & ABOX_UAIF_WS_POLAR_MASK) >> ABOX_UAIF_WS_POLAR_OFFSET) ?
		"Low for Right":"Low for Left");
	DBG_MSG ("[SLOT_MAX]            : 0x%lX\n",
		(ctrl1 & ABOX_UAIF_SLOT_MAX_MASK) >> ABOX_UAIF_SLOT_MAX_OFFSET);
	DBG_MSG ("[SBIT_MAX]            : 0x%lX\n",
		(ctrl1 & ABOX_UAIF_SBIT_MAX_MASK) >> ABOX_UAIF_SBIT_MAX_OFFSET);
	DBG_MSG ("[VALID_STR]           : 0x%lX\n",
		(ctrl1 & ABOX_UAIF_VALID_STR_MASK) >> ABOX_UAIF_VALID_STR_OFFSET);
	DBG_MSG ("[VALID_END]           : 0x%lX\n",
		(ctrl1 & ABOX_UAIF_VALID_END_MASK) >> ABOX_UAIF_VALID_END_OFFSET);
	DBG_MSG ("ABOX_UAIF%d_CTRL2[0x%08X]=========================================\n",
		data->id, ctrl2);
	DBG_MSG ("[SYNC_4TX]            : %s\n",
		((ctrl2 & ABOX_UAIF_SYNC_4TX_MASK) >> ABOX_UAIF_SYNC_4TX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[SYNC_4RX]            : %s\n",
		((ctrl2 & ABOX_UAIF_SYNC_4RX_MASK) >> ABOX_UAIF_SYNC_4RX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[SYNC_3TX]            : %s\n",
		((ctrl2 & ABOX_UAIF_SYNC_3TX_MASK) >> ABOX_UAIF_SYNC_3TX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[SYNC_3RX]            : %s\n",
		((ctrl2 & ABOX_UAIF_SYNC_3RX_MASK) >> ABOX_UAIF_SYNC_3RX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[SYNC_2TX]            : %s\n",
		((ctrl2 & ABOX_UAIF_V920_SYNC_2TX_MASK) >> ABOX_UAIF_V920_SYNC_2TX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[SYNC_2RX]            : %s\n",
		((ctrl2 & ABOX_UAIF_V920_SYNC_2RX_MASK) >> ABOX_UAIF_V920_SYNC_2RX_OFFSET) ? "SYNC":"Non SYNC");
	DBG_MSG ("[CK_VALID_MIC]        : 0x%lX\n",
		(ctrl2 & ABOX_UAIF_CK_VALID_MIC_MASK) >> ABOX_UAIF_CK_VALID_MIC_OFFSET);
	DBG_MSG ("[VALID_STR_MIC]       : 0x%lX\n",
		(ctrl2 & ABOX_UAIF_VALID_STR_MIC_MASK) >> ABOX_UAIF_VALID_STR_MIC_OFFSET);
	DBG_MSG ("[VALID_END_MIC]       : 0x%lX\n",
		(ctrl2 & ABOX_UAIF_VALID_END_MIC_MASK) >> ABOX_UAIF_VALID_END_MIC_OFFSET);
	DBG_MSG ("[FILTER_CNT]          : 0x%lX\n",
		(ctrl2 & ABOX_UAIF_FILTER_CNT_MASK) >> ABOX_UAIF_FILTER_CNT_OFFSET);
	DBG_MSG ("ABOX_UAIF%d_STATUS[0x%08X]========================================\n",
		data->id, status);
	DBG_MSG ("[ERROR_OF_MIC_4RX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_MIC_4RX_MASK) >> ABOX_UAIF_ERROR_OF_MIC_4RX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_SPK_4TX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_SPK_4TX_MASK) >> ABOX_UAIF_ERROR_OF_SPK_4TX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_MIC_3RX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_MIC_3RX_MASK) >> ABOX_UAIF_ERROR_OF_MIC_3RX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_SPK_3TX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_SPK_3TX_MASK) >> ABOX_UAIF_ERROR_OF_SPK_3TX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_MIC_2RX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_MIC_2RX_MASK) >> ABOX_UAIF_ERROR_OF_MIC_2RX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_SPK_2TX]    : %s\n",
		((status & ABOX_UAIF_ERROR_OF_SPK_2TX_MASK) >> ABOX_UAIF_ERROR_OF_SPK_2TX_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_MIC]        : %s\n",
		((status & ABOX_UAIF_ERROR_OF_MIC_MASK) >> ABOX_UAIF_ERROR_OF_MIC_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("[ERROR_OF_SPK]        : %s\n",
		((status & ABOX_UAIF_ERROR_OF_SPK_MASK) >> ABOX_UAIF_ERROR_OF_SPK_OFFSET) ?
		"Overflow":"No Error");
	DBG_MSG ("ABOX_UAIF%d_ROUTE_CTRL[0x%08X]====================================\n",
		data->id, route);
	DBG_MSG ("[ROUTE_UAIF_SPK_4TX]  : RDMA%ld\n",
		((route & ABOX_UAIF_ROUTE_SPK_4TX_MASK) >> ABOX_UAIF_ROUTE_SPK_4TX_L) - 1);
	DBG_MSG ("[ROUTE_UAIF_SPK_3TX]  : RDMA%ld\n",
		((route & ABOX_UAIF_ROUTE_SPK_3TX_MASK) >> ABOX_UAIF_ROUTE_SPK_3TX_L) - 1);
	DBG_MSG ("[ROUTE_UAIF_SPK_2TX]  : RDMA%ld\n",
		((route & ABOX_UAIF_ROUTE_SPK_2TX_MASK) >> ABOX_UAIF_ROUTE_SPK_2TX_L) - 1);
	DBG_MSG ("[ROUTE_UAIF_SPK]      : RDMA%ld\n",
		((route & ABOX_UAIF_ROUTE_SPK_1TX_MASK) >> ABOX_UAIF_ROUTE_SPK_1TX_L) - 1);
    }

void vxAboxRdmaShow(VXB_DEV_ID dev)
    {
    ABOX_DMA_CTRL                * pDevCtrl = NULL;
    struct abox_dma_data         * data = NULL;

    pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet(dev);
	data = pDevCtrl->pDmaData;
	unsigned int ctrl0 = 0;
	unsigned int ctrl1 = 0;
	unsigned int buf_str = 0;
	unsigned int buf_end = 0;
	unsigned int buf_offset = 0;
	unsigned int str_point = 0;
	unsigned int vol_factor = 0;
	unsigned int vol_change = 0;
	unsigned int sbank_limit = 0;
	unsigned int status = 0;
	unsigned int spus_ctrl0 = 0;
	unsigned int spus_ctrl1 = 0;
	unsigned int spus_sbank = 0;
	unsigned int sifs_cnt_val = 0;
	const char *ctrl0_func_str[] = {"Normal", "Pending", "Mute"};
	const char *ctrl0_format_bit_str[] = {"None", "16bit", "24bit", "32bit"};

	ctrl0 = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_CTRL0);
	ctrl1 = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_CTRL1);
	buf_str = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_BUF_STR);
	buf_end = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_BUF_END);
	buf_offset = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_BUF_OFFSET);
	str_point = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_STR_POINT);
	vol_factor = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_VOL_FACTOR);
	vol_change = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_VOL_CHANGE);
	sbank_limit = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_SBANK_LIMIT);
	status = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_STATUS);
	spus_ctrl0 = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_SPUS_CTRL0);
	spus_ctrl1 = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_SPUS_CTRL1);
	spus_sbank = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_SPUS_SBANK);
	sifs_cnt_val = vxSndSocComponentRead (data->cmpnt, ABOX_RDMA_SIFS_CNT_VAL);

	DBG_MSG ("ABOX_RDMA%d_CTRL0[0x%08X]=========================================\n",
		data->id, ctrl0);
	DBG_MSG ("[BURST_LEN]           : 0x%lX\n",
		(ctrl0 & ABOX_RDMA_BURST_LEN_MASK) >> ABOX_RDMA_BURST_LEN_L);
	DBG_MSG ("[FUNC]                : %s\n",
		ctrl0_func_str[(ctrl0 & ABOX_RDMA_FUNC_MASK) >> ABOX_RDMA_FUNC_L]);
	DBG_MSG ("[AUTO_FADE_IN]        : %s\n",
		((ctrl0 & ABOX_RDMA_AUTO_FADE_IN_MASK) >> ABOX_RDMA_AUTO_FADE_IN_L) ?
		"Enable":"Disable");
	DBG_MSG ("[EXPEND]              : %s\n",
		((ctrl0 & ABOX_RDMA_EXPEND_MASK) >> ABOX_RDMA_EXPEND_L) ? "Enable":"Disable");
	DBG_MSG ("[FORMAT]              : %s %ld Channel\n",
		ctrl0_format_bit_str[((ctrl0 & ABOX_RDMA_FORMAT_MASK) >> ABOX_RDMA_FORMAT_L) >> 5],
		(((ctrl0 & ABOX_RDMA_FORMAT_MASK) >> ABOX_RDMA_FORMAT_L) & 0x1f) + 1);
	DBG_MSG ("[BUF_UPDATE]          : 0x%lX\n",
		(ctrl0 & ABOX_RDMA_BUF_UPDATE_MASK) >> ABOX_RDMA_BUF_UPDATE_L);
	DBG_MSG ("[BUF_TYPE]            : 0x%lX\n",
		(ctrl0 & ABOX_RDMA_BUF_TYPE_MASK) >> ABOX_RDMA_BUF_TYPE_L);
	DBG_MSG ("[ENABLE]              : %s\n",
		((ctrl0 & ABOX_RDMA_ENABLE_MASK) >> ABOX_RDMA_ENABLE_L) ? "Enable":"Disable");
	DBG_MSG ("ABOX_RDMA%d_CTRL1[0x%08X]=========================================\n",
		data->id, ctrl1);
	DBG_MSG ("[RESTART]             : 0x%lX\n",
		(ctrl1 & ABOX_RDMA_RESTART_MASK) >> ABOX_RDMA_RESTART_L);
	DBG_MSG ("ABOX_RDMA%d_BUF_STR[0x%08X]=======================================\n",
		data->id, buf_str);
	DBG_MSG ("[BUF_STR]             : 0x%lX\n",
		(buf_str & ABOX_RDMA_BUF_STR_MASK) >> ABOX_RDMA_BUF_STR_L);
	DBG_MSG ("ABOX_RDMA%d_BUF_END[0x%08X]=======================================\n",
		data->id, buf_end);
	DBG_MSG ("[BUF_END]             : 0x%lX\n",
		(buf_end & ABOX_RDMA_BUF_END_MASK) >> ABOX_RDMA_BUF_END_L);
	DBG_MSG ("ABOX_RDMA%d_BUF_OFFSET[0x%08X]====================================\n",
		data->id, buf_offset);
	DBG_MSG ("[BUF_OFFSET]          : 0x%lX\n",
		(buf_offset & ABOX_RDMA_BUF_OFFSET_MASK) >> ABOX_RDMA_BUF_OFFSET_L);
	DBG_MSG ("ABOX_RDMA%d_STR_POINT[0x%08X]=====================================\n",
		data->id, str_point);
	DBG_MSG ("[STR_POINT]           : 0x%lX\n",
		(str_point & ABOX_RDMA_STR_POINT_MASK) >> ABOX_RDMA_STR_POINT_L);
	DBG_MSG ("ABOX_RDMA%d_VOL_FACTOR[0x%08X]====================================\n",
		data->id, vol_factor);
	DBG_MSG ("[VOL_FACTOR]          : 0x%lX\n",
		(vol_factor & ABOX_RDMA_VOL_FACTOR_MASK) >> ABOX_RDMA_VOL_FACTOR_L);
	DBG_MSG ("ABOX_RDMA%d_VOL_CHANGE[0x%08X]====================================\n",
		data->id, vol_change);
	DBG_MSG ("[VOL_CHANGE]          : 0x%lX\n",
		(vol_change & ABOX_RDMA_VOL_CHANGE_MASK) >> ABOX_RDMA_VOL_CHANGE_L);
	DBG_MSG ("ABOX_RDMA%d_SBANK_LIMIT[0x%08X]===================================\n",
		data->id, sbank_limit);
	DBG_MSG ("[SBANK_LIMIT]         : 0x%lX\n",
		(sbank_limit & ABOX_RDMA_BANK_LIMIT_MASK) >> ABOX_RDMA_SBANK_LIMIT_L);
	DBG_MSG ("ABOX_RDMA%d_STATUS[0x%08X]========================================\n",
		data->id, status);
	DBG_MSG ("[PROGRESS]            : 0x%lX\n",
		(status & ABOX_RDMA_PROGRESS_MASK) >> ABOX_RDMA_PROGRESS_L);
	DBG_MSG ("[RBUF_OFFSET]         : 0x%lX\n",
		(status & ABOX_RDMA_RBUF_OFFSET_MASK) >> ABOX_RDMA_RBUF_OFFSET_L);
	DBG_MSG ("[RBUF_CNT]            : 0x%lX\n",
		(status & ABOX_RDMA_RBUF_CNT_MASK) >> ABOX_RDMA_RBUF_CNT_L);
	DBG_MSG ("ABOX_RDMA%d_SPUS_CTRL0[0x%08X]====================================\n",
		data->id, spus_ctrl0);
	DBG_MSG ("[SPUS_ASRC_ID]        : ASRC%ld\n",
		(spus_ctrl0 & ABOX_RDMA_SPUS_ASRC_ID_MASK) >> ABOX_RDMA_SPUS_ASRC_ID_L);
	DBG_MSG ("[SPUS_FUNC_CHAIN]     : %s\n",
		((spus_ctrl0 & ABOX_RDMA_SPUS_FUNC_CHAIN_MASK) >> ABOX_RDMA_SPUS_FUNC_CHAIN_L) ?
		"ASRC":"SIFS");
	DBG_MSG ("ABOX_RDMA%d_SPUS_CTRL1[0x%08X]====================================\n",
		data->id, spus_ctrl1);
	DBG_MSG ("[SIFS_FLUSH]          : 0x%lX\n",
		(spus_ctrl1 & ABOX_RDMA_SIFS_FLUSH_MASK) >> ABOX_RDMA_SIFS_FLUSH_L);
	DBG_MSG ("ABOX_RDMA%d_SPUS_SBANK[0x%08X]====================================\n",
		data->id, spus_sbank);
	DBG_MSG ("[SBANK_SIZE]          : 0x%lX\n",
		(spus_sbank & ABOX_RDMA_SBANK_SIZE_MASK) >> ABOX_RDMA_SBANK_SIZE_L);
	DBG_MSG ("[SBANK_STR]           : 0x%lX\n",
		(spus_sbank & ABOX_RDMA_SBANK_STR_MASK) >> ABOX_RDMA_SBANK_STR_L);
	DBG_MSG ("ABOX_RDMA%d_SIFS_CNT_VAL[0x%08X]==================================\n",
		data->id, sifs_cnt_val);
	DBG_MSG ("[SIFS_CNT_VAL]        : 0x%lX\n",
		(sifs_cnt_val & ABOX_RDMA_SIFS_CNT_VAL_MASK) >> ABOX_RDMA_SIFS_CNT_VAL_L);
    }

void vxAboxWdmaShow(VXB_DEV_ID dev)
    {
    ABOX_DMA_CTRL                * pDevCtrl = NULL;
    struct abox_dma_data         * data = NULL;

	pDevCtrl = (ABOX_DMA_CTRL *) vxbDevSoftcGet(dev);
	data = pDevCtrl->pDmaData;
	unsigned int ctrl;
	unsigned int buf_str;
	unsigned int buf_end;
	unsigned int buf_offset;
	unsigned int str_point;
	unsigned int vol_factor;
	unsigned int vol_change;
	unsigned int sbank_limit;
	unsigned int status;
	unsigned int spum_ctrl0;
	unsigned int spum_sifm_sbank;
	const char *ctrl0_func_str[] = {"Normal", "Pending", "Mute"};
	const char *ctrl0_format_bit_str[] = {"None", "16bit", "24bit", "32bit"};

	ctrl = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_CTRL);
	buf_str = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_BUF_STR);
	buf_end = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_BUF_END);
	buf_offset = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_BUF_OFFSET);
	str_point = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_STR_POINT);
	vol_factor = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_VOL_FACTOR);
	vol_change = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_VOL_CHANGE);
	sbank_limit = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_SBANK_LIMIT);
	status = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_STATUS);
	spum_ctrl0 = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_SPUM_CTRL0);
	spum_sifm_sbank = vxSndSocComponentRead (data->cmpnt, ABOX_WDMA_SPUM_SIFM_SBANK);

	DBG_MSG ("ABOX_WDMA%d_CTRL[0x%08X]==========================================\n",
		data->id, ctrl);
	DBG_MSG ("[BURST_LEN]          : 0x%lX\n",
		(ctrl & ABOX_WDMA_BURST_LEN_MASK) >> ABOX_WDMA_BURST_LEN_L);
	DBG_MSG ("[FUNC]               : %s\n",
		ctrl0_func_str[(ctrl & ABOX_RDMA_FUNC_MASK) >> ABOX_RDMA_FUNC_L]);
	DBG_MSG ("[AUTO_FADE_IN]       : 0x%s\n",
		((ctrl & ABOX_WDMA_AUTO_FADE_IN_MASK) >> ABOX_WDMA_AUTO_FADE_IN_L) ?
		"Enable":"Disable");
	DBG_MSG ("[REDUCE]             : 0x%s\n",
		((ctrl & ABOX_WDMA_REDUCE_MASK) >> ABOX_WDMA_REDUCE_L) ? "Enable":"Disable");
	DBG_MSG ("[FORMAT]             : %s %ld Channel\n",
		ctrl0_format_bit_str[((ctrl & ABOX_WDMA_FORMAT_MASK) >> ABOX_WDMA_FORMAT_L) >> 5],
		(((ctrl & ABOX_WDMA_FORMAT_MASK) >> ABOX_WDMA_FORMAT_L) & 0x1f) + 1);
	DBG_MSG ("[BUF UPDATE]         : 0x%lX\n",
		(ctrl & ABOX_WDMA_BUF_UPDATE_MASK) >> ABOX_WDMA_BUF_UPDATE_L);
	DBG_MSG ("[ENABLE]             : 0x%lX\n",
		(ctrl & ABOX_WDMA_ENABLE_MASK) >> ABOX_WDMA_ENABLE_L);
	DBG_MSG ("ABOX_WDMA%d_BUF_STR[0x%08X]=======================================\n",
		data->id, buf_str);
	DBG_MSG ("[BUF_STR]            : 0x%lX\n",
		(buf_str & ABOX_WDMA_BUF_STR_MASK) >> ABOX_WDMA_BUF_STR_L);
	DBG_MSG ("ABOX_WDMA%d_BUF_END[0x%08X]=======================================\n",
		data->id, buf_end);
	DBG_MSG ("[BUF_END]            : 0x%lX\n",
		(buf_end & ABOX_WDMA_BUF_END_MASK) >> ABOX_WDMA_BUF_END_L);
	DBG_MSG ("ABOX_WDMA%d_BUF_OFFSET[0x%08X]====================================\n",
		data->id, buf_offset);
	DBG_MSG ("[BUF_OFFSET]         : 0x%lX\n",
		(buf_offset & ABOX_WDMA_BUF_OFFSET_MASK) >> ABOX_WDMA_BUF_OFFSET_L);
	DBG_MSG ("ABOX_WDMA%d_STR_POINT[0x%08X]=====================================\n",
		data->id, str_point);
	DBG_MSG ("[STR_POINT]          : 0x%lX\n",
		(str_point & ABOX_WDMA_STR_POINT_MASK) >> ABOX_WDMA_STR_POINT_L);
	DBG_MSG ("ABOX_WDMA%d_VOL_FACTOR[0x%08X]====================================\n",
		data->id, vol_factor);
	DBG_MSG ("[VOL_FACTOR]         : 0x%lX\n",
		(vol_factor & ABOX_WDMA_VOL_FACTOR_MASK) >> ABOX_WDMA_VOL_FACTOR_L);
	DBG_MSG ("ABOX_WDMA%d_VOL_CHANGE[0x%08X]====================================\n",
		data->id, vol_change);
	DBG_MSG ("[VOL_CHANGE]         : 0x%lX\n",
		(vol_change & ABOX_WDMA_VOL_CHANGE_MASK) >> ABOX_WDMA_VOL_CHANGE_L);
	DBG_MSG ("ABOX_WDMA%d_SBANK_LIMIT[0x%08X]===================================\n",
		data->id, sbank_limit);
	DBG_MSG ("[SBANK_LIMIT]        : 0x%lX\n",
		(sbank_limit & ABOX_WDMA_SBANK_LIMIT_MASK) >> ABOX_WDMA_SBANK_LIMIT_L);
	DBG_MSG ("ABOX_WDMA%d_STATUS[0x%08X]========================================\n",
		data->id, status);
	DBG_MSG ("[PROGRESS]           : 0x%lX\n",
		(status & ABOX_WDMA_PROGRESS_MASK) >> ABOX_WDMA_PROGRESS_L);
	DBG_MSG ("[WBUG_OFFSET]        : 0x%lX\n",
		(status & ABOX_WDMA_WBUF_OFFSET_MASK) >> ABOX_WDMA_WBUF_OFFSET_L);
	DBG_MSG ("[WBUF_CNT]           : 0x%lX\n",
		(status & ABOX_WDMA_WBUF_CNT_MASK) >> ABOX_WDMA_WBUF_CNT_L);
	DBG_MSG ("ABOX_WDMA%d_SPUM_CTRL0[0x%08X]====================================\n",
		data->id, spum_ctrl0);
	if (IS_V920)
		DBG_MSG ("[ROUTE_NSRC]          : 0x%lX\n",
		(spum_ctrl0 & ABOX_WDMA_ROUTE_NSRC_MASK) >> ABOX_WDMA_ROUTE_NSRC_L);
	DBG_MSG ("[ASRC_ID]             : ASRC%ld\n",
		(spum_ctrl0 & ABOX_WDMA_ASRC_MASK) >> ABOX_WDMA_ASRC_L);
	DBG_MSG ("[FUNC_CHAIN]          : %s\n",
		((spum_ctrl0 & ABOX_WDMA_FUNC_CHAIN_MASK) >> ABOX_WDMA_FUNC_CHAIN_L) ? "ASRC":"SIFS");
	DBG_MSG ("ABOX_WDMA%d_SPUM_SIFM_SBANK[0x%08X]===============================\n",
		data->id, spum_sifm_sbank);
	DBG_MSG ("[SBANK_STR]           : 0x%lX\n",
		(spum_sifm_sbank & ABOX_WDMA_SBANK_STR_MASK) >> ABOX_WDMA_SBANK_STR_L);
	DBG_MSG ("[SBANK_SIZE]          : 0x%lX\n",
		(spum_sifm_sbank & ABOX_WDMA_SBANK_SIZE_MASK) >> ABOX_WDMA_SBANK_SIZE_L);
    }

void vxSndDpcmShow
    (
    char * feName
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    char                     defalutName[] = {"PCM"};
    VX_SND_SOC_CARD * card;

    card = vxSndCardGet();
    if (card == NULL)
        {
        DBG_MSG ("sound card has been registered!\n");
        return;
        }

    if (feName == NULL)
        {
        DBG_MSG ("Haven't specified frontEnd name prefix."
                           " Use default prefix \"PCM\"\n");
        feName = defalutName;
        }

    FOR_EACH_SOC_CARD_RUNTIME (card, rtd)
        {
        if (strstr (rtd->daiLink->name, feName) != NULL)
            {
            if (rtd->pBe[SNDRV_PCM_STREAM_PLAYBACK] != NULL)
                {
                DBG_MSG ("\tPLAYBACK PATH: [FE: %s]", rtd->daiLink->name);
                FOR_EACH_SOC_BE_RTD (rtd, SNDRV_PCM_STREAM_PLAYBACK, beRtd)
                    {
                    DBG_MSG (" -> [BE: %s]", beRtd->daiLink->name);
                    }
                DBG_MSG ("\n");
                }

            if (rtd->pBe[SNDRV_PCM_STREAM_CAPTURE] != NULL)
                {
                DBG_MSG ("\tCAPTURE PATH:  [FE: %s]", rtd->daiLink->name);
                FOR_EACH_SOC_BE_RTD (rtd, SNDRV_PCM_STREAM_CAPTURE, beRtd)
                    {
                    DBG_MSG (" -> [BE: %s]", beRtd->daiLink->name);
                    }
                DBG_MSG ("\n");
                }
            }
        }
    DBG_MSG ("END\n");
    }

void vxSndComponentShow (BOOL verbose)
    {
    VX_SND_SOC_COMPONENT *  pCmpt;
    const VX_SND_SOC_CMPT_DRV  *  pCmptDrv;
    VX_SND_SOC_DAI *        pDai;
    VX_SND_SOC_DAI_DRIVER * pDaiDrv;
    uint32_t                idx;
    DL_LIST * pList;

    DBG_MSG ("\ncomponent information:\n=================\n");

    pList = vxSndCmpGet();
    if (pList == NULL)
        {
        DBG_MSG ("sound component has been registered!\n");
        return;
        }

    for (pCmpt = (VX_SND_SOC_COMPONENT *) DLL_FIRST (pList);
         pCmpt != NULL;
         pCmpt = (VX_SND_SOC_COMPONENT *) DLL_NEXT ((DL_NODE *) pCmpt))
        {
        if (semTake (pCmpt->ioMutex, WAIT_FOREVER) != OK)  //just for test
            {
            DBG_MSG ("component: %s get ioMux error\n", pCmpt->name);
            }
        pCmptDrv = pCmpt->driver;
        DBG_MSG ("component: %s\n", pCmpt->name);
        DBG_MSG ("\tcomponent.id: %d\n", pCmpt->id);
        DBG_MSG ("\tcomponent.namePrefix: %s\n", pCmpt->namePrefix == NULL ? "No" : pCmpt->namePrefix);
        DBG_MSG ("\tcomponent.pDev: %s\n", vxbDevNameGet (pCmpt->pDev));
        DBG_MSG ("\tcomponent.card: %s\n", pCmpt->card == NULL ? "no card" : pCmpt->card->name);
        DBG_MSG ("\tcomponent.activeCnt: %d\n", pCmpt->activeCnt);

        if (verbose)
            {
            DBG_MSG ("\n\tcomponent driver information:\n\t--------------------\n");
            DBG_MSG ("\t\tdriver.num_controls: %d\n", pCmptDrv->num_controls);
            if (pCmptDrv->num_controls != 0)
                {
                const VX_SND_CONTROL * pCtrl;
                for (idx = 0; idx < pCmptDrv->num_controls; idx++)
                    {
                    pCtrl = &pCmptDrv->controls[idx];
                    DBG_MSG ("\t\tdriver.controls[%d]: %s\n", idx, pCtrl->id.name);
                    }
                DBG_MSG ("\n");
                }
            DBG_MSG ("\t\tdriver.probe: \t%s func\n", pCmptDrv->probe == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.remove: \t%s func\n", pCmptDrv->remove == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.read: \t%s func\n", pCmptDrv->read == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.write: \t%s func\n", pCmptDrv->write == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.pcmConstruct: \t%s func\n", pCmptDrv->pcmConstruct == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.pcm_destruct: \t%s func\n", pCmptDrv->pcm_destruct == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.set_sysclk: \t%s func\n", pCmptDrv->set_sysclk == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.set_pll: \t%s func\n", pCmptDrv->set_pll == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.stream_event: \t%s func\n", pCmptDrv->stream_event == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.set_bias_level: \t%s func\n", pCmptDrv->set_bias_level == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.open: \t%s func\n", pCmptDrv->open == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.close: \t%s func\n", pCmptDrv->close == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.ioctl: \t%s func\n", pCmptDrv->ioctl == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.hwParams: \t%s func\n", pCmptDrv->hwParams == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.hwFree: \t%s func\n", pCmptDrv->hwFree == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.prepare: \t%s func\n", pCmptDrv->prepare == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.trigger: \t%s func\n", pCmptDrv->trigger == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.syncStop: \t%s func\n", pCmptDrv->syncStop == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.pointer: \t%s func\n", pCmptDrv->pointer == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.copy: \t%s func\n", pCmptDrv->copy == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.mmap: \t%s func\n", pCmptDrv->mmap == NULL ? "No": "Has");
            DBG_MSG ("\t\tdriver.probe_order: \t%d\n", pCmptDrv->probe_order);
            DBG_MSG ("\t\tdriver.remove_order: \t%d\n", pCmptDrv->remove_order);
            DBG_MSG ("\t\tdriver.be_hw_params_fixup: \t%s func\n", pCmptDrv->be_hw_params_fixup == NULL ? "No": "Has");
            }

        DBG_MSG ("\n\tcomponent.numDai: %d\n\t----------------------\n", pCmpt->numDai);

    for (pDai = (VX_SND_SOC_DAI *) DLL_FIRST (&pCmpt->dai_list);
         pDai != NULL;
         pDai = (VX_SND_SOC_DAI *) DLL_NEXT ((DL_NODE *) pDai))
            {
            pDaiDrv = pDai->driver;
            DBG_MSG ("\n\t\tdai: %s\n", pDai->name);
            DBG_MSG ("\t\tdai.id: %d\n", pDai->id);
            DBG_MSG ("\t\tdai.pDev: %s\n", vxbDevNameGet (pDai->pDev));
            DBG_MSG ("\t\tdai.streamActiveCnt[0]: %d\n", pDai->streamActiveCnt[0]);
            DBG_MSG ("\t\tdai.streamActiveCnt[1]: %d\n", pDai->streamActiveCnt[1]);
            DBG_MSG ("\t\tdai.tdmTxMask: %d\n", pDai->tdmTxMask);
            DBG_MSG ("\t\tdai.tdmRxMask: %d\n", pDai->tdmRxMask);
            DBG_MSG ("\t\tdai.probed: %d\n", pDai->probed);

            if (verbose)
                {
                DBG_MSG ("\n\t\tdai.driver: %s\n", pDaiDrv->name);
                DBG_MSG ("\t\tdriver id: %d\n", pDaiDrv->id);
                DBG_MSG ("\t\tdriver.probe: %s func\n", pDaiDrv->probe == NULL ? "No": "Has");
                DBG_MSG ("\t\tdriver.remove: %s func\n", pDaiDrv->remove == NULL ? "No": "Has");
                DBG_MSG ("\t\tdriver.ops: %s func\n", pDaiDrv->ops == NULL ? "No": "Has");

                if (pDaiDrv->playback.streamName != NULL)
                    {
                    DBG_MSG ("\n\t\t\tdai.driver has playback: %s\n", pDaiDrv->playback.streamName);
                    DBG_MSG ("\t\t\tdai.driver.playback.format: 0x%016llx\n", pDaiDrv->playback.formats);
                    DBG_MSG ("\t\t\tdai.driver.playback.rates: 0x%08x\n", pDaiDrv->playback.rates);
                    DBG_MSG ("\t\t\tdai.driver.playback.rateMin: %d\n", pDaiDrv->playback.rateMin);
                    DBG_MSG ("\t\t\tdai.driver.playback.rateMax: %d\n", pDaiDrv->playback.rateMax);
                    DBG_MSG ("\t\t\tdai.driver.playback.channelsMin: %d\n", pDaiDrv->playback.channelsMin);
                    DBG_MSG ("\t\t\tdai.driver.playback.channelsMax: %d\n", pDaiDrv->playback.channelsMax);
                    }

                if (pDaiDrv->capture.streamName != NULL)
                    {
                    DBG_MSG ("\n\t\t\tdai.driver has capture: %s\n", pDaiDrv->capture.streamName);
                    DBG_MSG ("\t\t\tdai.driver.capture.format: 0x%016llx\n", pDaiDrv->capture.formats);
                    DBG_MSG ("\t\t\tdai.driver.capture.rates: 0x%08x\n", pDaiDrv->capture.rates);
                    DBG_MSG ("\t\t\tdai.driver.capture.rateMin: %d\n", pDaiDrv->capture.rateMin);
                    DBG_MSG ("\t\t\tdai.driver.capture.rateMax: %d\n", pDaiDrv->capture.rateMax);
                    DBG_MSG ("\t\t\tdai.driver.capture.channelsMin: %d\n", pDaiDrv->capture.channelsMin);
                    DBG_MSG ("\t\t\tdai.driver.capture.channelsMax: %d\n", pDaiDrv->capture.channelsMax);
                    }
                }

            DBG_MSG ("\n");
            }
        DBG_MSG ("\n");
        (void) semGive (pCmpt->ioMutex);
        }
    }

