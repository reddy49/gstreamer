/* vxbFdtAboxMachine.c - Samsung Abox machine driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

This is the driver for the Abox machine module.
*/

#include <vxWorks.h>
#include <soc.h>
#include <vxbFdtAboxCtr.h>
#include <vxbFdtAboxMachine.h>
#include <subsys/pinmux/vxbPinMuxLib.h>

#undef ABOX_MACHINE_DEBUG
#ifdef  ABOX_MACHINE_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t aboxDbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((aboxDbgMask & (mask)) || ((mask) == DBG_ALL))              \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("[%s(%d)] ", __func__, __LINE__);     \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* ABOX_MACHINE_DEBUG */

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
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dai_fmt = (_dai_fmt),                                  \
    .noPcm = (_no_pcm),                                     \
    .ignore_suspend = (_ignore_suspend),                    \
    .ignore_pmdown_time = (_ignore_pmdown_time),            \
    .be_hw_params_fixup = (_be_hw_params_fixup),            \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),                       \
    .dpcmCapture = (_dpcm_capture),                         \
    .symmetric_rate = (_symmetric_rate),                    \
    .symmetric_channels = (_symmetric_channels),            \
    .symmetric_sample_bits = (_symmetric_sample_bits),

#define SND_SOC_DAI_INITP(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger_0, _trigger_1, _ops,      \
                         _dpcm_playback)                    \
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger_0, _trigger_1},                    \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),

#define SND_SOC_DAI_INITC(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger_0, _trigger_1, _ops,      \
                         _dpcm_capture)                     \
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger_0, _trigger_1},                    \
    .ops = (_ops),                                          \
    .dpcmCapture = (_dpcm_capture),

#define SND_SOC_DAI_INITW(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger_0, _trigger_1, _no_pcm,   \
                         _ops, _dpcm_capture)               \
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger_0, _trigger_1},                    \
    .noPcm = (_no_pcm),                                     \
    .ops = (_ops),                                          \
    .dpcmCapture = (_dpcm_capture),

#define SND_SOC_DAI_INITR(_name, _stream_name,              \
                         _dynamic, _ignore_suspend,         \
                         _trigger_0, _trigger_1, _no_pcm,   \
                         _ops, _dpcm_playback)              \
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dynamic = (_dynamic),                                  \
    .ignore_suspend = (_ignore_suspend),                    \
    .trigger = {_trigger_0, _trigger_1},                    \
    .noPcm = (_no_pcm),                                     \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),

#define SND_SOC_DAI_INITL(_name, _stream_name,              \
                          _dai_fmt, _no_pcm,                \
                          _ignore_suspend,                  \
                          _ignore_pmdown_time,              \
                          _be_hw_params_fixup,              \
                          _ops, _dpcm_playback,             \
                          _dpcm_capture)                    \
    .name = (_name),                                        \
    .streamName = (_stream_name),                           \
    .dai_fmt = (_dai_fmt),                                  \
    .noPcm = (_no_pcm),                                     \
    .ignore_suspend = (_ignore_suspend),                    \
    .ignore_pmdown_time = (_ignore_pmdown_time),            \
    .be_hw_params_fixup = (_be_hw_params_fixup),            \
    .ops = (_ops),                                          \
    .dpcmPlayback = (_dpcm_playback),                       \
    .dpcmCapture = (_dpcm_capture),

/* forward declarations */

LOCAL STATUS aboxMachineProbe (VXB_DEV_ID pDev);
LOCAL STATUS aboxMachineAttach (VXB_DEV_ID pDev);
LOCAL int32_t exynosauto9SndCardSuspendPost (VX_SND_SOC_CARD *card);
LOCAL int32_t exynosauto9SndCardResumePre (VX_SND_SOC_CARD *card);
LOCAL int32_t sadkMachTas6424HwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    );
LOCAL int32_t sadkMachTas6424HwFree
    (
    SND_PCM_SUBSTREAM *         substream
    );
LOCAL int32_t sadkMachTas6424Prepare
    (
    SND_PCM_SUBSTREAM *         substream
    );
LOCAL void sadkMachTas6424Shutdown
    (
    SND_PCM_SUBSTREAM *         substream
    );
LOCAL int32_t sadkMachTlv320adcHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    );
LOCAL int32_t dummyHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    );
LOCAL VX_SND_SOC_DAI_LINK * daiDefaultLinkGet
    (
    char *daiName
    );

LOCAL STATUS getSndDaiPhandle
    (
    int         fdtOfs,
    uint32_t *  phandle,
    int32_t *   pSndDaiOfs,
    uint32_t *  pSndDaiIdx
    );

