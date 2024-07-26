
/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

#include <vxSoundCore.h>
#include <soc.h>

/* forward declarations */
LOCAL STATUS sndDummyProbe(VXB_DEV_ID pDev);
LOCAL int dummyDmaOpen    (VX_SND_SOC_COMPONENT * component,
                             SND_PCM_SUBSTREAM * substream);
LOCAL STATUS sndDummyAttach    (   VXB_DEV_ID pdev);

/* defines */

#define STUB_RATES	SNDRV_PCM_RATE_8000_384000
#define STUB_FORMATS	(VX_SND_FMTBIT_S8 | \
			VX_SND_FMTBIT_U8 | \
			VX_SND_FMTBIT_S16_LE | \
			VX_SND_FMTBIT_U16_LE | \
			VX_SND_FMTBIT_S24_LE | \
			VX_SND_FMTBIT_S24_3LE | \
			VX_SND_FMTBIT_U24_LE | \
			VX_SND_FMTBIT_S32_LE | \
			VX_SND_FMTBIT_U32_LE)
#define PAGE_SIZE 4096
/* locals */

LOCAL UINT64 dummyDaiFormats =
	VX_SND_SOC_POSSIBLE_DAIFMT_I2S	|
	VX_SND_SOC_POSSIBLE_DAIFMT_RIGHT_J	|
	VX_SND_SOC_POSSIBLE_DAIFMT_LEFT_J	|
	VX_SND_SOC_POSSIBLE_DAIFMT_DSP_A	|
	VX_SND_SOC_POSSIBLE_DAIFMT_DSP_B	|
	VX_SND_SOC_POSSIBLE_DAIFMT_AC97	|
	VX_SND_SOC_POSSIBLE_DAIFMT_PDM	    |
	VX_SND_SOC_POSSIBLE_DAIFMT_GATED	|
	VX_SND_SOC_POSSIBLE_DAIFMT_CONT	|
	VX_SND_SOC_POSSIBLE_DAIFMT_NB_NF	|
	VX_SND_SOC_POSSIBLE_DAIFMT_NB_IF	|
	VX_SND_SOC_POSSIBLE_DAIFMT_IB_NF	|
	VX_SND_SOC_POSSIBLE_DAIFMT_IB_IF;

LOCAL const VX_SND_SOC_DAI_OPS dummyDaiOps = {
	.auto_selectable_formats	= &dummyDaiFormats,
	.num_auto_selectable_formats	= 1,
};

LOCAL const VX_SND_PCM_HARDWARE dummyDmaHardware = {
	/* Random values to keep userspace happy when checking constraints */
//	.info			= SNDRV_PCM_INFO_INTERLEAVED |
//				  SNDRV_PCM_INFO_BLOCK_TRANSFER,
	.bufferBytesMax	= 128*1024,
	.periodBytesMin	= PAGE_SIZE,
	.periodBytesMax	= PAGE_SIZE*2,
	.periodsMin		= 2,
	.periodsMax		= 128,
};


LOCAL VX_SND_SOC_DAI_DRIVER dummyDai = {
	.name = SND_SOC_DUMMY_CODEC_DAI_DRV_NAME,
	.playback = {
		.streamName	= "Playback",
		.channelsMin	= 1,
		.channelsMax	= 384,
		.rates		= STUB_RATES,
		.formats	= STUB_FORMATS,
	},
	.capture = {
		.streamName	= "Capture",
		.channelsMin	= 1,
		.channelsMax	= 384,
		.rates = STUB_RATES,
		.formats = STUB_FORMATS,
	 },
	.ops = &dummyDaiOps,
};

LOCAL const VX_SND_SOC_CMPT_DRV dummyCodec = {
    .name = SND_SOC_DUMMY_CODEC_COMPT_NAME,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
//	.non_legacy_dai_naming	= 1,
};

LOCAL const VX_SND_SOC_CMPT_DRV dummyPlatform = {
	.open		= dummyDmaOpen,
};


LOCAL VXB_FDT_DEV_MATCH_ENTRY sndDummyMatch[] =
    {
        {
        "snd-soc-dummy",
        NULL
        },
        {}                              /* empty terminated list */
    };

LOCAL VXB_DRV_METHOD SndDummyMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe),sndDummyProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), sndDummyAttach},
    {0, NULL}
    };


/* globals */

VXB_DRV vxbFdtSndDummyDrv =
    {
    {NULL},
    "sndDummyDriver",                           /* Name */
    "Snd Dummy Driver",                         /* Description */
    VXB_BUSID_FDT,                              /* Class */
    0,                                          /* Flags */
    0,                                          /* Reference count */
    SndDummyMethodList,                      /* Method table */
    };


VXB_DRV_DEF(vxbFdtSndDummyDrv);

/* functions */

LOCAL STATUS sndDummyProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, sndDummyMatch, NULL);
    }

LOCAL STATUS sndDummyAttach
    (
    VXB_DEV_ID pdev
    )
    {
    STATUS  ret = ERROR;

    ret = vxSndCpntRegister(pdev, &dummyCodec, &dummyDai, 1);
    if (ret != OK)
        {
        return ERROR;
        }

    return ret;
    }

LOCAL int dummyDmaOpen
    (
    VX_SND_SOC_COMPONENT * component,
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd = (VX_SND_SOC_PCM_RUNTIME *)
                                   (substream->privateData);

    /* BE's dont need dummy params */

    if (!rtd->daiLink->noPcm)
        vxSndSocRuntimeHwparamsSet(substream, &dummyDmaHardware);

    return 0;
    }

BOOL snd_soc_component_is_dummy
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    return (component->driver == &dummyPlatform ||
            component->driver == &dummyCodec);
    }

