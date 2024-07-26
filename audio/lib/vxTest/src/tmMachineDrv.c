/* tmMachineDrv.c - Audio Test for VxWorks Sound Core */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#include <vxWorks.h>
#include <soc.h>
#include "tmMachineDrv.h"

#define TEST_MACHINE_DEBUG
#ifdef  TEST_MACHINE_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t testDbgMask = DBG_ALL;

#define TEST_DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((testDbgMask & (mask)) || ((mask) == DBG_ALL))              \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#define TEST_DBG_MSG(...)
#endif  /* TEST_DBG_MSG */

/* defines */

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

#define SND_SOC_DAI_INITU(_name, _stream_name, _dai_fmt,    \
                         _no_pcm, _ignore_suspend,          \
                         _ignore_pmdown_time,               \
                         _be_hw_params_fixup, _ops,         \
                         _dpcm_playback, _dpcm_capture,     \
                         _symmetric_rate,                   \
                         _symmetric_channels,               \
                         _symmetric_sample_bits)            \
    {.name = ((const char *)_name),                                        \
    .streamName = ((const char *)_stream_name),                           \
    .dai_fmt = (_dai_fmt),                                  \
    .noPcm = (_no_pcm),                                    \
    .ignore_suspend = (_ignore_suspend),                    \
    .ignore_pmdown_time = (_ignore_pmdown_time),            \
    .be_hw_params_fixup = (_be_hw_params_fixup),            \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),                      \
    .dpcmCapture = (_dpcm_capture),                        \
    .symmetric_rate = (_symmetric_rate),                    \
    .symmetric_channels = (_symmetric_channels),            \
    .symmetric_sample_bits = (_symmetric_sample_bits),}

#define SND_SOC_DAI_INITP(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger1, _trigger2, _ops, _dpcm_playback)    \
    {.name = ((const char *)_name),                                        \
    .streamName = ((const char *)_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger1, _trigger2},                                  \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),}

#define SND_SOC_DAI_INITC(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger1, _trigger2, _ops, _dpcm_capture)     \
    {.name = ((const char *)_name),                                        \
    .streamName = ((const char *)_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger1, _trigger2},                                  \
    .ops = (_ops),                                          \
    .dpcmCapture = (_dpcm_capture),}

#define SND_SOC_DAI_INITW(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger1, _trigger2, _no_pcm, _ops,           \
                        _dpcm_capture)                      \
    {.name = ((const char *)_name),                                        \
    .streamName = ((const char *)_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger1, _trigger2},                                  \
    .noPcm = (_no_pcm),                                     \
    .ops = (_ops),                                          \
    .dpcmCapture = (_dpcm_capture),}

#define SND_SOC_DAI_INITR(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger1, _trigger2, _no_pcm, _ops,\
                          _dpcm_playback)                   \
    {.name = ((const char *)_name),                                        \
    .streamName = ((const char *)_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger1, _trigger2},                                  \
    .noPcm = (_no_pcm),                                     \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),}

#define SND_SOC_DAI_INITL(_name, _stream_name,              \
                          _dai_fmt, _no_pcm,                \
                          _ignore_suspend,                  \
                          _ignore_pmdown_time,              \
                          _be_hw_params_fixup,              \
                          _ops, _dpcm_playback,             \
                          _dpcm_capture)                    \
    {\
    .name = _name,                           \
    .streamName = _stream_name,               \
    .dai_fmt = _dai_fmt,                                  \
    .noPcm = _no_pcm,                                     \
    .ignore_suspend = _ignore_suspend,                    \
    .ignore_pmdown_time = _ignore_pmdown_time,            \
    .be_hw_params_fixup = _be_hw_params_fixup,            \
    .ops = _ops,                                          \
    .dpcmPlayback = _dpcm_playback,                      \
    .dpcmCapture = _dpcm_capture}


struct vxbDev g_testSocCardVxb;
struct vxbDev g_testCpuVxb;
struct vxbDev g_testPlatVxb;
struct vxbDev g_testCodecVxb;


/* forward declarations */

/* locals */

//testSocSnd_snd_card
LOCAL VX_SND_SOC_CARD testSocSndCard =
    {
    .name = "testSocSnd-sndcard",
    .suspend_post = testSocSndCardSuspendPost,
    .resume_pre = testSocSndCardResumePre,
//  .controls = testSocSndCardControl,
//  .num_controls = ELEMENTS(testSocSndCardControl),
//  .dapm_widgets = testSocSndCardWidget,
//  .num_dapm_widgets = ELEMENTS(testSocSndCardWidget),
    };

LOCAL VX_SND_SOC_OPS dummyOps =
    {
    .hwParams = dummyHwParams,
    };

