/* tmAudioNewLib.c - Audio Test for VxWorks Sound Core */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file provides testing case.

\cs
<module>
    <component>        INCLUDE_TM_NEW_AUDIO_LIB  </component>
    <minVxWorksVer>    7.0                       </minVxWorksVer>
    <maxVxWorksVer>    .*                        </maxVxWorksVer>
    <arch>             .*                         </arch>
    <cpu>              .*                          </cpu>
    <bsp>                                          </bsp>
</module>
\ce

*/


/* includes */

#include <vxTest.h>
#include <audioLibWav.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <dllLib.h>
#include <vxSoundCore.h>
#include <control.h>
#include <pcm.h>
#include <soc.h>
#include "tmAudioNew.h"
#include <dirent.h>


#ifdef _WRS_KERNEL
#include <unistd.h>
#endif /* _WRS_KERNEL */

#define TEST_CH1_VOL_CTRL        0x05
#define TEST_CH2_VOL_CTRL        0x06
#define TEST_CH3_VOL_CTRL        0x07
#define TEST_CH4_VOL_CTRL        0x08

#ifndef _WRS_CONFIG_BSP_exynosauto_v920
#define AUDIO_TEST_PCM_C_NAME                "/dev/snd/pcmC0D2c"
#define AUDIO_TEST_PCM_P_NAME                "/dev/snd/pcmC0D0p"
#define AUTIO_TEST_CONTROL_NAME              "/dev/snd/controlC0"
#else
#define AUDIO_TEST_PCM_C_NAME                "/dev/snd/pcmC1D2c"
#define AUDIO_TEST_PCM_P_NAME                "/dev/snd/pcmC1D0p"
#define AUTIO_TEST_CONTROL_NAME              "/dev/snd/controlC1"
#endif

extern struct vxbDev g_testCpuVxb;
extern struct vxbDev g_testPlatVxb;
extern struct vxbDev g_testCodecVxb;
extern STATUS testAudioSocCard ();

LOCAL VX_SND_CTRL_LIST g_testLibCtrlList;
LOCAL TEST_CTRL_INFO_ALL g_testLibCtrlInfo;
LOCAL BOOL g_cardRegisted = FALSE;

LOCAL int32_t testCmpntProbe
    (
    VX_SND_SOC_COMPONENT *  component
    );
LOCAL uint32_t testCmpntRead
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs
    );
LOCAL int32_t testCmpntWrite
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs,
    uint32_t                val
    );
LOCAL STATUS testCmpPrepare
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    );
LOCAL STATUS testCmpClose
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    );
LOCAL STATUS testCmpOpen
    (
    struct vxSndSocComponent *component,
    SND_PCM_SUBSTREAM *substream
    );
LOCAL int testConstruct
    (
    VX_SND_SOC_COMPONENT * component,
    VX_SND_SOC_PCM_RUNTIME * runtime
    );

LOCAL int32_t testHwParams
    (
    SND_PCM_SUBSTREAM *  substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai
    );
LOCAL int32_t testSetDaiFmt
    (
    VX_SND_SOC_DAI *        codecDai,
    uint32_t                      fmt
    );
LOCAL int32_t testMute
    (
    VX_SND_SOC_DAI *        codecDai,
    int32_t                       mute,
    int32_t                       stream
    );
LOCAL BOOL isDaiSupportStream
    (
    VX_SND_SOC_DAI * dai,
    STREAM_DIRECT    direct
    );

LOCAL TEST_CTRL_CONFIG_INFO g_testLibCtrlConf[] =
    {
    /* should not be used in real board */
    TEST_CTRL_CONFIG_ENUM("TEST CHAIN0_1", "NOT_USED"),
    TEST_CTRL_CONFIG_ENUM("TEST CHAIN2_2", "RESERVED2"),
    TEST_CTRL_CONFIG_ENUM("TEST CHAIN3_3", "RESERVED4"),

    TEST_CTRL_CONFIG_INT("test_1 CH1 Playback Volume", 143),
    TEST_CTRL_CONFIG_INT("test_2 CH2 Playback Volume", 143),
    TEST_CTRL_CONFIG_INT("test_3 CH3 Playback Volume", 143),
    TEST_CTRL_CONFIG_INT("test_1 CH4 Playback Volume", 143),
    };


