
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

#ifndef __INCvxbAboxPcmh
#define __INCvxbAboxPcmh

#include <vxWorks.h>
#include <soc.h>
#include <aboxIpc.h>
#include <vxbFdtAboxDma.h>
#include <vxbFdtAboxCtr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_PERIOD_TIME(bit, ch, rate, period_bytes) \
	(((long long)period_bytes * 1000000) / (long long)((bit * ch / 8) * rate))

#define INTER_VAL(para, x)          ((&para->intervals[x])->min)

/* RDMA Default SBANK Size */

#define DEFAULT_SBANK_SIZE		(512)
#define UAIF_INTERNAL_BUF_SIZE		(256)

#define ABOX_DMA_TIMEOUT_US		(40000)

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

enum HIFI_CORE_NUM {
	HIFI_CORE0,
	HIFI_CORE1,
	HIFI_CORE2,
	HIFI_CORE3,
};

enum abox_pcm_playback_ids {
    ABOX_PCM_PLAYBACK_NO_CONNECT = -1,
    ABOX_PCM0P_ID = 0,  //PCM0 Playback
    ABOX_PCM1P_ID,      //PCM1 Playback
    ABOX_PCM2P_ID,      //PCM2 Playback
    ABOX_PCM3P_ID,      //PCM3 Playback
    ABOX_PCM4P_ID,      //PCM4 Playback
    ABOX_PCM5P_ID,      //PCM5 Playback
    ABOX_PCM6P_ID,      //PCM6 Playback
    ABOX_PCM7P_ID,      //PCM7 Playback
    ABOX_PCM8P_ID,      //PCM8 Playback
    ABOX_PCM9P_ID,      //PCM9 Playback
    ABOX_PCM10P_ID,     //PCM10 Playback
    ABOX_PCM11P_ID,     //PCM11 Playback
    ABOX_PCM12P_ID,     //PCM12 Playback
    ABOX_PCM13P_ID,     //PCM13 Playback
    ABOX_PCM14P_ID,     //PCM14 Playback
    ABOX_PCM15P_ID,     //PCM15 Playback
    ABOX_PCM16P_ID,     //PCM16 Playback
    ABOX_PCM17P_ID,     //PCM17 Playback
    ABOX_PCM18P_ID,     //PCM18 Playback
    ABOX_PCM19P_ID,     //PCM19 Playback
    ABOX_PCM20P_ID,     //PCM20 Playback
    ABOX_PCM21P_ID,     //PCM21 Playback
    ABOX_PCM22P_ID,     //PCM22 Playback
    ABOX_PCM23P_ID,     //PCM23 Playback
    ABOX_PCM24P_ID,     //PCM24 Playback
    ABOX_PCM25P_ID,     //PCM25 Playback
    ABOX_PCM26P_ID,     //PCM26 Playback
    ABOX_PCM27P_ID,     //PCM27 Playback
    ABOX_PCM28P_ID,     //PCM28 Playback
    ABOX_PCM29P_ID,     //PCM29 Playback
    ABOX_PCM30P_ID,     //PCM30 Playback
    ABOX_PCM31P_ID,     //PCM31 Playback
};

enum abox_pcm_capture_ids {
    ABOX_PCM_CAPTURE_NO_CONNECT = -1,
    ABOX_PCM0C_ID = 0,  //PCM0 capture
    ABOX_PCM1C_ID,      //PCM1 capture
    ABOX_PCM2C_ID,      //PCM2 capture
    ABOX_PCM3C_ID,      //PCM3 capture
    ABOX_PCM4C_ID,      //PCM4 capture
    ABOX_PCM5C_ID,      //PCM5 capture
    ABOX_PCM6C_ID,      //PCM6 capture
    ABOX_PCM7C_ID,      //PCM7 capture
    ABOX_PCM8C_ID,      //PCM8 capture
    ABOX_PCM9C_ID,      //PCM9 capture
    ABOX_PCM10C_ID,     //PCM10 capture
    ABOX_PCM11C_ID,     //PCM11 capture
    ABOX_PCM12C_ID,     //PCM12 capture
    ABOX_PCM13C_ID,     //PCM13 capture
    ABOX_PCM14C_ID,     //PCM14 capture
    ABOX_PCM15C_ID,     //PCM15 capture
    ABOX_PCM16C_ID,     //PCM16 capture
    ABOX_PCM17C_ID,     //PCM17 capture
    ABOX_PCM18C_ID,     //PCM18 capture
    ABOX_PCM19C_ID,     //PCM19 capture
    ABOX_PCM20C_ID,     //PCM20 capture
    ABOX_PCM21C_ID,     //PCM21 capture
    ABOX_PCM22C_ID,     //PCM22 capture
    ABOX_PCM23C_ID,     //PCM23 capture
    ABOX_PCM24C_ID,     //PCM24 capture
    ABOX_PCM25C_ID,     //PCM25 capture
    ABOX_PCM26C_ID,     //PCM26 capture
    ABOX_PCM27C_ID,     //PCM27 capture
    ABOX_PCM28C_ID,     //PCM28 capture
    ABOX_PCM29C_ID,     //PCM29 capture
    ABOX_PCM30C_ID,     //PCM30 capture
    ABOX_PCM31C_ID,     //PCM31 capture
};