LOCAL void cardGpioCfg
    (
    VX_SND_SOC_CARD *   card,
    uint32_t            pinctrlId
    );

/* locals */


//exynosauto9_snd_card
LOCAL VX_SND_SOC_CARD exynosauto9SndCard =
    {
    .name = "exynosauto9-sndcard",
    .suspend_post = exynosauto9SndCardSuspendPost,
    .resume_pre = exynosauto9SndCardResumePre,
//  .controls = exynosauto9SndCardControl,
//  .num_controls = ELEMENTS(exynosauto9SndCardControl),
//  .dapm_widgets = exynosauto9SndCardWidget,
//  .num_dapm_widgets = ELEMENTS(exynosauto9SndCardWidget),
    };

//uaif_tas6424_ops
LOCAL VX_SND_SOC_OPS tas6424Ops =
    {
    .hwParams = sadkMachTas6424HwParams,
    .hwFree = sadkMachTas6424HwFree,
    .prepare = sadkMachTas6424Prepare,
    .shutdown = sadkMachTas6424Shutdown,
    };

//uaif_tlv320adc_ops
LOCAL VX_SND_SOC_OPS tlv320adcOps =
    {
    .hwParams = sadkMachTlv320adcHwParams,
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
        .name = SND_SOC_DUMMY_CODEC_COMPT_NAME,
        .dai_name = SND_SOC_DUMMY_CODEC_DAI_DRV_NAME,
        },
    };