/* test contrls */
LOCAL int ctrlTestGet
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }

LOCAL int ctrlTestSet
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }

LOCAL int ctrlTestGet1
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }

LOCAL int ctrlTestSet1
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }

LOCAL int ctrlTestGet2
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }

LOCAL int ctrlTestSet2
    (
    VX_SND_CONTROL *kcontrol,
    VX_SND_CTRL_DATA_VAL *ucontrol
    )
    {
    TEST_AUD_INFO ("\n");
    return OK;
    }


LOCAL const char * const texts[] =
    {
    "NOT_USED",
    "RESERVED1",
    "RESERVED2",
    "RESERVED3",
    "RESERVED4",
    "RESERVED5"
    };

LOCAL unsigned int dac_tlv[] = {SNDRV_CTL_TLVT_DB_SCALE, 8, -10350, 0|50};
LOCAL VX_SND_ENUM solution_enum = TEST_ENUM_DOUBLE(SND_SOC_NOPM, 0, 0, 6, texts);

LOCAL VX_SND_CONTROL g_testControls_1[] =
    {
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN0_1", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet,
        ctrlTestSet),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN1_1", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet1,
        ctrlTestSet1),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN2_1", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet2,
        ctrlTestSet2),
    TEST_SOC_SINGLE_TLV ("test_1 CH1 Playback Volume",
                    TEST_CH1_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_1 CH2 Playback Volume",
                    TEST_CH2_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_1 CH3 Playback Volume",
                    TEST_CH3_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_1 CH4 Playback Volume",
                    TEST_CH4_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    };

LOCAL VX_SND_CONTROL g_testControls_2[] =
    {
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN0_2", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet,
        ctrlTestSet),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN1_2", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet1,
        ctrlTestSet1),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN2_2", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet2,
        ctrlTestSet2),
    TEST_SOC_SINGLE_TLV ("test_2 CH1 Playback Volume",
                    TEST_CH1_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_2 CH2 Playback Volume",
                    TEST_CH2_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_2 CH3 Playback Volume",
                    TEST_CH3_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_2 CH4 Playback Volume",
                    TEST_CH4_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    };

LOCAL VX_SND_CONTROL g_testControls_3[] =
    {
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN0_3", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet,
        ctrlTestSet),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN1_3", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet1,
        ctrlTestSet1),
    TEST_SOC_ENUM_EXT_MULTI("TEST CHAIN2_3", (solution_enum),
        snd_soc_info_enum_double,
        ctrlTestGet2,
        ctrlTestSet2),
    TEST_SOC_SINGLE_TLV ("test_3 CH1 Playback Volume",
                    TEST_CH1_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_3 CH2 Playback Volume",
                    TEST_CH2_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_3 CH3 Playback Volume",
                    TEST_CH3_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    TEST_SOC_SINGLE_TLV ("test_3 CH4 Playback Volume",
                    TEST_CH4_VOL_CTRL, 0, 0xFF, 0, dac_tlv),
    };

LOCAL const VX_SND_SOC_DAI_OPS testDaiOps =
    {
    .hwParams      = testHwParams,
    .setFmt        = testSetDaiFmt,
    .muteStream    = testMute,
    };