LOCAL VX_SND_SOC_OPS pcmPlayBackOps = {};
LOCAL VX_SND_SOC_OPS pcmCaptureOps = {};
LOCAL VX_SND_SOC_OPS rdmaOps = {};
LOCAL VX_SND_SOC_OPS wdmaOps = {};

LOCAL VX_SND_SOC_DAI_LINK_COMPONENT dummyDaiLinkCmpnt[] =
    {
        {
        .name = "snd-soc-dummy",
        .dai_name = "snd-soc-dummy-dai",
        },
    };

LOCAL VX_SND_SOC_DAI_LINK testSocSndCardDaiDefaultLink[] =
    {
        SND_SOC_DAI_INITP ("PCM0p", "PCM0 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1),
        SND_SOC_DAI_INITP ("PCM1p", "PCM1 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1),

        SND_SOC_DAI_INITC ("PCM0c", "PCM0 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1),
        SND_SOC_DAI_INITC ("PCM1c", "PCM1 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1),

        SND_SOC_DAI_INITR ("RDMA0", "RDMA0", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1),
        SND_SOC_DAI_INITR ("RDMA1", "RDMA1", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1),

        SND_SOC_DAI_INITW ("WDMA0", "WDMA0", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1),
        SND_SOC_DAI_INITW ("WDMA1", "WDMA1", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1),

        SND_SOC_DAI_INITU ("UAIF0", "UAIF0",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, test_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0),
        SND_SOC_DAI_INITU ("UAIF1", "UAIF1",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, test_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0),

        SND_SOC_DAI_INITL ("LOOPBACK0", "LOOPBACK0",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, test_hw_params_fixup_helper,
            &dummyOps, 1, 1),
        SND_SOC_DAI_INITL ("LOOPBACK1", "LOOPBACK1",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, test_hw_params_fixup_helper,
            &dummyOps, 1, 1),
        {
        .name = "PCM Dummy Backend",
        .streamName = "PCM Dummy Backend",
        .dai_fmt = VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
        .noPcm = 1,
        .ignore_suspend = 1,
        .ignore_pmdown_time = 1,
        .be_hw_params_fixup = test_hw_params_fixup_helper,
        .dpcmPlayback = 1,
        .dpcmCapture = 1,
        .symmetric_rate = 0,
        .symmetric_channels = 0,
        .symmetric_sample_bits = 0,
        },
    };

STATUS test_hw_params_fixup_helper
    (
    VX_SND_SOC_PCM_RUNTIME *    rtd,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    return OK;
    }
/******************************************************************************
*
* testAudioSocCard - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

STATUS testAudioSocCard ()
    {
    VXB_ABOX_MACHINE_DRV_CTRL *     pMachineData;
    int32_t                         nLink = 0;
    BOOL                            foundCodec;
    VX_SND_SOC_CARD *               pCard;
    VX_SND_SOC_DAI_LINK *           pLink;
    VX_SND_SOC_DAI_LINK *           pDaiLink;
    VX_SND_SOC_DAI_LINK_COMPONENT * pDaiLinkCmpnt;
    uint32_t                        n;
    int32_t i = 0;
    VXB_DEV_ID                      pDev;

    g_testSocCardVxb.pName = "testSocCard";
    g_testCpuVxb.pName = "testCpuVxb";
    g_testPlatVxb.pName = "testPlatVxb";
    g_testCodecVxb.pName = "testCodecVxb";

    TEST_DBG_MSG (DBG_INFO, "%s\n", __func__);

    pDev = &g_testSocCardVxb;

    pMachineData = (VXB_ABOX_MACHINE_DRV_CTRL *)
                        vxbMemAlloc (sizeof (VXB_ABOX_MACHINE_DRV_CTRL));
    if (pMachineData == NULL)
        {
        TEST_DBG_MSG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        return ERROR;
        }

    nLink = ELEMENTS (testSocSndCardDaiDefaultLink);
    pLink = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK) * nLink);
    if (pLink == NULL)
        {
        TEST_DBG_MSG (DBG_ERR, "ERROR! %s() Line:%d\n", __func__, __LINE__);
        goto errOut;
        }

    for (i = 0; i < nLink; i++)
        {
        pDaiLink = &testSocSndCardDaiDefaultLink[i];

        if (pDaiLink->cpus == NULL)
            {
            pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
            if (pDaiLinkCmpnt == NULL)
                {
                goto errOut;
                }

            pDaiLink->cpus= pDaiLinkCmpnt;
            pDaiLink->cpuNum = 1;

            pDaiLinkCmpnt->pDev = &g_testCpuVxb;
            pDaiLinkCmpnt->dai_name = TM_DAI_CPU_NAME;
            }

        if (pDaiLink->platforms == NULL)
            {
            pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
            if (pDaiLinkCmpnt == NULL)
                {
                goto errOut;
                }

            pDaiLink->platforms= pDaiLinkCmpnt;
            pDaiLink->platformNum = 1;

            pDaiLinkCmpnt->pDev = &g_testPlatVxb;
            pDaiLinkCmpnt->dai_name = TM_DAI_PLAT_NAME;
            }

        if (pDaiLink->codecs == NULL)
            {
            pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
            if (pDaiLinkCmpnt == NULL)
                {
                goto errOut;
                }

            pDaiLink->codecs= pDaiLinkCmpnt;
            pDaiLink->codecNum = 1;

            pDaiLinkCmpnt->pDev = &g_testCodecVxb;
            pDaiLinkCmpnt->dai_name = TM_DAI_CODEC_NAME;

            foundCodec = TRUE;
            }

        if (!foundCodec)
            {
            pDaiLink->codecs = dummyDaiLinkCmpnt;
            pDaiLink->codecNum = ELEMENTS (dummyDaiLinkCmpnt);
            }

        memcpy (&pLink[i], pDaiLink, sizeof (VX_SND_SOC_DAI_LINK));
        }

    pMachineData->pDev = pDev;
    vxbDevSoftcSet (pDev, pMachineData);

    pCard = &testSocSndCard;
    pCard->dev = pDev;
    pCard->dai_link = pLink,
    pCard->num_links = nLink;
    pCard->drvdata = pMachineData;

    if (vxSndCardRegister (pCard) != OK)
        {
        TEST_DBG_MSG (DBG_ERR, "snd_soc_register_card() failed\n");
        goto errOut;
        }

    TEST_DBG_MSG (DBG_INFO, "%s snd_soc_register_card Done\n", __func__);
    return OK;

errOut:
    for (n = 0; n < nLink; n++)
        {
        if (testSocSndCardDaiDefaultLink[n].cpus != NULL)
            {
            vxbMemFree (testSocSndCardDaiDefaultLink[n].cpus);
            testSocSndCardDaiDefaultLink[n].cpus = NULL;
            testSocSndCardDaiDefaultLink[n].cpuNum = 0;
            }

        if (testSocSndCardDaiDefaultLink[n].platforms != NULL)
            {
            vxbMemFree (testSocSndCardDaiDefaultLink[n].platforms);
            testSocSndCardDaiDefaultLink[n].platforms = NULL;
            testSocSndCardDaiDefaultLink[n].platformNum = 0;
            }

        if (testSocSndCardDaiDefaultLink[n].codecs != NULL)
            {
            if (testSocSndCardDaiDefaultLink[n].codecs != dummyDaiLinkCmpnt)
                {
                vxbMemFree (testSocSndCardDaiDefaultLink[n].codecs);
                }
            testSocSndCardDaiDefaultLink[n].codecs = NULL;
            testSocSndCardDaiDefaultLink[n].codecNum = 0;
            }
        }

    if (pLink != NULL)
        {
        vxbMemFree (pLink);
        }

    if (pMachineData != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pMachineData);
        }

    return ERROR;
    }


LOCAL int32_t dummyHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;

    VX_SND_SOC_DAI *            cpu_dai = SOC_RTD_TO_CPU (rtd, 0);
    uint32_t                    fmt;

    TEST_DBG_MSG (DBG_INFO, "%s: %s-%d %dch, %dHz, %d width %d bytes\n",
             __func__,
             rtd->daiLink->name, substream->stream,
             PARAMS_CHANNELS (params), PARAMS_RATE (params),
             PARAMS_WIDTH (params), PARAMS_BUFFER_BYTES (params));

    if (PARAMS_CHANNELS (params) > 2)
        {
        fmt = VX_SND_DAI_FORMAT_DSP_A;
        }
    else
        {
        fmt = VX_SND_DAI_FORMAT_I2S;
        }

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        if (vxSndSocDaiFmtSet (cpu_dai, fmt
                               | SND_SOC_DAIFMT_NB_NF
                               | SND_SOC_DAIFMT_CBS_CFS) != 0)
            {
            TEST_DBG_MSG (DBG_ERR, "%s failed to set playback cpu fmt\n", __func__);
            return -1;
            }
        }
    else
        {
        if (vxSndSocDaiFmtSet (cpu_dai, fmt
                               | SND_SOC_DAIFMT_NB_NF
                               | SND_SOC_DAIFMT_CBM_CFM) != 0)
            {
            TEST_DBG_MSG (DBG_ERR, "%s failed to set capture cpu fmt\n", __func__);
            return -1;
            }
        }

    return 0;
    }

LOCAL int32_t testSocSndCardSuspendPost
    (
    VX_SND_SOC_CARD *card
    )
    {
    return 0;
    }

LOCAL int32_t testSocSndCardResumePre
    (
    VX_SND_SOC_CARD *card
    )
    {
    return 0;
    }