enum abox_pcm_irq_ids {
    ABOX_PCM0_IRQ_ID = 0,   //PCM0 Playback
    ABOX_PCM1_IRQ_ID,       //PCM1 Playback
    ABOX_PCM2_IRQ_ID,       //PCM2 Playback
    ABOX_PCM3_IRQ_ID,       //PCM3 Playback
    ABOX_PCM4_IRQ_ID,       //PCM4 Playback
    ABOX_PCM5_IRQ_ID,       //PCM5 Playback
    ABOX_PCM6_IRQ_ID,       //PCM6 Playback
    ABOX_PCM7_IRQ_ID,       //PCM7 Playback
    ABOX_PCM8_IRQ_ID,       //PCM8 Playback
    ABOX_PCM9_IRQ_ID,       //PCM9 Playback
    ABOX_PCM10_IRQ_ID,      //PCM10 Playback
    ABOX_PCM11_IRQ_ID,      //PCM11 Playback
    ABOX_PCM12_IRQ_ID,      //PCM12 Playback
    ABOX_PCM13_IRQ_ID,      //PCM13 Playback
    ABOX_PCM14_IRQ_ID,      //PCM14 Playback
    ABOX_PCM15_IRQ_ID,      //PCM15 Playback
    ABOX_PCM16_IRQ_ID,      //PCM16 Playback
    ABOX_PCM17_IRQ_ID,      //PCM17 Playback
    ABOX_PCM18_IRQ_ID,      //PCM18 Playback
    ABOX_PCM19_IRQ_ID,      //PCM19 Playback
    ABOX_PCM20_IRQ_ID,      //PCM20 Playback
    ABOX_PCM21_IRQ_ID,      //PCM21 Playback
    ABOX_PCM22_IRQ_ID,      //PCM22 Playback
    ABOX_PCM23_IRQ_ID,      //PCM23 Playback
    ABOX_PCM24_IRQ_ID,      //PCM24 Playback
    ABOX_PCM25_IRQ_ID,      //PCM25 Playback
    ABOX_PCM26_IRQ_ID,      //PCM26 Playback
    ABOX_PCM27_IRQ_ID,      //PCM27 Playback
    ABOX_PCM28_IRQ_ID,      //PCM28 Playback
    ABOX_PCM29_IRQ_ID,      //PCM29 Playback
    ABOX_PCM30_IRQ_ID,      //PCM30 Playback
    ABOX_PCM31_IRQ_ID,      //PCM31 Playback
    ABOX_PCM32_IRQ_ID,      //PCM0 Capture
    ABOX_PCM33_IRQ_ID,      //PCM1 Capture
    ABOX_PCM34_IRQ_ID,      //PCM2 Capture
    ABOX_PCM35_IRQ_ID,      //PCM3 Capture
    ABOX_PCM36_IRQ_ID,      //PCM4 Capture
    ABOX_PCM37_IRQ_ID,      //PCM5 Capture
    ABOX_PCM38_IRQ_ID,      //PCM6 Capture
    ABOX_PCM39_IRQ_ID,      //PCM7 Capture
    ABOX_PCM40_IRQ_ID,      //PCM8 Capture
    ABOX_PCM41_IRQ_ID,      //PCM9 Capture
    ABOX_PCM42_IRQ_ID,      //PCM10 Capture
    ABOX_PCM43_IRQ_ID,      //PCM11 Capture
    ABOX_PCM44_IRQ_ID,      //PCM12 Capture
    ABOX_PCM45_IRQ_ID,      //PCM13 Capture
    ABOX_PCM46_IRQ_ID,      //PCM14 Capture
    ABOX_PCM47_IRQ_ID,      //PCM15 Capture
    ABOX_PCM48_IRQ_ID,      //PCM16 Capture
    ABOX_PCM49_IRQ_ID,      //PCM17 Capture
    ABOX_PCM50_IRQ_ID,      //PCM18 Capture
    ABOX_PCM51_IRQ_ID,      //PCM19 Capture
    ABOX_PCM52_IRQ_ID,      //PCM20 Capture
    ABOX_PCM53_IRQ_ID,      //PCM21 Capture
    ABOX_PCM54_IRQ_ID,      //PCM22 Capture
    ABOX_PCM55_IRQ_ID,      //PCM23 Capture
    ABOX_PCM56_IRQ_ID,      //PCM24 Capture
    ABOX_PCM57_IRQ_ID,      //PCM25 Capture
    ABOX_PCM58_IRQ_ID,      //PCM26 Capture
    ABOX_PCM59_IRQ_ID,      //PCM27 Capture
    ABOX_PCM60_IRQ_ID,      //PCM28 Capture
    ABOX_PCM61_IRQ_ID,      //PCM29 Capture
    ABOX_PCM62_IRQ_ID,      //PCM30 Capture
    ABOX_PCM63_IRQ_ID,      //PCM31 Capture
    MAX_PCM_IRQ_ID = ABOX_PCM63_IRQ_ID,
    NUM_OF_PCM_IRQ
};