//exynosauto9_dai & exynosauto9_dai_base
LOCAL VX_SND_SOC_DAI_LINK exynosauto9SndCardDaiDefaultLink[] =
    {
        {
        SND_SOC_DAI_INITP ("PCM0p", "PCM0 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM1p", "PCM1 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM2p", "PCM2 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM3p", "PCM3 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM4p", "PCM4 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM5p", "PCM5 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM6p", "PCM6 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM7p", "PCM7 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM8p", "PCM8 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM9p", "PCM9 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM10p", "PCM10 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM11p", "PCM11 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM12p", "PCM12 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM13p", "PCM13 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM14p", "PCM14 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM15p", "PCM15 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM16p", "PCM16 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM17p", "PCM17 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM18p", "PCM18 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM19p", "PCM19 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM20p", "PCM20 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM21p", "PCM21 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM22p", "PCM22 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM23p", "PCM23 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM24p", "PCM24 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM25p", "PCM25 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM26p", "PCM26 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM27p", "PCM27 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM28p", "PCM28 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM29p", "PCM29 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM30p", "PCM30 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITP ("PCM31p", "PCM31 Playback", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmPlayBackOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM0c", "PCM0 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM1c", "PCM1 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM2c", "PCM2 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM3c", "PCM3 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM4c", "PCM4 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM5c", "PCM5 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM6c", "PCM6 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM7c", "PCM7 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM8c", "PCM8 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM9c", "PCM9 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM10c", "PCM10 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM11c", "PCM11 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM12c", "PCM12 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM13c", "PCM13 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM14c", "PCM14 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM15c", "PCM15 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM16c", "PCM16 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM17c", "PCM17 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM18c", "PCM18 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM19c", "PCM19 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM20c", "PCM20 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM21c", "PCM21 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM22c", "PCM22 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM23c", "PCM23 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM24c", "PCM24 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM25c", "PCM25 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM26c", "PCM26 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM27c", "PCM27 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM28c", "PCM28 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM29c", "PCM29 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM30c", "PCM30 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITC ("PCM31c", "PCM31 Capture", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            &pcmCaptureOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA0", "RDMA0", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA1", "RDMA1", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA2", "RDMA2", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA3", "RDMA3", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA4", "RDMA4", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA5", "RDMA5", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA6", "RDMA6", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA7", "RDMA7", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA8", "RDMA8", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA9", "RDMA9", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA10", "RDMA10", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA11", "RDMA11", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA12", "RDMA12", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA13", "RDMA13", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA14", "RDMA14", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA15", "RDMA15", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA16", "RDMA16", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA17", "RDMA17", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA18", "RDMA18", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA19", "RDMA19", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA20", "RDMA20", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITR ("RDMA21", "RDMA21", 1, 1,
            SND_SOC_DPCM_TRIGGER_PRE, SND_SOC_DPCM_TRIGGER_POST,
            1, &rdmaOps, 1)
        },

        {
        SND_SOC_DAI_INITW ("WDMA0", "WDMA0", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA1", "WDMA1", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA2", "WDMA2", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA3", "WDMA3", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA4", "WDMA4", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA5", "WDMA5", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA6", "WDMA6", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA7", "WDMA7", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA8", "WDMA8", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA9", "WDMA9", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA10", "WDMA10", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA11", "WDMA11", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA12", "WDMA12", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA13", "WDMA13", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA14", "WDMA14", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA15", "WDMA15", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA16", "WDMA16", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA17", "WDMA17", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA18", "WDMA18", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA19", "WDMA19", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA20", "WDMA20", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITW ("WDMA21", "WDMA21", 1, 1,
            SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_PRE,
            1, &wdmaOps, 1)
        },
        {
        SND_SOC_DAI_INITU ("UAIF0", "UAIF0",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF1", "UAIF1",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF2", "UAIF2",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &tas6424Ops, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF3", "UAIF3",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &tas6424Ops, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF4", "UAIF4",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF5", "UAIF5",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &tlv320adcOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF6", "UAIF6",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF7", "UAIF7",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF8", "UAIF8",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &tas6424Ops, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITU ("UAIF9", "UAIF9",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1,  abox_hw_params_fixup_helper,
            &tas6424Ops, 1, 1, 0, 0, 0)
        },
        {
        SND_SOC_DAI_INITL ("LOOPBACK0", "LOOPBACK0",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1)
        },
        {
        SND_SOC_DAI_INITL ("LOOPBACK1", "LOOPBACK1",
            VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
            1, 1, 1, abox_hw_params_fixup_helper,
            &dummyOps, 1, 1)
        },
        {
        .name = "PCM Dummy Backend",
        .streamName = "PCM Dummy Backend",
        .dai_fmt = VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
        .noPcm = 1,
        .ignore_suspend = 1,
        .ignore_pmdown_time = 1,
        .be_hw_params_fixup = abox_hw_params_fixup_helper,
        .dpcmPlayback = 1,
        .dpcmCapture = 1,
        .symmetric_rate = 0,
        .symmetric_channels = 0,
        .symmetric_sample_bits = 0,
        },
    };

LOCAL VXB_DRV_METHOD aboxMachineMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxMachineProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxMachineAttach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxMachineMatch[] =
    {
        {
        "samsung,exynosautov920-sadk",
        NULL
        },
        {}                              /* empty terminated list */
    };

/* globals */

TASK_ID aboxMachineAttachTaskId = TASK_ID_ERROR;

VXB_DRV aboxMachineDrv =
    {
    {NULL},
    ABOX_MACHINE_NAME,                      /* Name */
    "Abox SADK MACHINE FDT driver",         /* Description */
    VXB_BUSID_FDT,                          /* Class */
    0,                                      /* Flags */
    0,                                      /* Reference count */
    aboxMachineMethodList                   /* Method table */
    };

VXB_DRV_DEF(aboxMachineDrv)

LOCAL STATUS aboxMachineProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxMachineMatch, NULL);
    }

LOCAL STATUS _aboxMachineAttach
    (
    VXB_DEV_ID                      pDev
    )
    {
    VXB_ABOX_MACHINE_DRV_CTRL *     pMachineData;
    VXB_FDT_DEV *                   pFdtDev;
    int32_t                         len;
    int32_t                         offset;
    int32_t                         childoffset;
    int32_t                         index = 0;
    int32_t                         nLink = 0;
    char *                          daiName = NULL;
    const char *                    nodeName;
    BOOL                            foundCodec;
    VX_SND_SOC_CARD *               pCard;
    VX_SND_SOC_DAI_LINK *           pLink;
    VX_SND_SOC_DAI_LINK *           pDaiLink;
    VX_SND_SOC_DAI_LINK_COMPONENT * pDaiLinkCmpnt;
    uint32_t                        sndDaiPhandle;
    int32_t                         sndDaiFdtOfs;
    uint32_t                        sndDaiIdx;
    uint32_t                        n;
    const char *                    daiNodeName;
    BOOL                            cpuDaiEnabled = FALSE;

    DBG_MSG (DBG_INFO, "enter\n");

    /* abox control driver must be ready first */

    if (ABOX_CTR_GLB_DATA == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    if (pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    pMachineData = (VXB_ABOX_MACHINE_DRV_CTRL *)
                        vxbMemAlloc (sizeof (VXB_ABOX_MACHINE_DRV_CTRL));
    if (pMachineData == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    nLink = ELEMENTS (exynosauto9SndCardDaiDefaultLink);
    pLink = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK) * nLink);
    if (pLink == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        goto errOut;
        }

    index = 0;
    offset = pFdtDev->offset;
    for (offset = vxFdtFirstSubnode (offset); offset > 0;
         offset = vxFdtNextSubnode (offset))
        {
        daiName = (char *) vxFdtPropGet (offset, "dai-name", &len);
        if (daiName == NULL)
            {
            DBG_MSG (DBG_ERR, "failed to get dai name.\n");
            goto errOut;
            }

        DBG_MSG (DBG_INFO, "dai %s\n", daiName);

        pDaiLink = daiDefaultLinkGet (daiName);
        if (pDaiLink == NULL)
            {
            DBG_MSG (DBG_ERR, "cannot find dai link: %s\n", daiName);
            goto errOut;
            }

        childoffset   = offset;
        foundCodec    = FALSE;
        cpuDaiEnabled = TRUE;

        for (childoffset = vxFdtFirstSubnode (childoffset); childoffset > 0;
             childoffset = vxFdtNextSubnode (childoffset))
            {
            nodeName = vxFdtGetName (childoffset, &len);
            DBG_MSG (DBG_INFO, "nodeName = %s\n", nodeName);

            if (pDaiLink->cpus == NULL)
                {
                if (!strncmp (nodeName, "cpu", strlen ("cpu")))
                    {
                    if (getSndDaiPhandle (childoffset,
                                          &sndDaiPhandle,
                                          &sndDaiFdtOfs,
                                          &sndDaiIdx) != OK)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO,
                             "cpu sound dai pHandle=0x%x fdtOfs=0x%x idx=0x%x\n",
                             sndDaiPhandle, sndDaiFdtOfs, sndDaiIdx);

                    daiNodeName = vxFdtGetName (sndDaiFdtOfs, &len);
                    DBG_MSG (DBG_INFO, "dai node name:%s\n",
                             (daiNodeName == NULL) ? "N/A" : daiNodeName);

                    if (!vxFdtIsEnabled (sndDaiFdtOfs))
                        {
                        DBG_MSG (DBG_INFO, "sound dai node status is disabled\n");
                        cpuDaiEnabled = FALSE;
                        break;
                        }
                    else
                        {
                        DBG_MSG (DBG_INFO, "sound dai node status is enabled\n");
                        }

                    pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
                    if (pDaiLinkCmpnt == NULL)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    pDaiLinkCmpnt->pDev = vxbFdtDevAcquireByOffset (sndDaiFdtOfs);
                    if (pDaiLinkCmpnt->pDev == NULL)
                        {
                        DBG_MSG (DBG_ERR, "failed to get dai dev ID\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO, "pDaiLinkCmpnt->pDev=0x%llx\n",
                             pDaiLinkCmpnt->pDev);

                    if (sndSocGetDaiName (pDaiLinkCmpnt->pDev,
                                          sndDaiIdx,
                                          &pDaiLinkCmpnt->dai_name) != OK)
                        {
                        DBG_MSG (DBG_ERR, "failed to get dai name.\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO, "pDaiLinkCmpnt->dai_name=%s\n",
                             pDaiLinkCmpnt->dai_name);

                    pDaiLink->cpus = pDaiLinkCmpnt;
                    pDaiLink->cpuNum = 1;
                    }
                }

            if (pDaiLink->platforms == NULL)
                {
                if (!strncmp (nodeName, "platform", strlen ("platform")))
                    {
                    if (getSndDaiPhandle (childoffset,
                                          &sndDaiPhandle,
                                          &sndDaiFdtOfs,
                                          &sndDaiIdx) != OK)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO,
                             "platform sound dai pHandle=0x%x fdtOfs=0x%x idx=0x%x\n",
                             sndDaiPhandle, sndDaiFdtOfs, sndDaiIdx);

                    daiNodeName = vxFdtGetName (sndDaiFdtOfs, &len);
                    DBG_MSG (DBG_INFO, "dai node name:%s\n",
                             (daiNodeName == NULL) ? "N/A" : daiNodeName);

                    pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
                    if (pDaiLinkCmpnt == NULL)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    /* platforms DAI is usused now, just set pDev */

                    pDaiLinkCmpnt->pDev = vxbFdtDevAcquireByOffset (sndDaiFdtOfs);
                    if (pDaiLinkCmpnt->pDev == NULL)
                        {
                        DBG_MSG (DBG_ERR, "failed to get dai dev ID\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO, "pDaiLinkCmpnt->pDev=0x%llx\n",
                             pDaiLinkCmpnt->pDev);

                    pDaiLink->platforms = pDaiLinkCmpnt;
                    pDaiLink->platformNum = 1;
                    }
                }

            if (pDaiLink->codecs == NULL)
                {
                if (!strncmp(nodeName, "codec", strlen ("codec")))
                    {
                    if (getSndDaiPhandle (childoffset,
                                          &sndDaiPhandle,
                                          &sndDaiFdtOfs,
                                          &sndDaiIdx) != OK)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO,
                             "codec sound dai pHandle=0x%x fdtOfs=0x%x idx=0x%x\n",
                             sndDaiPhandle, sndDaiFdtOfs, sndDaiIdx);

                    daiNodeName = vxFdtGetName (sndDaiFdtOfs, &len);
                    DBG_MSG (DBG_INFO, "dai node name:%s\n",
                             (daiNodeName == NULL) ? "N/A" : daiNodeName);

                    pDaiLinkCmpnt = vxbMemAlloc (sizeof (VX_SND_SOC_DAI_LINK_COMPONENT));
                    if (pDaiLinkCmpnt == NULL)
                        {
                        DBG_MSG (DBG_ERR, "ERROR\n");
                        goto errOut;
                        }

                    pDaiLinkCmpnt->pDev = vxbFdtDevAcquireByOffset (sndDaiFdtOfs);
                    if (pDaiLinkCmpnt->pDev == NULL)
                        {
                        DBG_MSG (DBG_ERR, "failed to get dai dev ID\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO, "pDaiLinkCmpnt->pDev=0x%llx\n",
                             pDaiLinkCmpnt->pDev);

                    if (sndSocGetDaiName (pDaiLinkCmpnt->pDev,
                                          sndDaiIdx,
                                          &pDaiLinkCmpnt->dai_name) != OK)
                        {
                        DBG_MSG (DBG_ERR, "failed to get dai name.\n");
                        goto errOut;
                        }

                    DBG_MSG (DBG_INFO, "pDaiLinkCmpnt->dai_name=%s\n",
                             pDaiLinkCmpnt->dai_name);

                    pDaiLink->codecs = pDaiLinkCmpnt;
                    pDaiLink->codecNum = 1;

                    foundCodec = TRUE;
                    }
                }
            }

        if (!cpuDaiEnabled)
            {

            /* in a dai link, cpu dai fdt node must be enabled */

            continue;
            }

        if (!foundCodec)
            {
            pDaiLink->codecs = dummyDaiLinkCmpnt;
            pDaiLink->codecNum = ELEMENTS (dummyDaiLinkCmpnt);
            }

        memcpy (&pLink[index], pDaiLink, sizeof (VX_SND_SOC_DAI_LINK));
        index++;
        if (index == nLink)
            {
            break;
            }
        }

    pMachineData->resetPinctrlId = vxbPinMuxGetIdByName (pDev, "tas6424_reset");
    if (pMachineData->resetPinctrlId == PINMUX_ERROR_ID)
        {
        DBG_MSG (DBG_ERR, "failed to get tas6424_reset pinctrl.\n");
        goto errOut;
        }

    pMachineData->idlePinctrlId = vxbPinMuxGetIdByName (pDev, "tas6424_idle");
    if (pMachineData->idlePinctrlId == PINMUX_ERROR_ID)
        {
        DBG_MSG (DBG_ERR, "failed to get tas6424_idle pinctrl.\n");
        goto errOut;
        }

    pMachineData->pDev = pDev;
    vxbDevSoftcSet (pDev, pMachineData);

    pCard = &exynosauto9SndCard;
    pCard->dev = pDev;
    pCard->dai_link = pLink,
    pCard->num_links = index;
    pCard->drvdata = pMachineData;

    pMachineData->mclk = vxbClkGet (pDev, "mclk");
    if (pMachineData->mclk == NULL)
        {
        DBG_MSG (DBG_ERR, "cannot get mclk\n");
        goto errOut;
        }

    if (vxSndCardRegister (pCard) != OK)
        {
        DBG_MSG (DBG_ERR, "snd_soc_register_card() failed\n");
        goto errOut;
        }

    cardGpioCfg (pCard, pMachineData->resetPinctrlId);

    DBG_MSG (DBG_INFO, "snd_soc_register_card Done\n");
    return OK;

errOut:
    for (n = 0; n < nLink; n++)
        {
        if (exynosauto9SndCardDaiDefaultLink[n].cpus != NULL)
            {
            vxbMemFree (exynosauto9SndCardDaiDefaultLink[n].cpus);
            exynosauto9SndCardDaiDefaultLink[n].cpus = NULL;
            exynosauto9SndCardDaiDefaultLink[n].cpuNum = 0;
            }

        if (exynosauto9SndCardDaiDefaultLink[n].platforms != NULL)
            {
            vxbMemFree (exynosauto9SndCardDaiDefaultLink[n].platforms);
            exynosauto9SndCardDaiDefaultLink[n].platforms = NULL;
            exynosauto9SndCardDaiDefaultLink[n].platformNum = 0;
            }

        if (exynosauto9SndCardDaiDefaultLink[n].codecs != NULL)
            {
            if (exynosauto9SndCardDaiDefaultLink[n].codecs != dummyDaiLinkCmpnt)
                {
                vxbMemFree (exynosauto9SndCardDaiDefaultLink[n].codecs);
                }
            exynosauto9SndCardDaiDefaultLink[n].codecs = NULL;
            exynosauto9SndCardDaiDefaultLink[n].codecNum = 0;
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

LOCAL void aboxMachineAttachTask
    (
    VXB_DEV_ID  pDev
    )
    {
    if (_aboxMachineAttach (pDev) != OK)
        {
        DBG_MSG (DBG_ERR, "abox machine attach init failed\n");
        return;
        }
    }

LOCAL STATUS aboxMachineAttach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;

    if (pDev == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    aboxMachineAttachTaskId = taskCreate (ABOX_MACHINE_ATTACH_TASK_NAME,
                                          ABOX_MACHINE_ATTACH_TASK_PRI,
                                          0,
                                          ABOX_MACHINE_ATTACH_TASK_STACK,
                                          (FUNCPTR) aboxMachineAttachTask,
                                          (_Vx_usr_arg_t) pDev,
                                          0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (aboxMachineAttachTaskId == TASK_ID_ERROR)
        {
        DBG_MSG (DBG_ERR, "ERROR\n");
        return ERROR;
        }

    return OK;
    }

//exynosauto9_dummy_hw_params
LOCAL int32_t dummyHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;
    VX_SND_SOC_DAI *            cpu_dai = SOC_RTD_TO_CPU (rtd, 0);
    uint32_t                    fmt;

    DBG_MSG (DBG_INFO, "%s-0x%llx %dch, %dHz, %d width %d bytes\n",
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
            DBG_MSG (DBG_ERR, "failed to set playback cpu fmt\n");
            return -1;
            }
        }
    else
        {
        if (vxSndSocDaiFmtSet (cpu_dai, fmt
                               | SND_SOC_DAIFMT_NB_NF
                               | SND_SOC_DAIFMT_CBM_CFM) != 0)
            {
            DBG_MSG (DBG_ERR, "failed to set capture cpu fmt\n");
            return -1;
            }
        }

    return 0;
    }

//exynosauto9_suspend_post
LOCAL int32_t exynosauto9SndCardSuspendPost
    (
    VX_SND_SOC_CARD *card
    )
    {
    return 0;
    }

//exynosauto9_resume_pre
LOCAL int32_t exynosauto9SndCardResumePre
    (
    VX_SND_SOC_CARD *card
    )
    {
    return 0;
    }

//exynosauto9_tas6424_hw_params
LOCAL int32_t sadkMachTas6424HwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;
    VX_SND_SOC_CARD *           card = rtd->card;
    VX_SND_SOC_DAI *            cpu_dai = SOC_RTD_TO_CPU (rtd, 0);
    VX_SND_SOC_DAI *            codec_dai = SOC_RTD_TO_CODEC (rtd, 0);
    VXB_ABOX_MACHINE_DRV_CTRL * pMachineData;
    uint32_t                    rate = PARAMS_RATE (params);
    int32_t                     i = 0;
    uint32_t                    tx_mask;
    uint32_t                    rx_mask;
    int32_t                     num_slot;
    int32_t                     slot_width;

    pMachineData = card->drvdata;

    DBG_MSG (DBG_INFO, "%s-0x%llx %dch, %dHz, %d width %dbytes\n",
             rtd->daiLink->name, substream->stream,
             PARAMS_CHANNELS (params), PARAMS_RATE (params),
             PARAMS_WIDTH (params), PARAMS_BUFFER_BYTES (params));

    //In the case of TAS6424 codec, it doesn't support slave mode
    if (PARAMS_CHANNELS (params) > 2)
        {
        //TDM mode
        // CAPTURE using DUMMY CODEC TDM
        if (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE)
            {
            /* Set CPU DAI configuration */
            if (vxSndSocDaiFmtSet (cpu_dai,
                                   VX_SND_DAI_FORMAT_DSP_A
                                   | SND_SOC_DAIFMT_NB_NF
                                   | SND_SOC_DAIFMT_CBM_CFM) != 0)
                {
                DBG_MSG (DBG_ERR, "Failed to set aif1 cpu fmt\n");
                return -1;
                }
            DBG_MSG (DBG_INFO, "Capture using Dummy Codec\n");
            }
        else
            {
            // PLAYBACK
            //cpu dai
            if (vxSndSocDaiFmtSet (cpu_dai,
                                   VX_SND_DAI_FORMAT_DSP_A
                                   | SND_SOC_DAIFMT_NB_NF
                                   | SND_SOC_DAIFMT_CBS_CFS) != 0)
                {
                DBG_MSG (DBG_ERR, "Failed to set aif1 cpu fmt\n");
                return -1;
                }

            // Return if using dummy codec
            if (strcmp (rtd->daiLink->name, "PLAYBACK DUMMY PRI") == 0)
                {
                DBG_MSG (DBG_INFO, "Playback using Dummy Codec\n");
                return 0;
                }

            //codec dai
            for (i = 0; i < rtd->codecNum; i++)
                {
                codec_dai = SOC_RTD_TO_CODEC (rtd, i);
                if (codec_dai == NULL)
                    {
                    continue;
                    }

                if (vxSndSocDaiFmtSet (codec_dai,
                                       VX_SND_DAI_FORMAT_DSP_A
                                       | SND_SOC_DAIFMT_NB_NF
                                       | SND_SOC_DAIFMT_CBS_CFS) != 0)
                    {
                    DBG_MSG (DBG_ERR, "Failed to set codec dai fmt\n");
                    return -1;
                    }

                tx_mask = 0x1;
                rx_mask = 0x10;
                num_slot = 4;
                slot_width = 32;

                if (snd_soc_dai_set_tdm_slot (codec_dai,
                                              tx_mask, rx_mask,
                                              num_slot, slot_width) != OK)
                    {
                    DBG_MSG (DBG_ERR, "Failed to set codec tdm slot\n");
                    return -1;
                    }
                }
            }
        }
    else
        {
        // CAPTURE using DUMMY CODEC I2S
        if (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE)
            {
            /* Set CPU DAI configuration */
            if (vxSndSocDaiFmtSet (cpu_dai,
                                   VX_SND_DAI_FORMAT_I2S
                                   | SND_SOC_DAIFMT_NB_NF
                                   | SND_SOC_DAIFMT_CBM_CFM) != 0)
                {
                DBG_MSG (DBG_ERR, "Failed to set aif1 cpu fmt\n");
                return -1;
                }
            DBG_MSG (DBG_INFO, "Capture using Dummy Codec\n");
            }
        else
            {
            //PLAYBACK
            //cpu dai
            if (vxSndSocDaiFmtSet (cpu_dai,
                                   VX_SND_DAI_FORMAT_I2S
                                   | SND_SOC_DAIFMT_NB_NF
                                   | SND_SOC_DAIFMT_CBS_CFS) != 0)
                {
                DBG_MSG (DBG_ERR, "Failed to set aif1 cpu fmt\n");
                return -1;
                }

            // Return if using dummy codec
            if (strcmp (rtd->daiLink->name, "PLAYBACK DUMMY PRI") == 0)
                {
                DBG_MSG (DBG_INFO, "Playback using Dummy Codec\n");
                return 0;
                }

            //codec dai
            for (i = 0; i < rtd->codecNum; i++)
                {
                codec_dai = SOC_RTD_TO_CODEC (rtd, i);
                if (codec_dai == NULL)
                    {
                    continue;
                    }

                if (vxSndSocDaiFmtSet (codec_dai,
                                       VX_SND_DAI_FORMAT_I2S
                                       | SND_SOC_DAIFMT_NB_NF
                                       | SND_SOC_DAIFMT_CBS_CFS) != 0)
                    {
                    DBG_MSG (DBG_ERR, "Failed to set codec dai fmt\n");
                    return -1;
                    }
                }
            }
        }

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        pMachineData->mclkValue = (rate != 96000) ? (rate * 256) : (rate * 128);

        if (vxbClkEnable (pMachineData->mclk) != OK)
            {
            DBG_MSG (DBG_ERR, "Failed to enable clk_mclk\n");
            return -1;
            }

        (void) aboxClkRateSet (pMachineData->mclk,
                               pMachineData->mclkValue);

        DBG_MSG (DBG_INFO, "MCLK Rate : %ld\n", pMachineData->mclkValue);
        }

    return 0;
    }

//exynosauto9_tas6424_hw_params_free
LOCAL int32_t sadkMachTas6424HwFree
    (
    SND_PCM_SUBSTREAM *         substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;
    VX_SND_SOC_CARD *           card = rtd->card;
    VXB_ABOX_MACHINE_DRV_CTRL * pMachineData;

    pMachineData = card->drvdata;

    if (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED)
        {
        DBG_MSG (DBG_ERR, "DMA is disconnected\n");
        return -1;
        }

    (void) vxbClkDisable (pMachineData->mclk);

    return 0;
    }

//exynosauto9_tas6424_prepare
LOCAL int32_t sadkMachTas6424Prepare
    (
    SND_PCM_SUBSTREAM *         substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;

    if (snd_soc_dai_set_tristate (SOC_RTD_TO_CPU (rtd, 0), 0) != OK)
        {
        DBG_MSG (DBG_ERR, "set dai tristate failed\n");
        return -1;
        }

    return 0;
    }

//exynosauto9_tas6424_shutdown
LOCAL void sadkMachTas6424Shutdown
    (
    SND_PCM_SUBSTREAM *         substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;

    (void) snd_soc_dai_set_tristate (SOC_RTD_TO_CPU (rtd, 0), 1);
    }

//exynosauto9_tlv320adc_hw_params
LOCAL int32_t sadkMachTlv320adcHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    )
    {
    VX_SND_SOC_PCM_RUNTIME *    rtd = substream->privateData;
    VX_SND_SOC_DAI *            cpu_dai = SOC_RTD_TO_CPU (rtd, 0);
    VX_SND_SOC_DAI *            codec_dai = SOC_RTD_TO_CODEC (rtd, 0);
    uint32_t                    cpu_fmt;
    uint32_t                    codec_fmt;

    DBG_MSG(DBG_INFO, "\n");

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        DBG_MSG (DBG_ERR, "Playback Stream isn't supported\n");
        return -1;
        }

    if (PARAMS_CHANNELS(params) > 2)
        {
        //TDM mode
        cpu_fmt = VX_SND_DAI_FORMAT_DSP_A | SND_SOC_DAIFMT_NB_NF;
        codec_fmt = VX_SND_DAI_FORMAT_DSP_A | SND_SOC_DAIFMT_NB_NF;
        }
    else
        {
        cpu_fmt = VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF;
        codec_fmt = VX_SND_DAI_FORMAT_I2S | SND_SOC_DAIFMT_NB_NF;
        }

    if (vxSndSocDaiFmtSet (cpu_dai, cpu_fmt | SND_SOC_DAIFMT_CBM_CFM) != 0)
        {
        DBG_MSG (DBG_ERR, "Failed to set cpu_fmt\n");
        return -1;
        }

    if (vxSndSocDaiFmtSet (codec_dai, codec_fmt | SND_SOC_DAIFMT_CBM_CFM) != 0)
        {
        DBG_MSG (DBG_ERR,"Failed to set codec_fmt\n");
        return -1;
        }

    return 0;
    }

//exynosauto9_cfg_gpio
LOCAL void cardGpioCfg
    (
    VX_SND_SOC_CARD *   card,
    uint32_t            pinctrlId
    )
    {
    if (vxbPinMuxEnableById (card->dev, pinctrlId) != OK)
        {
        DBG_MSG (DBG_ERR, "failed to set pinctrlId=%d\n", pinctrlId);
        }
    }

LOCAL VX_SND_SOC_DAI_LINK * daiDefaultLinkGet
    (
    char *daiName
    )
    {
    VX_SND_SOC_DAI_LINK * pLink = NULL;
    int32_t i = 0;
    int32_t nLink = 0;

    nLink = ELEMENTS (exynosauto9SndCardDaiDefaultLink);
    for (i = 0; i < nLink; i++)
        {
        pLink = &exynosauto9SndCardDaiDefaultLink[i];
        if (strncmp (daiName, pLink->name, strlen(daiName)) == 0)
            {
            return pLink;
            }
        }

    return pLink;
    }

LOCAL STATUS getSndDaiPhandle
    (
    int         fdtOfs,
    uint32_t *  phandle,
    int32_t *   pSndDaiOfs,
    uint32_t *  pSndDaiIdx
    )
    {
    uint32_t *  pSndDai;
    int32_t     sndDaiLen;
    uint32_t *  pSndDaiCell;
    int32_t     sndDaiCellLen;
    uint32_t    daiCells;

    pSndDai = (uint32_t *) vxFdtPropGet (fdtOfs, "sound-dai", &sndDaiLen);
    if (pSndDai == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to get sound dai.\n");
        return ERROR;
        }

    *phandle = vxFdt32ToCpu (*pSndDai); //first arg must be DAI component phandle
    *pSndDaiOfs = vxFdtNodeOffsetByPhandle (*phandle);

    pSndDaiCell = (uint32_t *) vxFdtPropGet (*pSndDaiOfs,
                                             "#sound-dai-cells",
                                             &sndDaiCellLen);
    if (pSndDaiCell == NULL)
        {
        DBG_MSG (DBG_ERR, "failed to get #sound-dai-cells.\n");
        return ERROR;
        }

    daiCells = vxFdt32ToCpu (*pSndDaiCell);
    if (daiCells == 1)  //only one or zero arg is supported
        {
        pSndDai++;
        *pSndDaiIdx = vxFdt32ToCpu (*pSndDai);
        }
    else
        {
        *pSndDaiIdx = 0;
        }

    return OK;
    }

