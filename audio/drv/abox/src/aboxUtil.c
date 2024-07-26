
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

#include <vxWorks.h>
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <soc.h>
#include <vxbFdtAboxCtr.h>
#include <vxbAboxPcm.h>
#include "aboxUtil.h"

STATUS dmaMemAlloc
    (
    VXB_DEV_ID          pDev,
    ABOX_CTR_DATA     * data,
    SND_PCM_SUBSTREAM * substream
    )
    {
    ABOX_PCM_CTRL_DATA * pcmData = vxbDevSoftcGet(pDev);
    VX_SND_SUBSTREAM_DMA_BUF *dmaBuf = &substream->dmaBuf;
    VIRT_ADDR ptr_dma_buffer = 0;
    PHYS_ADDR phyAddr = 0;

    ptr_dma_buffer = data->adspData[1].virtAddr + 
                     HIFI4_CORE1_PCM_DMA_START(pcmData->irqId);
    phyAddr = data->adspData[1].physAddr + HIFI4_CORE1_PCM_DMA_START(pcmData->irqId);

    dmaBuf->private_data = NULL;
    dmaBuf->area = (unsigned char *)(ptr_dma_buffer);;
    dmaBuf->phyAddr =  phyAddr;
    dmaBuf->bytes = pcmData->aboxDmaHardware.bufferBytesMax;
    if (!dmaBuf->area)
        return ERROR;

    return OK;
    }


void hwParamsSet
    (
    int                   offset,
    VX_SND_PCM_HARDWARE * hw
    )
    {
    uint32_t                     * prop = NULL;
    int                            len;

    prop = (uint32_t *)vxFdtPropGet(offset, "samsung,buffer_bytes_max", &len);
    if ((prop == NULL) || (len != sizeof(int)))
        {
        hw->bufferBytesMax = DEFAULT_BUFFER_BYTES_MAX;
        }
    else
        {
        hw->bufferBytesMax = vxFdt32ToCpu(*prop);
        }

    hw->formats	= ABOX_SAMPLE_FORMATS;
    hw->channelsMin = 1;
    hw->channelsMax = 32;
    hw->periodBytesMin = PERIOD_BYTES_MIN,
    hw->periodBytesMax = hw->bufferBytesMax / 2;
    hw->periodsMin = hw->bufferBytesMax / hw->periodBytesMax;
    hw->periodsMax = hw->bufferBytesMax / hw->periodBytesMin;
    }


