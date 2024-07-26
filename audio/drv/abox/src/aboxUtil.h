
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

#ifndef __INCaboxUtilh
#define __INCaboxUtilh

#include <vxWorks.h>

#ifdef __cplusplus
extern "C" {
#endif

/* macors */

#define HIFI4_CORE_MEM_SIZE_144K	0x00024000 /* 144KB */
#define HIFI4_CORE_MEM_SIZE_34M_288K	0x02248000 /* 34MB_288KB */
#define HIFI4_CORE1_PCM_DMA_START(pcmIrqId) \
        (HIFI4_CORE_MEM_SIZE_34M_288K + (HIFI4_CORE_MEM_SIZE_144K * pcmIrqId))


extern STATUS dmaMemAlloc(VXB_DEV_ID  pDev, ABOX_CTR_DATA  * data,
                          SND_PCM_SUBSTREAM * substream);
extern void hwParamsSet(int offset, VX_SND_PCM_HARDWARE * hw);


#ifdef __cplusplus
    }
#endif

#endif /* __INCaboxUtilh */