LOCAL VX_SND_SOC_DAI_DRIVER testDaiDriverLib[] =
    {
        {
        .name = TM_DAI_CPU_NAME,
        .playback =
            {
            .streamName    = "Playback-cpu",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .capture =
            {
            .streamName    = "Capture-cpu",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .ops = &testDaiOps,
        },
        {
        .name = TM_DAI_PLAT_NAME,
        .playback =
            {
            .streamName    = "Playback-platform",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .capture =
            {
            .streamName    = "Capture-platform",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .ops = &testDaiOps,
        },
        {
        .name = TM_DAI_CODEC_NAME,
        .playback =
            {
            .streamName    = "testPlayback-codec",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .capture =
            {
            .streamName    = "testCapture-codec",
            .channelsMin   = 2,
            .channelsMax   = 8,
            .rates          = (SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000),
            .formats        = VX_SND_FMTBIT_S16_LE,
            .rateMin        = 8000,
            .rateMax        = 384000,
            },
        .ops = &testDaiOps,
        }
    };

LOCAL const VX_SND_SOC_CMPT_DRV g_testCmpDrv1 =
    {
    .name               = "testCpntDrv1",
    .probe              = testCmpntProbe,
    .controls           = g_testControls_1,
    .num_controls       = ARRAY_SIZE (g_testControls_1),
    .read               = testCmpntRead,
    .write              = testCmpntWrite,
    .prepare            = testCmpPrepare,
    .close              = testCmpClose,
    .open               = testCmpOpen,
    .pcmConstruct       = testConstruct,
    };

LOCAL const VX_SND_SOC_CMPT_DRV g_testCmpDrv2 =
    {
    .name               = "testCpntDrv2",
    .probe              = testCmpntProbe,
    .controls           = g_testControls_2,
    .num_controls       = ARRAY_SIZE (g_testControls_2),
    .read               = testCmpntRead,
    .write              = testCmpntWrite,
    .prepare            = testCmpPrepare,
    .close              = testCmpClose,
    .open               = testCmpOpen,
    .pcmConstruct       = testConstruct,
    };

LOCAL const VX_SND_SOC_CMPT_DRV g_testCmpDrv3 =
    {
    .name               = "testCpntDrv3",
    .probe              = testCmpntProbe,
    .controls           = g_testControls_3,
    .num_controls       = ARRAY_SIZE (g_testControls_3),
    .read               = testCmpntRead,
    .write              = testCmpntWrite,
    .prepare            = testCmpPrepare,
    .close              = testCmpClose,
    .open               = testCmpOpen,
    .pcmConstruct       = testConstruct,
    };

LOCAL int32_t testCmpntProbe
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    TEST_AUD_INFO("component prebe function is called %s\n", component->name);
    return OK;
    }

LOCAL uint32_t testCmpntRead
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs
    )
    {
    TEST_AUD_INFO("component read function is called %s\n", cmpnt->name);
    return OK;
    }


LOCAL int32_t testCmpntWrite
    (
    VX_SND_SOC_COMPONENT *  cmpnt,
    uint32_t                regOfs,
    uint32_t                val
    )
    {
    TEST_AUD_INFO("component write function is called %s\n", cmpnt->name);
    return OK;
    }

LOCAL STATUS testCmpPrepare
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    )
    {
    TEST_AUD_INFO("component prepare function is called %s\n", component->name);
    return OK;
    }

LOCAL STATUS testCmpClose
    (
    struct vxSndSocComponent * component,
    SND_PCM_SUBSTREAM *        substream
    )
    {
    TEST_AUD_INFO("component close function is called %s\n", component->name);
    return OK;
    }

LOCAL STATUS testCmpOpen
    (
    struct vxSndSocComponent *component,
    SND_PCM_SUBSTREAM *substream
    )
    {
    VX_SND_PCM_HARDWARE *hw = &substream->runtime->hw;
    TEST_AUD_INFO("component open function is called %s\n", component->name);
    // function same as hwParamsSet()

    hw->bufferBytesMax = 36864;

    hw->formats	= (VX_SND_FMTBIT_S16_LE | VX_SND_FMTBIT_S24_LE | VX_SND_FMTBIT_S32_LE);
    hw->channelsMin = 1;
    hw->channelsMax = 32;
    hw->periodBytesMin = 128,
    hw->periodBytesMax = hw->bufferBytesMax / 2;
    hw->periodsMin = hw->bufferBytesMax / hw->periodBytesMax;
    hw->periodsMax = hw->bufferBytesMax / hw->periodBytesMin;

    return OK;
    }

LOCAL int testConstruct
    (
    VX_SND_SOC_COMPONENT * component,
    VX_SND_SOC_PCM_RUNTIME * runtime
    )
    {
    VX_SND_PCM *                pcm = runtime->pcm;
    SND_PCM_STREAM *            stream;
    SND_PCM_SUBSTREAM *         substream;
    VX_SND_SUBSTREAM_DMA_BUF *  dmaBuf;
    BOOL                        hasPlayback = FALSE;
    BOOL                        hasCapture  = FALSE;
    uint32_t                    idx;
    VX_SND_SOC_DAI *            cpuDai;
    VX_SND_SOC_DAI *            codecDai;

    TEST_AUD_INFO("component construct function is called %s\n", component->name);

    /* check if has playback and capture */
    if (runtime->daiLink->dynamic == 1 || runtime->daiLink->noPcm)
        {
        if (runtime->daiLink->dpcmPlayback == 1)
            {
            FOR_EACH_RUNTIME_CPU_DAIS (runtime, idx, cpuDai)
                {
                if (isDaiSupportStream (cpuDai, SNDRV_PCM_STREAM_PLAYBACK))
                    {
                    hasPlayback = TRUE;
                    break;
                    }
                }
            }
        if (runtime->daiLink->dpcmCapture == 1)
            {
            FOR_EACH_RUNTIME_CPU_DAIS (runtime, idx, cpuDai)
                {
                if (isDaiSupportStream (cpuDai, SNDRV_PCM_STREAM_CAPTURE))
                    {
                    hasCapture = TRUE;
                    break;
                    }
                }
            }
        }
    else
        {
        FOR_EACH_RUNTIME_CODEC_DAIS (runtime, idx, codecDai)
            {
            if (runtime->cpuNum == 1)
                {
                cpuDai = SOC_RTD_TO_CPU (runtime, 0);
                }
            else if (runtime->cpuNum == runtime->codecNum)
                {
                cpuDai = SOC_RTD_TO_CPU (runtime, idx);
                }
            else
                {
                TEST_AUD_INFO ("cpu dai number (when > 1) should be equal to "
                             "codec dai number\n");
                return ERROR;
                }

            if (isDaiSupportStream (cpuDai, SNDRV_PCM_STREAM_PLAYBACK) &&
                isDaiSupportStream (codecDai, SNDRV_PCM_STREAM_PLAYBACK))
                    {
                    hasPlayback = TRUE;
                    }

            if (isDaiSupportStream (cpuDai, SNDRV_PCM_STREAM_CAPTURE) &&
                isDaiSupportStream (codecDai, SNDRV_PCM_STREAM_CAPTURE))
                    {
                    hasCapture = TRUE;
                    }
            }
        }

    if (hasCapture)
        {
        stream = &pcm->stream[SNDRV_PCM_STREAM_CAPTURE];
        substream = stream->substream;
        dmaBuf = &substream->dmaBuf;
        
        /* actrually, the dma buff is not allocated by OS, maybe prepared by DSP */
        if (dmaBuf->area != NULL)
            {
            TEST_AUD_INFO("component(%s) has already allocate dma buff (c)\n", component->name);
            return OK;
            }
        
        dmaBuf->bytes = AUD_TEST_AUD_DMA_BUFF_SIZE;
        dmaBuf->private_data = NULL;
        dmaBuf->area = (unsigned char *) malloc (AUD_TEST_AUD_DMA_BUFF_SIZE);
        if (dmaBuf->area == NULL)
            {
            TEST_AUD_INFO("to allocate dma buffer failed \n");
            return ERROR;
            }
        
        /* just set the phyAddr to the same addr as area for test */
        dmaBuf->phyAddr = (PHYS_ADDR)dmaBuf->area;
        }
    else
        {
        stream = &pcm->stream[SNDRV_PCM_STREAM_PLAYBACK];
        substream = stream->substream;
        dmaBuf = &substream->dmaBuf;
        
        /* actrually, the dma buff is not allocated by OS, maybe prepared by DSP */
        if (dmaBuf->area != NULL)
            {
            TEST_AUD_INFO("component(%s) has already allocate dma buff (p)\n", component->name);
            return OK;
            }
        
        dmaBuf->bytes = AUD_TEST_AUD_DMA_BUFF_SIZE;
        dmaBuf->private_data = NULL;
        dmaBuf->area = (unsigned char *) malloc (AUD_TEST_AUD_DMA_BUFF_SIZE);
        if (dmaBuf->area == NULL)
            {
            TEST_AUD_INFO("to allocate dma buffer failed \n");
            return ERROR;
            }
        /* just set the phyAddr to the same addr as area for test */
        dmaBuf->phyAddr = (PHYS_ADDR)dmaBuf->area;
        }
    
    return OK;
    }

LOCAL int32_t testHwParams
    (
    SND_PCM_SUBSTREAM *  substream,
    VX_SND_PCM_HW_PARAMS *  params,
    VX_SND_SOC_DAI *        dai
    )
    {
    TEST_AUD_INFO("DAI HW params function is called %s\n", dai->name);
    return OK;
    }

LOCAL int32_t testSetDaiFmt
    (
    VX_SND_SOC_DAI *        codecDai,
    uint32_t                      fmt
    )
    {
    TEST_AUD_INFO("DAI set FMT function is called %s\n", codecDai->name);
    return OK;
    }

LOCAL int32_t testMute
    (
    VX_SND_SOC_DAI *        codecDai,
    int32_t                       mute,
    int32_t                       stream
    )
    {
    TEST_AUD_INFO("DAI MUTE function is called %s\n", codecDai->name);
    return OK;
    }

LOCAL BOOL isDaiSupportStream
    (
    VX_SND_SOC_DAI * dai,
    STREAM_DIRECT    direct
    )
    {
    VX_SND_SOC_PCM_STREAM * socStream;

    socStream = direct == SNDRV_PCM_STREAM_PLAYBACK ? &dai->driver->playback :
                          &dai->driver->capture;
    return socStream->channelsMin > 0 ? TRUE : FALSE;
    }

STATUS tmAudio_libTest()
    {
    STATUS          retVal = ERROR;
    SND_CONTROL_DEV sndCtrlDev;
    TEST_AUDIO_INFO audioInfo;

    memset (&audioInfo, 0, sizeof(TEST_AUDIO_INFO));

    if (!g_cardRegisted)
        {
        retVal = vxSndCpntRegister(&g_testCpuVxb, &g_testCmpDrv1,
                                &testDaiDriverLib[0], 1);
        if (retVal != OK)
            {
            TEST_AUD_ERR("register component %s failed\n", g_testCmpDrv1.name);
            return ERROR;
            }

        retVal = vxSndCpntRegister(&g_testPlatVxb, &g_testCmpDrv2,
                                &testDaiDriverLib[1], 1);
        if (retVal != OK)
            {
            TEST_AUD_ERR("register component %s failed\n", g_testCmpDrv2.name);
            return ERROR;
            }

        retVal = vxSndCpntRegister(&g_testCodecVxb, &g_testCmpDrv3,
                                &testDaiDriverLib[2], 1);
        if (retVal != OK)
            {
            TEST_AUD_ERR("register component %s failed\n", g_testCmpDrv3.name);
            return ERROR;
            }

        /*
        register a test soc snd card.
        it will create core card, soc runtime and register several
        components with controls.
        */
        testAudioSocCard();

        g_cardRegisted = TRUE;
        }

    /*
    after testAudioSocCard, the pcm and control dev will be ready
    */
    memset (&g_testLibCtrlList, 0, sizeof(VX_SND_CTRL_LIST));
    memset (&g_testLibCtrlInfo.ctrlInfo[0], 0,
            sizeof (VX_SND_CTRL_INFO) * AUD_TEST_CTRL_INFO_NUM);
    g_testLibCtrlInfo.cnt = 0;

    g_testCtrl = &sndCtrlDev;
    strncpy(g_testCtrl->name, AUTIO_TEST_CONTROL_NAME, SND_DEV_NAME_LEN);

    retVal = getWavData (&audioInfo, FALSE, NULL);
    if (retVal != OK)
        {
        TEST_AUD_ERR("get audio info from wav file failed\n");
        return ERROR;
        }

    audioInfo.playback = TRUE;
    strncpy(audioInfo.playbackName, AUDIO_TEST_PCM_P_NAME, SND_DEV_NAME_LEN);

    audioInfo.numCtrlConf = sizeof (g_testLibCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testLibCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio dev failed\n");
        return ERROR;
        }
    TEST_AUD_INFO("close playback fd\n");
    close(audioInfo.devFd);
    TEST_AUD_INFO("close control device fd\n");
    close(audioInfo.ctrlFd);

    /*
    cant't work with data transfer, since more driver functions need to 
    implemented, such as: pointer, etc.
    */

    /* read from capture */
    audioInfo.playback = FALSE;
    strncpy(audioInfo.captureName, AUDIO_TEST_PCM_C_NAME, SND_DEV_NAME_LEN);
    audioInfo.numCtrlConf = sizeof (g_testLibCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testLibCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio dev failed\n");
        return ERROR;
        }
    TEST_AUD_INFO("close capture fd\n");
    close(audioInfo.devFdCapture);
    TEST_AUD_INFO("close control device fd\n");
    close(audioInfo.ctrlFd);

    cleanupAudioInfo(&audioInfo);
    return retVal;
    }


void audioLibTestInit()
    {
    g_testSndPcmDbgMask = (TEST_SND_AUD_DBG_INFO | TEST_SND_AUD_DBG_ERR);
    g_cardRegisted = FALSE;
    return;
    }


LOCAL VXTEST_ENTRY vxTestTbl_tmAudioNewLib[] =
{
    /*pTestName,             FUNCPTR,                pArg,   flags,   cpuSet,   timeout,   exeMode,   osMode,   level*/
    {"tmAudio_libTest",        (FUNCPTR)tmAudio_libTest, 0, 0, 0, 5000, VXTEST_EXEMODE_ALL, VXTEST_OSMODE_ALL, 0,"test functionality of audio device driver."},
    {NULL, (FUNCPTR)"tmAudioNew", 0, 0, 0, 1000000, 0, 0, 0}
};

/**************************************************************************
*
* tmAudioNewLibExec - Exec test module
*
* This routine should be called to initialize the test module.
*
* RETURNS: N/A
*
* \NOMANUAL
*/
#ifdef _WRS_KERNEL

STATUS tmAudioNewLibExec
    (
    char * testCaseName,
    VXTEST_RESULT * pTestResult
    )
    {
    return vxTestRun((VXTEST_ENTRY**)&vxTestTbl_tmAudioNewLib, testCaseName, pTestResult);
    }

#else

STATUS tmAudioNewLibExec
    (
    char * testCaseName,
    VXTEST_RESULT * pTestResult,
    int    argc,
    char * argv[]
    )
    {
    return vxTestRun((VXTEST_ENTRY**)&vxTestTbl_tmAudioNewLib, testCaseName, pTestResult, argc, argv);
    }

/**************************************************************************
* main - User application entry function
*
* This routine is the entry point for user application. A real time process
* is created with first task starting at this entry point.
*
* \NOMANUAL
*/
int main
    (
    int    argc,    /* number of arguments */
    char * argv[]   /* array of arguments */
    )
    {
    return tmAudioNewLibExec(NULL, NULL, argc, argv);
    }

#endif