struct abox_platform_of_data {
    VX_SND_SOC_DAI_DRIVER *base_dai_drv;
    unsigned int num_of_dai_drv;
};

//abox_platform_data
typedef struct aboxPcmControlData {
    unsigned int id;
    unsigned int irqId;
    unsigned long pointer;
    unsigned int pcm_dev_num;
    unsigned int streamType;
    int dmaId;
    VXB_DEV_ID   pdev;
    BOOL pp_enabled;
    VIRT_ADDR pp_pointer_base;
    PHYS_ADDR pp_pointer_phys;
    size_t pp_pointer_size;

    VXB_DEV_ID  pdevAbox;
    ABOX_CTR_DATA * aboxData;
    VXB_DEV_ID  pdevDma;
    struct abox_dma_data *dmaData;

    SND_PCM_SUBSTREAM *substream;
    VX_SND_PCM_HARDWARE aboxDmaHardware;
    VX_SND_PCM_HW_PARAMS *params;
    VX_SND_SOC_COMPONENT * cmpnt;
    VX_SND_SOC_DAI_DRIVER * daiDrv;
} ABOX_PCM_CTRL_DATA;


_WRS_INLINE SND_FRAMES_S_T bytesToFrames
    (
    VX_SND_PCM_RUNTIME * runtime,
    ssize_t size
    )
    {
    return size * 8 / runtime->frameBits;
    }

_WRS_INLINE ssize_t framesToBytes
    (
    VX_SND_PCM_RUNTIME * runtime,
    SND_FRAMES_S_T size
    )
    {
    return size * runtime->frameBits / 8;
    }

_WRS_INLINE size_t sndPcmlibBufferBytes
    (
    SND_PCM_SUBSTREAM *substream
    )
    {
    VX_SND_PCM_RUNTIME *runtime = substream->runtime;
    return framesToBytes(runtime, runtime->bufferSize);
    }

_WRS_INLINE size_t sndPcmlibPeriodBytes
    (
    SND_PCM_SUBSTREAM *substream
    )
    {
    VX_SND_PCM_RUNTIME *runtime = substream->runtime;
    return runtime->periodBytes;
    }

_WRS_INLINE int sndIntervalSetinteger(VX_SND_INTERVAL * intervals)
    {
    if (intervals->integer)
        return 0;
    if (intervals->openMin && intervals->openMax && intervals->min == intervals->max)
        {
        errnoSet(EINVAL);
        return (-1);
        }
    intervals->integer = 1;
    return 1;
    }

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbAboxPcmh */


