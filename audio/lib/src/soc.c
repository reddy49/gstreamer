/* soc.c - PCM SoC device driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides a PCM driver that manages all PCM devices.
*/

/* includes */

#include <cacheLib.h>
#include <iosLib.h>
#include <sysLib.h>
#include <scMemVal.h>
#include <subsys/timer/vxbTimerLib.h>
#include <private/iosLibP.h>
#include <pmapLib.h>
#include <vxSoundCore.h>
#include <control.h>
#include <soc.h>

#undef DEBUG_VX_SND_SOC

#ifdef DEBUG_VX_SND_SOC
#include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_SOC_DBG_OFF          0x00000000U
#define VX_SND_SOC_DBG_ERR          0x00000001U
#define VX_SND_SOC_DBG_INFO         0x00000002U
#define VX_SND_SOC_DBG_VERBOSE      0x00000004U
#define VX_SND_SOC_DBG_ALL          0xffffffffU
UINT32 vxSndSocDbgMask = VX_SND_SOC_DBG_ALL;

#define VX_SND_SOC_MSG(mask, fmt, ...)                                  \
    do                                                                  \
        {                                                               \
        if ((vxSndSocDbgMask & (mask)))                                 \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_SOC : [%s,%d] "fmt, __func__,    \
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define SND_SOC_MODE_INFO(...)                                          \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define SND_SOC_ERR(fmt, ...)    \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_ERR,     fmt, ##__VA_ARGS__)
#define SND_SOC_INFO(fmt, ...)   \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_INFO,    fmt, ##__VA_ARGS__)
#define SND_SOC_DBG(fmt, ...)    \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_VERBOSE, fmt, ##__VA_ARGS__)
#else /* DEBUG_VX_SND_SOC */
#define SND_SOC_ERR(x...)      do { } while (FALSE)
#define SND_SOC_INFO(x...)     do { } while (FALSE)
#define SND_SOC_DBG(x...)      do { } while (FALSE)
#define SND_SOC_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_SOC */

/* defines */

#ifndef max
# define max(x, y) ((x) < (y) ? (y) : (x))
#endif

/* forward declarations */

LOCAL uint32_t SetBitCountGet (uint32_t n);
LOCAL BOOL isDaiSupportStream (VX_SND_SOC_DAI * dai, STREAM_DIRECT direct);
LOCAL void vxSndSocDaiActiveCntAdd (VX_SND_SOC_DAI * dai, STREAM_DIRECT direct,
                                    int cnt);
LOCAL void vxSndSocRuntimeActive (VX_SND_SOC_PCM_RUNTIME * runtime,
                                  STREAM_DIRECT direct);
LOCAL void vxSndSocRuntimeDeactive (VX_SND_SOC_PCM_RUNTIME * runtime,
                                    STREAM_DIRECT direct);
LOCAL void vxSocPcmHwDefInit (VX_SND_PCM_HARDWARE * hw);
LOCAL void vxSocPcmHwRateUpdate (VX_SND_PCM_HARDWARE * hw,
                                 VX_SND_SOC_PCM_STREAM * socStream);
LOCAL void vxSocPcmHwChannelUpdate (VX_SND_PCM_HARDWARE * hw,
                                    VX_SND_SOC_PCM_STREAM * socStream);
LOCAL void vxSocPcmHwFormateUpdate (VX_SND_PCM_HARDWARE * hw,
                                    VX_SND_SOC_PCM_STREAM * socStream);
LOCAL STATUS vxSocPcmRuntimeHwInit (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndSocRuntimeHwCalculate (VX_SND_SOC_PCM_RUNTIME * runtime,
                                         VX_SND_PCM_HARDWARE * hw,
                                         STREAM_DIRECT direct);
LOCAL void vxSocPcmClean (SND_PCM_SUBSTREAM * substream, BOOL unNormal);
LOCAL STATUS vxSocPcmOpen (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSocPcmClose (SND_PCM_SUBSTREAM * substream);

LOCAL void vxSocPcmHwClean (SND_PCM_SUBSTREAM * substream, BOOL unnormal);
LOCAL STATUS vxSocPcmHwParams (SND_PCM_SUBSTREAM * substream,
                               VX_SND_PCM_HW_PARAMS * params);
LOCAL STATUS vxSocPcmHwFree (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSocPcmTrigger (SND_PCM_SUBSTREAM * substream, int cmd);
LOCAL STATUS vxSocPcmPrepare (SND_PCM_SUBSTREAM * substream);
LOCAL SND_FRAMES_U_T vxSocPcmPointer (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndSocPcmComponentIoctl (SND_PCM_SUBSTREAM * substream,
                                        uint32_t cmd, void *arg);
LOCAL STATUS vxSocPcmComponentSyncStop (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSocPcmComponentCopy (SND_PCM_SUBSTREAM * substream,
                                    int channel, unsigned long pos,
                                    void * buf, unsigned long bytes);
LOCAL STATUS dpcm_fe_dai_open (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS dpcm_fe_dai_close (SND_PCM_SUBSTREAM * substream);
LOCAL void   dpcm_be_dai_hw_free (VX_SND_SOC_PCM_RUNTIME * feRtd,
                                  STREAM_DIRECT direct);
LOCAL STATUS dpcm_fe_dai_hw_params (SND_PCM_SUBSTREAM * substream,
                                    VX_SND_PCM_HW_PARAMS * params);
LOCAL STATUS dpcm_fe_dai_hw_free (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS dpcm_fe_dai_prepare (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS dpcm_fe_dai_trigger (SND_PCM_SUBSTREAM * substream, int cmd);
LOCAL STATUS dpcm_dai_trigger_fe_be (SND_PCM_SUBSTREAM * substream,
                                     int cmd, BOOL feFirst);
LOCAL STATUS dpcm_be_dai_trigger (VX_SND_SOC_PCM_RUNTIME * feRtd,
                                  STREAM_DIRECT direct, int cmd);

/* locals */

/*******************************************************************************
*
* SetBitCountGet - get number of 1s
*
* This routine gets the number of 1s in a 32-bit unsigned integer
*
* RETURNS: the number of 1s
*
* ERRNO: N/A
*/

LOCAL uint32_t SetBitCountGet
    (
    uint32_t n
    )
    {
    uint32_t count = 0;
    while (n != 0) {
        n &= (n - 1);
        count++;
    }
    return count;
}

/*******************************************************************************
*
* isDaiSupportStream - check the supported direction
*
* This routine checks the supported direction of DAI. If the minimum channels is
* not 0, it supports the direction.
*
* RETURNS: TRUE, or FALSE if not support
*
* ERRNO: N/A
*/

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

/*******************************************************************************
*
* vxSndSocPcmCreate - create PCM
*
* This routine creates PCM accroding to SoC PCM runtime and initializes it.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

//int soc_new_pcm(struct snd_soc_pcm_runtime *rtd, int num)

STATUS vxSndSocPcmCreate
    (
    VX_SND_SOC_PCM_RUNTIME * runtime,
    int                      runtimeIdx
    )
    {
    SND_CARD *             card = runtime->card->sndCard;
    VX_SND_PCM *           pcm;
    char                   pcmName[SND_DEV_NAME_LEN];
    BOOL                   hasPlayback = FALSE;
    BOOL                   hasCapture  = FALSE;
    uint32_t               idx;
    VX_SND_SOC_DAI *       cpuDai;
    VX_SND_SOC_DAI *       codecDai;
    VX_SND_SOC_COMPONENT * component;
    SND_PCM_SUBSTREAM *    substream;
    BOOL                   internal = FALSE;

    /* get PCM name */

    if (runtime->daiLink->noPcm == 1)
        {
        (void) snprintf_s (pcmName, sizeof (pcmName), "(%s)",
                           runtime->daiLink->streamName);
        internal = TRUE;
        }
    else if (runtime->daiLink->dynamic == 1)
        {
        (void) snprintf_s (pcmName, sizeof (pcmName), "(%s)",
                           runtime->daiLink->streamName);
        }
    else
        {
        (void) snprintf_s (pcmName, sizeof (pcmName), "(%s)",
                           runtime->daiLink->streamName);
        }

    if (runtime->daiLink->dynamic == 1 && runtime->cpuNum > 1)
        {
        SND_SOC_ERR ("PCM %s should have only one cpu dai\n", pcmName);
        }

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
                SND_SOC_ERR ("cpu dai number (when > 1) should be equal to "
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

    if (runtime->daiLink->playbackOnly == 1)
        {
        hasPlayback = TRUE;
        hasCapture  = FALSE;
        }

    if (runtime->daiLink->captureOnly == 1)
        {
        hasCapture  = TRUE;
        hasPlayback = FALSE;
        }

    /* create PCM */

    if (vxSndPcmCreate (card, pcmName, runtimeIdx, hasPlayback, hasCapture,
                        internal, &pcm) != OK)
        {
        SND_SOC_ERR ("failed to create PCM %s\n", pcmName);
        return ERROR;
        }

    runtime->pcm     = pcm;
    pcm->privateData = (void *) runtime;

    if (runtime->daiLink->noPcm == 1)
        {
        if (hasPlayback)
            {
            substream = pcm->stream[SNDRV_PCM_STREAM_PLAYBACK].substream;
            substream->privateData = (void *) runtime;
            }
        if (hasCapture)
            {
            substream = pcm->stream[SNDRV_PCM_STREAM_CAPTURE].substream;
            substream->privateData = (void *) runtime;
            }

        return OK;
        }

    /* ASoC PCM operations */
    if (runtime->daiLink->dynamic == 1)
        {
        runtime->ops.open       = dpcm_fe_dai_open;
        runtime->ops.hwParams   = dpcm_fe_dai_hw_params;
        runtime->ops.prepare    = dpcm_fe_dai_prepare;
        runtime->ops.trigger    = dpcm_fe_dai_trigger;
        runtime->ops.hwFree     = dpcm_fe_dai_hw_free;
        runtime->ops.close      = dpcm_fe_dai_close;
        runtime->ops.pointer    = vxSocPcmPointer;
        }
    else
        {
        runtime->ops.open      = vxSocPcmOpen;
        runtime->ops.hwParams  = vxSocPcmHwParams;
        runtime->ops.prepare   = vxSocPcmPrepare;
        runtime->ops.trigger   = vxSocPcmTrigger;
        runtime->ops.hwFree    = vxSocPcmHwFree;
        runtime->ops.close     = vxSocPcmClose;
        runtime->ops.pointer   = vxSocPcmPointer;
        }

    /*
     * Poll all components, if component driver has ioctl/syncStop...
     * operations, then assign related ops to runtime->ops, since
     * runtime->ops will call component driver's ops ioctl/syncStop...
     */

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        const VX_SND_SOC_CMPT_DRV * drv = component->driver;

        if (drv->ioctl != NULL)
            {
            runtime->ops.ioctl    = vxSndSocPcmComponentIoctl;
            }
        if (drv->syncStop != NULL)
            {
            runtime->ops.syncStop = vxSocPcmComponentSyncStop;
            }
        if (drv->copy != NULL)
            {
            runtime->ops.copy     = vxSocPcmComponentCopy;
            }
        if (drv->ack != NULL)
            {
            runtime->ops.ack      = vxSocPcmComponentAck;
            }
        }

    if (hasPlayback)
        {
        substream = pcm->stream[SNDRV_PCM_STREAM_PLAYBACK].substream;
        substream->ops = &runtime->ops;
        }

    if (hasCapture)
        {
        substream = pcm->stream[SNDRV_PCM_STREAM_CAPTURE].substream;
        substream->ops = &runtime->ops;
        }

FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
    {
    if (component->driver->pcmConstruct != NULL &&
        component->driver->pcmConstruct (component, runtime) != OK)
        {
        SND_SOC_ERR ("component(%s)->pcmConstruct() error\n",
                     component->name);
        return ERROR;
        }
    }

    return OK;
    }

LOCAL STATUS dpcm_fe_dai_open
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd     = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_PCM_RUNTIME *     feRuntime = substream->runtime;
    VX_SND_PCM_HARDWARE *    hw        = &feRuntime->hw;
    STREAM_DIRECT            direct    = substream->stream->direct;
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    VX_SND_SOC_DAI *         dai;
    uint32_t                 idx;
    SND_PCM_SUBSTREAM *      beSubstream;

#ifdef DEBUG_VX_SND_SOC
    SND_SOC_MODE_INFO ("\ndpcm_fe_dai_open\n\tDPCM list:[FE:%s]",
                       feRtd->daiLink->name);
    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        SND_SOC_MODE_INFO (" -> [BE:%s]", beRtd->daiLink->name);
        }
    SND_SOC_MODE_INFO ("\n\n");
#endif

    /* all back end substreams open */

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }
        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_NEW &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_CLOSE)
            {
            continue;
            }
        beSubstream->runtime = feRuntime;
        if (vxSocPcmOpen (beSubstream) != OK)
            {
            beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_CLOSE;
            SND_SOC_ERR ("BE: substream(%s) vxSocPcmOpen() error\n",
                         beSubstream->name);
            return ERROR;
            }
        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_OPEN;
        }

    /* front end open */

    if (vxSocPcmOpen (substream) != OK)
        {
        SND_SOC_ERR ("FE: substream(%s) vxSocPcmOpen() error\n",
                     substream->name);
        return ERROR;
        }
    feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_OPEN;

    /*
     * runtime->hw was repeated initialized and calculated in every
     * vxSocPcmOpen(). For dpcm hw, it needs to be re-calculated according to
     * all FE/BE parameters at the same time.
     */

    /* dpcm_runtime_setup_fe(fe_substream) */

    vxSocPcmHwDefInit (hw);

    FOR_EACH_RUNTIME_CPU_DAIS (feRtd, idx, dai)
        {
        VX_SND_SOC_PCM_STREAM * cpuStream;
        if (!isDaiSupportStream (dai, direct))
            {
            continue;
            }
        cpuStream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                    &dai->driver->playback : &dai->driver->capture;

        vxSocPcmHwRateUpdate (hw, cpuStream);
        vxSocPcmHwChannelUpdate (hw, cpuStream);
        vxSocPcmHwFormateUpdate (hw, cpuStream);
        }

    /* dpcm_runtime_setup_be_format(fe_substream) */

    if (feRtd->daiLink->dpcm_merged_format != 0)
        {
        FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
            {
            VX_SND_SOC_PCM_STREAM * codecStream;

            FOR_EACH_RUNTIME_CODEC_DAIS (beRtd, idx, dai)
                {
                if (!isDaiSupportStream (dai, direct))
                    {
                    continue;
                    }
                codecStream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                              &dai->driver->playback : &dai->driver->capture;
                vxSocPcmHwFormateUpdate (hw, codecStream);
                }
            }
        }

    /* dpcm_runtime_setup_be_chan(fe_substream) */

    if (feRtd->daiLink->dpcm_merged_chan != 0)
        {
        FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
            {
            VX_SND_SOC_PCM_STREAM * cpuStream;

            FOR_EACH_RUNTIME_CPU_DAIS (beRtd, idx, dai)
                {
                if (!isDaiSupportStream (dai, direct))
                    {
                    continue;
                    }
                cpuStream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                            &dai->driver->playback : &dai->driver->capture;
                vxSocPcmHwChannelUpdate (hw, cpuStream);
                }

            if (beRtd->codecNum == 1)
                {
                VX_SND_SOC_DAI * codecDai = SOC_RTD_TO_CODEC (beRtd, 0);
                VX_SND_SOC_PCM_STREAM * codecStream =
                           direct == SNDRV_PCM_STREAM_PLAYBACK ?
                           &codecDai->driver->playback : &codecDai->driver->capture;
                vxSocPcmHwChannelUpdate (hw, codecStream);
                }
            }
        }

    /* dpcm_runtime_setup_be_rate(fe_substream) */

    if (feRtd->daiLink->dpcm_merged_rate != 0)
        {
        FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
            {
            VX_SND_SOC_PCM_STREAM * stream;

            FOR_EACH_RUNTIME_DAIS (beRtd, idx, dai)
                {
                if (!isDaiSupportStream (dai, direct))
                    {
                    continue;
                    }
                stream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                        &dai->driver->playback : &dai->driver->capture;
                vxSocPcmHwRateUpdate (hw, stream);
                }
            }
        }

#ifdef DEBUG_VX_SND_SOC
        SND_SOC_MODE_INFO ("\n(%s)hardware supported parameters:", __func__);
        SND_SOC_MODE_INFO ("\thw->info =\t0x%08llx\n", hw->info);
        SND_SOC_MODE_INFO ("\thw->formats =\t0x%08x\n", hw->formats);
        SND_SOC_MODE_INFO ("\thw->rates =\t0x%08x\n", hw->rates);
        SND_SOC_MODE_INFO ("\thw->channelsMin =\t0x%08x\n", hw->channelsMin);
        SND_SOC_MODE_INFO ("\thw->channelsMax =\t0x%08x\n", hw->channelsMax);
        SND_SOC_MODE_INFO ("\thw->bufferBytesMax =\t0x%08x\n",
                           hw->bufferBytesMax);
        SND_SOC_MODE_INFO ("\thw->periodBytesMin =\t0x%08x\n",
                           hw->periodBytesMin);
        SND_SOC_MODE_INFO ("\thw->periodBytesMax =\t0x%08x\n",
                           hw->periodBytesMax);
        SND_SOC_MODE_INFO ("\thw->periodsMin =\t0x%08x\n", hw->periodsMin);
        SND_SOC_MODE_INFO ("\thw->periodsMax =\t0x%08x\n", hw->periodsMax);
        SND_SOC_MODE_INFO ("\n\n");
#endif

    return OK;
    }

LOCAL STATUS dpcm_fe_dai_close
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd  = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct = substream->stream->direct;
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    SND_PCM_SUBSTREAM *      beSubstream;

    /* all back end substreams close */

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }

        /* hw_free if necessary before close() */

        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_OPEN &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_FREE)
            {
            (void) vxSocPcmHwFree (beSubstream);
            beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_FREE;
            }

        (void) vxSocPcmClose (beSubstream);
        beSubstream->runtime = NULL;
        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_CLOSE;
        }

    /* front end close */

    (void) vxSocPcmClose (substream);
    feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_CLOSE;

    return OK;
    }

LOCAL void dpcm_be_dai_hw_free
    (
    VX_SND_SOC_PCM_RUNTIME * feRtd,
    STREAM_DIRECT            direct
    )
    {
    SND_PCM_SUBSTREAM *      beSubstream;
    VX_SND_SOC_PCM_RUNTIME * beRtd;

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return;
            }

        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PREPARE &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_PARAMS &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_FREE &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PAUSED &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_SUSPEND &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_STOP)
            {
            continue;
            }

        (void) vxSocPcmHwFree (beSubstream);

        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_FREE;
        }
    }

LOCAL STATUS dpcm_fe_dai_hw_params
    (
    SND_PCM_SUBSTREAM *    substream,
    VX_SND_PCM_HW_PARAMS * params
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd  = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct = substream->stream->direct;
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    SND_PCM_SUBSTREAM *      beSubstream;

    (void) memcpy_s (&feRtd->dpcmUserParams, sizeof (VX_SND_PCM_HW_PARAMS),
                     params, sizeof (VX_SND_PCM_HW_PARAMS));

    /* all back end substreams hw_params */

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }

        (void) memcpy_s (&beRtd->dpcmUserParams, sizeof (VX_SND_PCM_HW_PARAMS),
                         params, sizeof (VX_SND_PCM_HW_PARAMS));

        if (sndSocLinkBeHwParamsFixup (beRtd, &beRtd->dpcmUserParams) != OK)
            {
            SND_SOC_ERR ("BE: beRtd(%s) sndSocLinkBeHwParamsFixup() error\n",
                         beRtd->daiLink->name);
            goto BeHwFree;
            }

        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_OPEN &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_PARAMS &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_FREE)
            {
            continue;
            }

        if (vxSocPcmHwParams (beSubstream, &beRtd->dpcmUserParams) != OK)
            {
            SND_SOC_ERR ("BE: substream(%s) vxSocPcmHwParams() error\n",
                         beSubstream->name);
            return ERROR;
            }
        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_PARAMS;
        }

    /* front end hwParams */

    if (vxSocPcmHwParams (substream, params) != OK)
        {
        SND_SOC_ERR ("FE: substream(%s) vxSocPcmHwParams() error\n",
                     substream->name);
        dpcm_be_dai_hw_free (feRtd, direct);
        return ERROR;
        }
    feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_PARAMS;

    return OK;

BeHwFree:
      FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }

        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_OPEN &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_PARAMS &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_FREE &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_STOP)
            {
            continue;
            }

        (void) vxSocPcmHwFree (beSubstream);

        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_FREE;
        }

    return ERROR;
    }

LOCAL STATUS dpcm_fe_dai_hw_free
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct    = substream->stream->direct;

    (void) vxSocPcmHwFree (substream);

    dpcm_be_dai_hw_free (feRtd, direct);
    feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_HW_FREE;

    return OK;
    }

LOCAL STATUS dpcm_fe_dai_prepare
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct    = substream->stream->direct;
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    SND_PCM_SUBSTREAM *      beSubstream;

    /* all back end substreams prepare */

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }

        if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_HW_PARAMS &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_STOP      &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_SUSPEND   &&
            beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PAUSED)
            {
            continue;
            }

        if (vxSocPcmPrepare (beSubstream) != OK)
            {
            SND_SOC_ERR ("BE: substream(%s) vxSocPcmPrepare() error\n",
                         beSubstream->name);
            return ERROR;
            }

        beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_PREPARE;
        }

    /* front end close */

    if (vxSocPcmPrepare (substream) != OK)
        {
        SND_SOC_ERR ("FE: substream(%s) vxSocPcmPrepare() error\n",
                     substream->name);
        return ERROR;
        }
    feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_PREPARE;

    return OK;
    }

LOCAL STATUS dpcm_fe_dai_trigger
    (
    SND_PCM_SUBSTREAM * substream,
    int                 cmd
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd   = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct  = substream->stream->direct;
    VX_SND_SOC_DPCM_TRIGGER  trigger = feRtd->daiLink->trigger[direct];

    switch (trigger)
        {
        case SND_SOC_DPCM_TRIGGER_PRE:
            switch (cmd)
                {
                case SNDRV_PCM_TRIGGER_START:
                case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
                case SNDRV_PCM_TRIGGER_DRAIN:
                    if (dpcm_dai_trigger_fe_be(substream, cmd, TRUE) != OK)
                        {
                        SND_SOC_ERR ("substream(%s) failed to execute cmd-%d\n",
                                     cmd);
                        return ERROR;
                        }
                    break;
                case SNDRV_PCM_TRIGGER_STOP:
                case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
                    if (dpcm_dai_trigger_fe_be(substream, cmd, FALSE) != OK)
                        {
                        SND_SOC_ERR ("substream(%s) failed to execute cmd-%d\n",
                                     cmd);
                        return ERROR;
                        }
                    break;
                default:
                    SND_SOC_ERR ("substream(%s) doesn't support cmd-%d\n", cmd);
                    return ERROR;
                }
            break;
        case SND_SOC_DPCM_TRIGGER_POST:
            switch (cmd)
                {
                case SNDRV_PCM_TRIGGER_START:
                case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
                case SNDRV_PCM_TRIGGER_DRAIN:
                    if (dpcm_dai_trigger_fe_be(substream, cmd, FALSE) != OK)
                        {
                        SND_SOC_ERR ("substream(%s) failed to execute cmd-%d\n",
                                     cmd);
                        return ERROR;
                        }
                    break;
                case SNDRV_PCM_TRIGGER_STOP:
                case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
                    if (dpcm_dai_trigger_fe_be(substream, cmd, TRUE) != OK)
                        {
                        SND_SOC_ERR ("substream(%s) failed to execute cmd-%d\n",
                                     cmd);
                        return ERROR;
                        }
                    break;
                default:
                    SND_SOC_ERR ("substream(%s) doesn't support cmd-%d\n", cmd);
                    return ERROR;
                }
            break;
        default:
            SND_SOC_ERR ("substream(%s) trigger mode error: trigger-%d\n",
                         trigger);
            return ERROR;
        }

    /* update state */

    switch (cmd)
        {
        case SNDRV_PCM_TRIGGER_START:
        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
            feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_START;
            break;
        case SNDRV_PCM_TRIGGER_STOP:
            feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_STOP;
            break;
        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
            feRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_PAUSED;
            break;
        default:
            break;
        }

    return OK;
    }

LOCAL STATUS dpcm_dai_trigger_fe_be
    (
    SND_PCM_SUBSTREAM * substream,
    int                 cmd,
    BOOL                feFirst
    )
    {
    VX_SND_SOC_PCM_RUNTIME * feRtd  = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    STREAM_DIRECT            direct = substream->stream->direct;

    if (feFirst)
        {
        if (vxSocPcmTrigger (substream, cmd) != OK)
            {
            SND_SOC_ERR ("FE:substream(%s) trigger cmd-%d failed\n",
                         substream->name, cmd);
            return ERROR;
            }

        if (dpcm_be_dai_trigger(feRtd, direct, cmd) != OK)
            {
            SND_SOC_ERR ("substream(%s)'s BE trigger cmd-%d failed\n",
                         substream->name, cmd);
            return ERROR;
            }
        }
    else
        {
        if (dpcm_be_dai_trigger(feRtd, direct, cmd) != OK)
            {
            SND_SOC_ERR ("substream(%s)'s BE trigger cmd-%d failed\n",
                         substream->name, cmd);
            return ERROR;
            }

        if (vxSocPcmTrigger (substream, cmd) != OK)
            {
            SND_SOC_ERR ("FE:substream(%s) trigger cmd-%d failed\n",
                         substream->name, cmd);
            return ERROR;
            }
        }

    return OK;
    }

LOCAL STATUS dpcm_be_dai_trigger
    (
    VX_SND_SOC_PCM_RUNTIME * feRtd,
    STREAM_DIRECT            direct,
    int                      cmd
    )
    {
    VX_SND_SOC_PCM_RUNTIME * beRtd;
    SND_PCM_SUBSTREAM *      beSubstream;

    /* all back end substreams prepare */

    FOR_EACH_SOC_BE_RTD (feRtd, direct, beRtd)
        {
        beSubstream = SOC_GET_SUBSTREAM_FROM_RTD (beRtd, direct);
        if (beSubstream == NULL)
            {
            SND_SOC_ERR ("BE: beRtd(%s) doesn't have a substream\n",
                         beRtd->daiLink->name);
            return ERROR;
            }

        switch (cmd)
            {
            case SNDRV_PCM_TRIGGER_START:
                if ((beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PREPARE) &&
                    (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_STOP)    &&
                    (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PAUSED))
                    {
                    continue;
                    }

                if (vxSocPcmTrigger (beSubstream, cmd) != OK)
                    {
                    SND_SOC_ERR ("BE: substream(%s) trigger cmd-%d failed\n",
                                 beSubstream->name, cmd);
                    return ERROR;
                    }

                beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_START;
                break;

            case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
                if ((beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PAUSED))
                    {
                    continue;
                    }

                if (vxSocPcmTrigger (beSubstream, cmd) != OK)
                    {
                    SND_SOC_ERR ("BE: substream(%s) trigger cmd-%d failed\n",
                                 beSubstream->name, cmd);
                    return ERROR;
                    }

                beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_START;
                break;

            case SNDRV_PCM_TRIGGER_STOP:
                if ((beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_START) &&
                    (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_PAUSED))
                    {
                    continue;
                    }

                if (vxSocPcmTrigger (beSubstream, cmd) != OK)
                    {
                    SND_SOC_ERR ("BE: substream(%s) trigger cmd-%d failed\n",
                                 beSubstream->name, cmd);
                    return ERROR;
                    }

                beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_STOP;
                break;

            case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
                if (beRtd->dpcmState[direct] != SND_SOC_DPCM_STATE_START)
                    {
                    continue;
                    }

                if (vxSocPcmTrigger (beSubstream, cmd) != OK)
                    {
                    SND_SOC_ERR ("BE: substream(%s) trigger cmd-%d failed\n",
                                 beSubstream->name, cmd);
                    return ERROR;
                    }

                beRtd->dpcmState[direct] = SND_SOC_DPCM_STATE_PAUSED;
                break;

            default:
                break;
            }
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndSocDaiStartup - DAI startup
*
* This routine calls the startup() of DAI driver.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_dai_startup(struct snd_soc_dai *dai,
//                        struct snd_pcm_substream *substream)
STATUS vxSndSocDaiStartup
    (
    VX_SND_SOC_DAI *    dai,
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (dai->driver->ops != NULL && dai->driver->ops->startup != NULL &&
        dai->driver->ops->startup (substream, dai) != OK)
        {
        SND_SOC_ERR ("DAI(%s) startup error\n", dai->name);
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndSocDaiShutdown - DAI shutdown
*
* This routine calls the shutdown() of DAI driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//void snd_soc_dai_shutdown(struct snd_soc_dai *dai,
//                          struct snd_pcm_substream *substream,  int rollback)
void vxSndSocDaiShutdown
    (
    VX_SND_SOC_DAI *    dai,
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (dai->driver->ops != NULL && dai->driver->ops->shutdown != NULL)
        {
        dai->driver->ops->shutdown (substream, dai);
        }
    }

/*******************************************************************************
*
* vxSndSocRuntimeHwparamsSet - copy hw parameters
*
* This routine copys hw parameters of DAI link runtime. It should be called in
* component's open() or DAI startup().
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxSndSocRuntimeHwparamsSet
    (
    SND_PCM_SUBSTREAM *         substream,
    const VX_SND_PCM_HARDWARE * hw
    )
    {
    substream->runtime->hw = *hw;
    }

/*******************************************************************************
*
* vxSndSocDaiActiveCntAdd - add DAI and component active count
*
* This routine adds DAI and component active count. The value added can be
* negative.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
// snd_soc_dai_action
LOCAL void vxSndSocDaiActiveCntAdd
    (
    VX_SND_SOC_DAI * dai,
    STREAM_DIRECT    direct,
    int              cnt
    )
    {
    dai->streamActiveCnt[direct] += cnt;
    dai->component->activeCnt    += cnt;
    }

/*******************************************************************************
*
* vxSndSocDaiActiveAllCntGet - get sum of all stream active counts
*
* This routine gets sum of all stream active counts.
*
* RETURNS: active count
*
* ERRNO: N/A
*/

uint32_t vxSndSocDaiActiveAllCntGet
    (
    VX_SND_SOC_DAI * dai
    )
    {
    uint32_t       activeCnt = 0;
    STREAM_DIRECT  direct;

    FOR_EACH_PCM_STREAMS (direct)
        {
        activeCnt += dai->streamActiveCnt[direct];
        }
    return activeCnt;
    }

/*******************************************************************************
*
* vxSndSocDaiActiveAllCntGet - get specified stream active count
*
* This routine gets specified stream active count.
*
* RETURNS: active count
*
* ERRNO: N/A
*/
//snd_soc_dai_stream_active
uint32_t vxSndSocDaiActiveStreamCntGet
    (
    VX_SND_SOC_DAI * dai,
    STREAM_DIRECT    direct
    )
    {
    return dai->streamActiveCnt[direct];
    }

/*******************************************************************************
*
* vxSndSocComponentActiveCntGet - get component active count
*
* This routine gets component active count.
*
* RETURNS: active count
*
* ERRNO: N/A
*/
//snd_soc_component_active
uint32_t vxSndSocComponentActiveCntGet
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    return component->activeCnt;
    }

/*******************************************************************************
*
* vxSndSocRuntimeActive - active all DAIs
*
* This routine actives all DAIs.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//int snd_soc_runtime_activate(struct snd_soc_dai *dai)
LOCAL void vxSndSocRuntimeActive
    (
    VX_SND_SOC_PCM_RUNTIME * runtime,
    STREAM_DIRECT            direct
    )
    {
    VX_SND_SOC_DAI * dai;
    uint32_t         idx;

    FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
        {
        vxSndSocDaiActiveCntAdd (dai, direct, 1);
        }
    }

 /*******************************************************************************
 *
 * vxSndSocRuntimeDeactive - deactive all DAIs
 *
 * This routine deactives all DAIs.
 *
 * RETURNS: N/A
 *
 * ERRNO: N/A
 */
//static inline void snd_soc_runtime_deactivate(struct snd_soc_pcm_runtime *rtd,
//                                              int stream)
 LOCAL void vxSndSocRuntimeDeactive
     (
     VX_SND_SOC_PCM_RUNTIME * runtime,
     STREAM_DIRECT            direct
     )
     {
     VX_SND_SOC_DAI * dai;
     uint32_t         idx;

     FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
         {
         vxSndSocDaiActiveCntAdd (dai, direct, -1);
         }
     }

/*******************************************************************************
*
* vxSocPcmHwDefInit - initialize hardware parameters to default values
*
* This routine initializes hardware parameters to default values.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//static void soc_pcm_hw_init(struct snd_pcm_hardware *hw)
LOCAL void vxSocPcmHwDefInit
    (
    VX_SND_PCM_HARDWARE * hw
    )
    {
    hw->rates       = UINT_MAX;
    hw->rateMin     = 0;
    hw->rateMax     = UINT_MAX;
    hw->channelsMin = 0;
    hw->channelsMax = UINT_MAX;
    hw->formats     = ULLONG_MAX;
    }

/*******************************************************************************
*
* vxSocPcmHwRateUpdate - update hardware parameters
*
* This routine updates hardware paramters with the specified stream parameters.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//static void soc_pcm_hw_update_rate(struct snd_pcm_hardware *hw,
//                                   struct snd_soc_pcm_stream *p)

LOCAL void vxSocPcmHwRateUpdate
    (
    VX_SND_PCM_HARDWARE *   hw,
    VX_SND_SOC_PCM_STREAM * socStream
    )
    {
    VX_SND_PCM_HARDWARE hwStream;

    hwStream.rates   = socStream->rates;
    hwStream.rateMin = socStream->rateMin;
    hwStream.rateMax = socStream->rateMax;

    (void) vxSndPcmHwRateToBitmap (&hwStream);

    hw->rates &= hwStream.rates;
    }

/*******************************************************************************
*
* vxSocPcmHwChannelUpdate - update hardware channles
*
* This routine updates hardware channles with the specified stream parameters.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

//static void soc_pcm_hw_update_chan
LOCAL void vxSocPcmHwChannelUpdate
    (
    VX_SND_PCM_HARDWARE *   hw,
    VX_SND_SOC_PCM_STREAM * socStream
    )
    {
    hw->channelsMin = max (hw->channelsMin, socStream->channelsMin);
    hw->channelsMax = min (hw->channelsMax, socStream->channelsMax);
    }

/*******************************************************************************
*
* vxSocPcmHwFormateUpdate - update hardware formats
*
* This routine updates hardware formats with the specified stream parameters.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSocPcmHwFormateUpdate
    (
    VX_SND_PCM_HARDWARE *   hw,
    VX_SND_SOC_PCM_STREAM * socStream
    )
    {
    hw->formats &= socStream->formats;
    }

/*******************************************************************************
*
* vxSocPcmRuntimeHwInit - initialize hardware parameters
*
* This routine initializes hardware parameters with all the DAIs' parameters.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static void soc_pcm_init_runtime_hw(struct snd_pcm_substream *substream)
LOCAL STATUS vxSocPcmRuntimeHwInit
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_HARDWARE * hw         = &substream->runtime->hw;
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    uint64_t preFormats              = hw->formats;

    if (vxSndSocRuntimeHwCalculate (runtime, hw,
                                    substream->stream->direct) != OK)
        {
        SND_SOC_ERR ("hw calculate error\n");
        return ERROR;
        }

    if (preFormats != 0)
        {
        hw->formats &= preFormats;
        }
    return OK;
    }

/*******************************************************************************
*
* vxSndSocRuntimeHwCalculate - calculate the max/min hardware parameters
*
* This routine calculates the max/min hardware parameters supported by all DAIs.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

LOCAL STATUS vxSndSocRuntimeHwCalculate
    (
    VX_SND_SOC_PCM_RUNTIME * runtime,
    VX_SND_PCM_HARDWARE *    hw,
    STREAM_DIRECT            direct
    )
    {
    VX_SND_SOC_DAI *        dai;
    uint32_t                idx;
    VX_SND_SOC_PCM_STREAM * socStream;
    uint32_t                tempChannelMin;
    uint32_t                tempChannelMax;

    /* need to initialize hw to default? may contain some info */

    vxSocPcmHwDefInit (hw);

    FOR_EACH_RUNTIME_CPU_DAIS (runtime, idx, dai)
        {
        if (!isDaiSupportStream (dai, direct))
            {
            continue;
            }
        socStream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                    &dai->driver->playback : &dai->driver->capture;

        vxSocPcmHwRateUpdate (hw, socStream);
        vxSocPcmHwChannelUpdate (hw, socStream);
        vxSocPcmHwFormateUpdate (hw, socStream);
        }

    tempChannelMin = hw->channelsMin;
    tempChannelMax = hw->channelsMax;

    FOR_EACH_RUNTIME_CODEC_DAIS (runtime, idx, dai)
        {
        if (!isDaiSupportStream (dai, direct))
            {
            continue;
            }
        socStream = direct == SNDRV_PCM_STREAM_PLAYBACK ?
                    &dai->driver->playback : &dai->driver->capture;

        vxSocPcmHwRateUpdate (hw, socStream);
        vxSocPcmHwChannelUpdate (hw, socStream);
        vxSocPcmHwFormateUpdate (hw, socStream);
        }

    if (hw->channelsMin == 0)
        {
        SND_SOC_ERR ("hw->channelsMin should not be 0\n");
        return ERROR;
        }

    if (runtime->codecNum > 1)
        {
        hw->channelsMax = tempChannelMax;
        hw->channelsMin = tempChannelMin;
        }

    return OK;
    }

/*******************************************************************************
*
* vxSocPcmClean - clean PCM
*
* This routine deactives DAIs, shutdown DAI link and close all components.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//static int soc_pcm_clean(struct snd_pcm_substream *substream, int rollback)
LOCAL void vxSocPcmClean
    (
    SND_PCM_SUBSTREAM * substream,
    BOOL                unNormal
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    VX_SND_SOC_DAI *         dai;
    int idx;

    if (!unNormal)
        {
        vxSndSocRuntimeDeactive (runtime, substream->stream->direct);

        FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
            {
            (void) vxSndSocDaiShutdown (dai, substream);
            }

        if (runtime->daiLink->ops != NULL &&
            runtime->daiLink->ops->shutdown != NULL)
            {
            runtime->daiLink->ops->shutdown (substream);
            }

        FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
            {
            if (component->driver->close != NULL)
                {
                (void) component->driver->close (component, substream);
                }

            }
        }

    /* pinctrl pm set sleep ... */
    }

/*******************************************************************************
*
* vxSocPcmOpen - open PCM substream
*
* This routine opens PCM substream, include initializing hardware, calling
* DAI/DAI link startup() and component open().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int soc_pcm_open(struct snd_pcm_substream *substream)
LOCAL STATUS vxSocPcmOpen
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    VX_SND_SOC_DAI *         dai;
    VX_SND_PCM_HARDWARE *    hw;
    uint32_t                 idx;

    if (runtime == NULL)
        {
        SND_SOC_ERR ("runtime pointer of substream %s is NULL\n",
                     substream->name);
        return ERROR;
        }

    /* handle components pinctrl ... */

    /*
     * Open all components on DAI link runtime.
     * vxSndSocRuntimeHwparamsSet() sets the hardware parameters to
     * runtime->hw in component open().
     */

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->open != NULL &&
            component->driver->open (component, substream) != OK)
            {
            SND_SOC_ERR ("component(%s)->driver->open() error\n",
                         component->name);
            goto errOut;
            }
        }

    /* startup DAI link */

    if (runtime->daiLink->ops != NULL &&
        runtime->daiLink->ops->startup != NULL &&
        runtime->daiLink->ops->startup (substream) != OK)
        {
        SND_SOC_ERR ("runtime->daiLink->ops->startup() error\n");
        goto errOut;
        }

    /* startup all DAIs on DAI link*/

    FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
        {
        if (vxSndSocDaiStartup (dai, substream) != OK)
            {
            goto errOut;
            }
        if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
            {
            dai->tdmTxMask = 0;
            }
        else
            {
            dai->tdmRxMask = 0;
            }
        }

    /* below compatible checks for dynamic DAI not do here */

    if (runtime->daiLink->dynamic == 1 || runtime->daiLink->noPcm)
        {
        goto dynamicSkip;
        }

    if (vxSocPcmRuntimeHwInit (substream) != OK)
        {
        goto errOut;
        }

    /* todo: playback/capture rate/channel/sample_bits symmetry check ... */

    /* hardware parameters sanity check */

    hw = &substream->runtime->hw;
    if (hw->rates == 0 || hw->formats == 0 || hw->channelsMax == 0 ||
        hw->channelsMin == 0 || hw->channelsMin > hw->channelsMax)
        {
        SND_SOC_ERR ("component(%s)->driver->open() error\n",
                     component->name);
        goto errOut;
        }

    /*
     * to do:
     * Get maximum from all dai->driver->capture/playback->sigBits and
     * add it to hardware parameters chek rules.
     * soc_pcm_apply_msb()
     * set symmetry rate/channel/sample_bits
     * soc_pcm_apply_symmetry()
     */

dynamicSkip:

    vxSndSocRuntimeActive (runtime, substream->stream->direct);
    return OK;

errOut:
    vxSocPcmClean (substream, TRUE);

    return ERROR;
    }

/*******************************************************************************
*
* vxSocPcmOpen - close PCM substream
*
* This routine closes and shuts down DAI, DAI link and component.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int soc_pcm_close(struct snd_pcm_substream *substream)
LOCAL STATUS vxSocPcmClose
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    vxSocPcmClean (substream, FALSE);

    return OK;
    }

/*******************************************************************************
*
* sndSocLinkBeHwParamsFixup - fix up BE hardware parameters
*
* This routine fixes up BE hardware parameters.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_link_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
//                                    struct snd_pcm_hw_params *params)
STATUS sndSocLinkBeHwParamsFixup
    (
    VX_SND_SOC_PCM_RUNTIME * runtime,
    VX_SND_PCM_HW_PARAMS *   params
    )
    {
    if (runtime->daiLink->be_hw_params_fixup != NULL)
        {
        return runtime->daiLink->be_hw_params_fixup (runtime, params);
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndSocDaiDigitalMute - mute DAI digital signal
*
* This routine mutes DAI digital signal.
*
* RETURNS: OK, or ERROR if failed to mute
*
* ERRNO: N/A
*/
//int snd_soc_dai_digital_mute(struct snd_soc_dai *dai, int mute, int direction)
STATUS vxSndSocDaiDigitalMute
    (
    VX_SND_SOC_DAI * dai,
    BOOL             mute,
    STREAM_DIRECT    direct
    )
    {
    if (dai->driver->ops != NULL && dai->driver->ops->muteStream != NULL &&
        (direct == SNDRV_PCM_STREAM_PLAYBACK ||
         dai->driver->ops->noCaptureMute == 0))
        {
        return dai->driver->ops->muteStream (dai, mute, direct);
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndSocDaiHwFree - free DAI hardware parameters
*
* This routine frees DAI hardware parameters.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void vxSndSocDaiHwFree
    (
    VX_SND_SOC_DAI *    dai,
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (dai->driver->ops != NULL && dai->driver->ops->hwFree != NULL)
        {
        (void) dai->driver->ops->hwFree (substream, dai);
        }
    }

/*******************************************************************************
*
* vxSocPcmHwClean - clean DAIs hardware parameters
*
* This routine cleans DAIs hardware parameters and mutes DAIs.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSocPcmHwClean
    (
    SND_PCM_SUBSTREAM * substream,
    BOOL                unnormal
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_DAI *         dai;
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
        {
        if (vxSndSocDaiActiveAllCntGet (dai) == 1)
            {
            dai->rate       = 0;
            dai->channels   = 0;
            dai->sampleBits = 0;
            }
        if (vxSndSocDaiActiveStreamCntGet (dai, substream->stream->direct) == 1)
            {
            (void) vxSndSocDaiDigitalMute (dai, TRUE,
                                           substream->stream->direct);
            }
        }

    if (!unnormal)
        {

        /* DAI link hwFree() */

        if (runtime->daiLink->ops != NULL &&
            runtime->daiLink->ops->hwFree != NULL)
            {
            (void) runtime->daiLink->ops->hwFree (substream);
            }

        /* component hwFree() */

        FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
            {
            if (component->driver->hwFree != NULL &&
                component->driver->hwFree (component, substream) != OK)
                {
                SND_SOC_ERR ("component(%s)->driver->hwFree() error\n",
                             component->name);
                }
            }

        /* DAI hwFree() */

        FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
            {
            if (!isDaiSupportStream (dai, substream->stream->direct))
                {
                continue;
                }
            vxSndSocDaiHwFree (dai, substream);
            }

        }
    }

/*******************************************************************************
*
* vxSocPcmHwParams - set substream hardware parameters
*
* This routine sets substream hardware parameters.
*
* RETURNS: OK, or ERROR if failed to set
*
* ERRNO: N/A
*/
//static int soc_pcm_hw_params(struct snd_pcm_substream *substream,
//                             struct snd_pcm_hw_params *params)
LOCAL STATUS vxSocPcmHwParams
    (
    SND_PCM_SUBSTREAM *    substream,
    VX_SND_PCM_HW_PARAMS * params
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_DAI *         cpuDai;
    VX_SND_SOC_DAI *         codecDai;
    VX_SND_SOC_COMPONENT *   component;
    uint32_t                 channels;
    int                      idx;

    /* to do: parameters symmetry check ...*/

    /* DAI link hwParams() */

    if (runtime->daiLink->ops != NULL &&
        runtime->daiLink->ops->hwParams != NULL &&
        runtime->daiLink->ops->hwParams (substream, params) != OK)
        {
        SND_SOC_ERR ("runtime->daiLink->ops->hwParams() error\n");
        goto errOut;
        }

    FOR_EACH_RUNTIME_CODEC_DAIS(runtime, idx, codecDai)
        {
        VX_SND_PCM_HW_PARAMS codecParams;

        if (!isDaiSupportStream (codecDai, substream->stream->direct))
            {
            continue;
            }

        codecParams = *params;

        /*
         * TDM slot may be updated in DAI link hwParams(), and channels in
         * in supportHwParams need to be updated too.
         */

        if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK &&
            codecDai->tdmTxMask != 0)
            {
            channels = SetBitCountGet (codecDai->tdmTxMask);
            codecParams.intervals[VX_SND_HW_PARAM_CHANNELS].min = channels;
            codecParams.intervals[VX_SND_HW_PARAM_CHANNELS].max = channels;
            }

        if (substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE &&
            codecDai->tdmRxMask != 0)
            {
            channels = SetBitCountGet (codecDai->tdmRxMask);
            codecParams.intervals[VX_SND_HW_PARAM_CHANNELS].min = channels;
            codecParams.intervals[VX_SND_HW_PARAM_CHANNELS].max = channels;
            }

        /* DAI hwParams */

        if (codecDai->driver->ops != NULL &&
            codecDai->driver->ops->hwParams != NULL)
            {
            if (sndSocLinkBeHwParamsFixup (runtime, &codecParams) != OK)
                {
                SND_SOC_ERR ("sndSocLinkBeHwParamsFixup() error\n");
                goto errOut;
                }
            if (codecDai->driver->ops->hwParams (substream, &codecParams,
                                                 codecDai) != OK)
                {
                SND_SOC_ERR ("runtime->daiLink->ops->hwParams() error\n");
                goto errOut;
                }
            }

        codecDai->rate       = PARAMS_RATE (&codecParams);
        codecDai->channels   = PARAMS_CHANNELS (&codecParams);
        codecDai->sampleBits =
                          vxSndPcmFmtPhyWidthGet (PARAMS_FORMAT (&codecParams));
        }

    FOR_EACH_RUNTIME_CPU_DAIS (runtime, idx, cpuDai)
        {
        if (!isDaiSupportStream (cpuDai, substream->stream->direct))
            {
            continue;
            }

        /* DAI hwParams */

        if (cpuDai->driver->ops != NULL &&
            cpuDai->driver->ops->hwParams != NULL)
            {
            if (sndSocLinkBeHwParamsFixup (runtime, params) != OK)
                {
                SND_SOC_ERR ("sndSocLinkBeHwParamsFixup() error\n");
                goto errOut;
                }
            if (cpuDai->driver->ops->hwParams (substream, params,
                                                 cpuDai) != OK)
                {
                SND_SOC_ERR ("runtime->daiLink->ops->hwParams() error\n");
                goto errOut;
                }
            }

        cpuDai->rate       = PARAMS_RATE (params);
        cpuDai->channels   = PARAMS_CHANNELS (params);
        cpuDai->sampleBits = vxSndPcmFmtPhyWidthGet (PARAMS_FORMAT (params));
        }

    /* components hwParams() */

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->hwParams != NULL &&
            component->driver->hwParams (component, substream, params) != OK)
            {
            SND_SOC_ERR ("component->driver->hwParams() error\n");
            goto errOut;
            }
        }

    return OK;

errOut:
    vxSocPcmHwClean (substream, TRUE);

    return ERROR;
    }

/*******************************************************************************
*
* vxSocPcmHwFree - free substream hardware parameters
*
* This routine frees substream hardware parameters.
*
* RETURNS: OK, or ERROR if failed to free
*
* ERRNO: N/A
*/

LOCAL STATUS vxSocPcmHwFree
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    vxSocPcmHwClean (substream, FALSE);
    return OK;
    }

/*******************************************************************************
*
* vxSocPcmTrigger - trigger substream command
*
* This routine triggers substream command.
*
* RETURNS: OK, or ERROR if failed to trigger
*
* ERRNO: N/A
*/
//static int soc_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
LOCAL STATUS vxSocPcmTrigger
    (
    SND_PCM_SUBSTREAM * substream,
    int                 cmd
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_DAI *         dai;
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    if (cmd == SNDRV_PCM_TRIGGER_START ||
        cmd == SNDRV_PCM_TRIGGER_PAUSE_RELEASE)
        {

        /* DAI link trigger() */

        if (runtime->daiLink->ops != NULL &&
            runtime->daiLink->ops->trigger != NULL &&
            runtime->daiLink->ops->trigger (substream, cmd) != OK)
            {
            goto errOut;
            }

        /* components trigger() */

        FOR_EACH_RUNTIME_COMPONENTS(runtime, idx, component)
            {
            if (component->driver->trigger != NULL &&
                component->driver->trigger (component, substream,
                                            cmd) != OK)
                {
                goto errOut;
                }
            }

        /* DAIs trigger() */

        FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
            {
            if (dai->driver->ops != NULL && dai->driver->ops->trigger != NULL &&
                dai->driver->ops->trigger (substream, cmd, dai) != OK)
                {
                goto errOut;
                }
            }
        }
    else if (cmd == SNDRV_PCM_TRIGGER_STOP ||
        cmd == SNDRV_PCM_TRIGGER_PAUSE_PUSH)
        {
        if (runtime->daiLink->stopDmaFirst == 1)
            {

            /* components trigger() */

            FOR_EACH_RUNTIME_COMPONENTS(runtime, idx, component)
                {
                if (component->driver->trigger != NULL &&
                    component->driver->trigger (component, substream,
                                                cmd) != OK)
                    {
                    goto errOut;
                    }
                }

            /* DAIs trigger() */

            FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
                {
                if (dai->driver->ops != NULL &&
                    dai->driver->ops->trigger != NULL &&
                    dai->driver->ops->trigger (substream, cmd, dai) != OK)
                    {
                    goto errOut;
                    }
                }
            }
        else
            {

            /* DAIs trigger() */

            FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
                {
                if (dai->driver->ops != NULL &&
                    dai->driver->ops->trigger != NULL &&
                    dai->driver->ops->trigger (substream, cmd, dai) != OK)
                    {
                    goto errOut;
                    }
                }

            /* components trigger() */

            FOR_EACH_RUNTIME_COMPONENTS(runtime, idx, component)
                {
                if (component->driver->trigger != NULL &&
                    component->driver->trigger (component, substream,
                                                cmd) != OK)
                    {
                    goto errOut;
                    }
                }
            }

        /* DAI link trigger() */

        if (runtime->daiLink->ops != NULL &&
            runtime->daiLink->ops->trigger != NULL &&
            runtime->daiLink->ops->trigger (substream, cmd) != OK)
            {
            goto errOut;
            }
        }
    return OK;

errOut:
    SND_SOC_ERR ("trigger error\n");
    return ERROR;
    }

/*******************************************************************************
*
* vxSocPcmPrepare - prepare substream
*
* This routine prepares substream before start to playback/capture.
*
* RETURNS: OK, or ERROR if failed to prepare
*
* ERRNO: N/A
*/
//static int soc_pcm_prepare(struct snd_pcm_substream *substream)
LOCAL STATUS vxSocPcmPrepare
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_DAI *         dai;
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    /* DAI link prepare() */

    if (runtime->daiLink->ops != NULL && runtime->daiLink->ops->prepare != NULL)
        {
        if (runtime->daiLink->ops->prepare (substream) != OK)
            {
            SND_SOC_ERR ("DAI link parepare() error\n");
            goto errOut;
            }
        }

    /* component prepare() */

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->prepare != NULL &&
            component->driver->prepare (component, substream) != OK)
            {
            SND_SOC_ERR ("component(%s)->driver->prepare() error\n",
                         component->name);
            goto errOut;
            }
        }

    /* DAI prepare */

    FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
        {
        if (dai->driver->ops != NULL && dai->driver->ops->prepare != NULL)
            {
            if (dai->driver->ops->prepare (substream, dai) != OK)
                {
                SND_SOC_ERR ("dai(%s)->driver->prepare() error\n",
                             dai->name);
                goto errOut;
                }
            }
        }

    /* if support delay work to reduce pop noise, cancel it */

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK &&
        runtime->popWait == 1)
        {
        runtime->popWait = 0;
        }

    /* unmute all DAIs */

    FOR_EACH_RUNTIME_DAIS (runtime, idx, dai)
        {
        (void) vxSndSocDaiDigitalMute (dai, FALSE, substream->stream->direct);
        }

    return OK;

errOut:
    SND_SOC_ERR ("prepare() error\n");

    return ERROR;
    }

/*******************************************************************************
*
* vxSocPcmPointer - get DMA buffer hardware pointer
*
* This routine gets DMA buffer hardware pointer.
*
* RETURNS: OK, or ERROR if failed to get
*
* ERRNO: N/A
*/
//static snd_pcm_uframes_t soc_pcm_pointer(struct snd_pcm_substream *substream)
LOCAL SND_FRAMES_U_T vxSocPcmPointer
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    SND_FRAMES_U_T           offset  = 0;
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->pointer != NULL)
            {
            offset = component->driver->pointer (component, substream);
            break;
            }
        }

    /* if support runtime->delay, add cpuDai and codecDai delay */

    return offset;
    }

/*******************************************************************************
*
* vxSndSocPcmComponentIoctl - call component ioctl()
*
* This routine calls component ioctl().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_pcm_component_ioctl(struct snd_pcm_substream *substream,
//                               unsigned int cmd, void *arg)
LOCAL STATUS vxSndSocPcmComponentIoctl
    (
    SND_PCM_SUBSTREAM * substream,
    uint32_t            cmd,
    void *              arg
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    /*
     * Use first found component ioctl(). If all components have on ioctl(),
     * this routine will not be registered to runtime->ops->ioctl.
     */

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->ioctl != NULL)
            {
            return component->driver->ioctl (component, substream, cmd, arg);
            }
        }

    return sndPcmDefaultIoctl (substream, cmd, arg);
    }

/*******************************************************************************
*
* vxSocPcmComponentSyncStop - call component syncStop()
*
* This routine calls component syncStop().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_pcm_component_sync_stop(struct snd_pcm_substream *substream)
LOCAL STATUS vxSocPcmComponentSyncStop
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->syncStop != NULL)
            {
            return component->driver->syncStop (component,substream);
            }
        }

    return OK;
    }

/*******************************************************************************
*
* vxSocPcmComponentCopy - call component copy()
*
* This routine calls component copy().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_pcm_component_copy_user
LOCAL STATUS vxSocPcmComponentCopy
    (
    SND_PCM_SUBSTREAM * substream,
    int                 channel,
    unsigned long       pos,
    void *              buf,
    unsigned long       bytes
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->copy != NULL)
            {
            return component->driver->copy (component, substream, channel, pos,
                                            buf, bytes);
            }
        }

    return ERROR;
    }

/*******************************************************************************
*
* vxSocPcmComponentCopy - call component ack()
*
* This routine calls component ack().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_pcm_component_ack(struct snd_pcm_substream *substream)
STATUS vxSocPcmComponentAck
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime = VX_SOC_SUBSTREAM_TO_RUNTIME (substream);
    VX_SND_SOC_COMPONENT *   component;
    int                      idx;

    FOR_EACH_RUNTIME_COMPONENTS (runtime, idx, component)
        {
        if (component->driver->ack != NULL)
            {
            return component->driver->ack (component, substream);
            }
        }

    return OK;
    }

//unsigned int snd_soc_component_read(struct snd_soc_component *component,
//                                    unsigned int reg)

uint32_t vxSndSocComponentRead
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg
    )
    {
    uint32_t val;

    if (component->driver == NULL || component->driver->read == NULL)
        {
        SND_SOC_ERR ("component (%s) has no read()\n", component->name);
        return (uint32_t)-1;
        }

    (void) semTake (component->ioMutex, WAIT_FOREVER);
    val = component->driver->read (component, reg);
    (void) semGive (component->ioMutex);

    return val;
    }

uint32_t vxSndSocComponentReadUnlock
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg
    )
    {
    uint32_t val;

    if (component->driver == NULL || component->driver->read == NULL)
        {
        SND_SOC_ERR ("component (%s) has no read()\n", component->name);
        return (uint32_t)-1;
        }

    val = component->driver->read (component, reg);

    return val;
    }

STATUS vxSndSocComponentWriteUnlock
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg,
    uint32_t               val
    )
    {
    STATUS   status;

    if (component->driver == NULL || component->driver->write == NULL)
        {
        SND_SOC_ERR ("component (%s) has no write()\n", component->name);
        return ERROR;
        }

    status = component->driver->write (component, reg, val);

    return status;
    }

//int snd_soc_component_write(struct snd_soc_component *component,
//                            unsigned int reg, unsigned int val)

STATUS vxSndSocComponentWrite
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg,
    uint32_t               val
    )
    {
    STATUS   status;

    if (component->driver == NULL || component->driver->write == NULL)
        {
        SND_SOC_ERR ("component (%s) has no write()\n", component->name);
        return ERROR;
        }

    (void) semTake (component->ioMutex, WAIT_FOREVER);
    status = component->driver->write (component, reg, val);
    (void) semGive (component->ioMutex);

    return status;
    }

//int snd_soc_component_update_bits(struct snd_soc_component *component,
//                                  unsigned int reg, unsigned int mask,
//                                  unsigned int val)

STATUS vxSndSocComponentUpdate
    (
    VX_SND_SOC_COMPONENT * component,
    uint32_t               reg,
    uint32_t               mask,
    uint32_t               data
    )
    {
    STATUS   status;
    uint32_t regVal;

    if (component->driver == NULL || component->driver->read == NULL ||
        component->driver->write == NULL)
        {
        SND_SOC_ERR ("component (%s) has no read() or write()\n",
                     component->name);
        return ERROR;
        }

    (void) semTake (component->ioMutex, WAIT_FOREVER);
    regVal = component->driver->read (component, reg);

    regVal = (regVal & ~mask) | (data & mask);

    status = component->driver->write (component, reg, regVal);
    (void) semGive (component->ioMutex);

    return status;
    }

/*******************************************************************************
*
* vxSndSocComponentCtrlsAdd - add controls to a component
*
* This routine adds a list of controls to a component.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_add_component_controls(struct snd_soc_component *component,
//                                   const struct snd_kcontrol_new *controls,
//                                   unsigned int num_controls)
STATUS vxSndSocComponentCtrlsAdd
    (
    VX_SND_SOC_COMPONENT * component,
    VX_SND_CONTROL *       ctrlList,
    uint32_t               ctrlNum
    )
    {
    SND_CARD * card = component->card->sndCard;

    return vxSndControlsAdd (card, ctrlList, ctrlNum, component->namePrefix,
                             component);
    }

/*******************************************************************************
*
* vxSndSocDaiCtrlsAdd - add controls to a DAI
*
* This routine adds a list of controls to a DAI.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_add_dai_controls(struct snd_soc_dai *dai,
//                             const struct snd_kcontrol_new *controls,
//                             int num_controls)
STATUS vxSndSocDaiCtrlsAdd
    (
    VX_SND_SOC_DAI * dai,
    VX_SND_CONTROL * ctrlList,
    uint32_t         ctrlNum
    )
    {
    SND_CARD * card = dai->component->card->sndCard;

    return vxSndControlsAdd (card, ctrlList, ctrlNum, NULL, dai);
    }

//int snd_soc_add_card_controls(struct snd_soc_card *soc_card,
//                              const struct snd_kcontrol_new *controls,
//                              int num_controls)
STATUS vxSndSocCardCtrlsAdd
    (
    VX_SND_SOC_CARD * socCard,
    VX_SND_CONTROL *  ctrlList,
    int               ctrlNum
    )
    {
    SND_CARD * card = socCard->sndCard;

    return vxSndControlsAdd (card, ctrlList, ctrlNum, NULL, card);
    }

/*******************************************************************************
*
* vxSndCtrlVolumeInfo - get the volume control information
*
* This routine gets the specified volume control information.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_info_volsw(struct snd_kcontrol *kcontrol,
//                       struct snd_ctl_elem_info *uinfo)
STATUS vxSndCtrlVolumeInfo
    (
    VX_SND_CONTROL *   ctrl,
    VX_SND_CTRL_INFO * info
    )
    {
    VX_SND_MIXER_CTRL * mixerCtrl = (VX_SND_MIXER_CTRL *) ctrl->privateValue;

    if (mixerCtrl == NULL)
        {
        return ERROR;
        }

    if (mixerCtrl->platformMax == 0)
        {
        mixerCtrl->platformMax = mixerCtrl->max;
        }

    if (mixerCtrl->platformMax == 1)
        {
        info->type = VX_SND_CTL_DATA_TYPE_BOOLEAN;
        }
    else
        {
        info->type = VX_SND_CTL_DATA_TYPE_INTEGER;
        }

    info->count = MIXER_CTRL_IS_STEREO (mixerCtrl) ? 2 : 1;
    info->value.integer32.min = 0;
    info->value.integer32.max = mixerCtrl->platformMax - mixerCtrl->min;

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlVolumeGet - get the volume control value
*
* This routine gets the specified volume control value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_get_volsw(struct snd_kcontrol *kcontrol,
//                      struct snd_ctl_elem_value *ucontrol)
STATUS vxSndCtrlVolumeGet
    (
    VX_SND_CONTROL *       ctrl,
    VX_SND_CTRL_DATA_VAL * data
    )
    {
    VX_SND_MIXER_CTRL *    mixerCtrl = (VX_SND_MIXER_CTRL *) ctrl->privateValue;
    VX_SND_SOC_COMPONENT * cmpt    = (VX_SND_SOC_COMPONENT *) ctrl->privateData;
    uint32_t               mask    = (1 << ffs32Msb (mixerCtrl->max)) - 1;
    uint32_t               value;

    if (mixerCtrl == NULL || cmpt == NULL)
        {
        return ERROR;
        }

    value = vxSndSocComponentRead (cmpt, mixerCtrl->reg);
    value = (value >> mixerCtrl->shift) & mask;

    value -= mixerCtrl->min;
    if (mixerCtrl->invert != 0)
        {
        value = mixerCtrl->max - value;
        }

    data->value.integer32.value[0] = value;

    if (MIXER_CTRL_IS_STEREO (mixerCtrl))
        {
        value = vxSndSocComponentRead (cmpt, mixerCtrl->rreg);
        if (mixerCtrl->reg == mixerCtrl->rreg)
            {
            value = (value >> mixerCtrl->shiftRight) & mask;
            }
        else
            {
            value = (value >> mixerCtrl->shift) & mask;
            }

        value -= mixerCtrl->min;
        if (mixerCtrl->invert != 0)
            {
            value = mixerCtrl->max - value;
            }

        data->value.integer32.value[1] = value;
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlVolumePut - set the volume control value
*
* This routine sets the specified volume control value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_put_volsw(struct snd_kcontrol *kcontrol,
//                      struct snd_ctl_elem_value *ucontrol)
STATUS vxSndCtrlVolumePut
    (
    VX_SND_CONTROL *       ctrl,
    VX_SND_CTRL_DATA_VAL * data
    )
    {
    VX_SND_MIXER_CTRL *    mixerCtrl = (VX_SND_MIXER_CTRL *) ctrl->privateValue;
    VX_SND_SOC_COMPONENT * cmpt    = (VX_SND_SOC_COMPONENT *) ctrl->privateData;
    uint32_t               mask    = (1 << ffs32Msb (mixerCtrl->max)) - 1;
    uint32_t               value;
    uint32_t               valueR;
    uint32_t               regMask;
    BOOL                   oneReg  = TRUE;

    if (mixerCtrl == NULL || cmpt == NULL)
        {
        return ERROR;
        }

    /* left channel */

    value = data->value.integer32.value[0];
    if (mixerCtrl->platformMax > 0 &&
        (value + mixerCtrl->min) > mixerCtrl->platformMax)
        {
        SND_SOC_ERR ("value (%d) out of range, platformMax (%d)\n",
                     value + mixerCtrl->min, mixerCtrl->platformMax);
        return ERROR;
        }

    if (value > mixerCtrl->max - mixerCtrl->min)
        {
        SND_SOC_ERR ("value (%d) is greater than max-min (%d)\n",
                     value, mixerCtrl->max - mixerCtrl->min);
        return ERROR;
        }

    value = (value + mixerCtrl->min) & mask;
    if (mixerCtrl->invert > 0)
        {
        value = mixerCtrl->max - value;
        }

    value <<= mixerCtrl->shift;

    regMask = mask << mixerCtrl->shift;

    /* right channel */

    if (MIXER_CTRL_IS_STEREO (mixerCtrl))
        {
        valueR = data->value.integer32.value[1];
        if (mixerCtrl->platformMax > 0 &&
            (valueR + mixerCtrl->min) > mixerCtrl->platformMax)
            {
            SND_SOC_ERR ("value (%d) out of range, platformMax (%d)\n",
                         valueR + mixerCtrl->min, mixerCtrl->platformMax);
            return ERROR;
            }

        if (valueR > mixerCtrl->max - mixerCtrl->min)
            {
            SND_SOC_ERR ("value (%d) is greater than max-min (%d)\n",
                         valueR, mixerCtrl->max - mixerCtrl->min);
            return ERROR;
            }

        valueR = (valueR + mixerCtrl->min) & mask;
        if (mixerCtrl->invert > 0)
            {
            valueR = mixerCtrl->max - valueR;
            }

        if (mixerCtrl->reg == mixerCtrl->rreg)
            {
            regMask |= mask << mixerCtrl->shiftRight;
            value |= valueR << mixerCtrl->shiftRight;
            oneReg = TRUE;
            }
        else
            {
            oneReg = FALSE;
            valueR <<= mixerCtrl->shift;
            }
        }

    if (vxSndSocComponentUpdate (cmpt, mixerCtrl->reg, regMask,
                                 value) != OK)
        {
        SND_SOC_ERR ("failed to update all channels' reg\n");
        return ERROR;
        }

    if (!oneReg)
        {
        if (vxSndSocComponentUpdate (cmpt, mixerCtrl->rreg, regMask,
                                     valueR) != OK)
            {
            SND_SOC_ERR ("failed to update right channel reg\n");
            return ERROR;
            }
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlEnumValToItem - convert enum value to item index
*
* This routine converts enum value to item index.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static inline unsigned int snd_soc_enum_val_to_item(struct soc_enum *e,
//                                                    unsigned int val)
uint32_t vxSndCtrlEnumValToItem
    (
    VX_SND_ENUM * pEnum,
    uint32_t      value
    )
    {
    uint32_t idx;

    /* if valueList is NULL, enum value is the item index */

    if (pEnum->valueList == NULL)
        {
        return value;
        }

    for (idx = 0; idx < pEnum->itemNum; idx++)
        {
        if (pEnum->valueList[idx] == value)
            {
            return idx;
            }
        }

    /* not found, always return 0 */

    return 0;
    }

/*******************************************************************************
*
* vxSndCtrlItemToEnumVal - convert item index to enum value
*
* This routine converts item index to enum value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static inline unsigned int snd_soc_enum_item_to_val(struct soc_enum *e,
//                                                    unsigned int item)
uint32_t vxSndCtrlItemToEnumVal
    (
    VX_SND_ENUM * pEnum,
    uint32_t      item
    )
    {

    /* if valueList is NULL, enum value is the item index */

    if (pEnum->valueList == NULL)
        {
        return item;
        }

    return pEnum->valueList[item];
    }

/*******************************************************************************
*
* vxSndCtrlEnumInfo - get the overall info and the selected one enum text
*
* This routine gets the overall enum number and returns the selected one enum
* text.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_info_enum_double(struct snd_kcontrol *kcontrol,
//                             struct snd_ctl_elem_info *uinfo)
STATUS snd_soc_info_enum_double
    (
    VX_SND_CONTROL *   ctrl,
    VX_SND_CTRL_INFO * info
    )
    {
    VX_SND_ENUM * pEnum = (VX_SND_ENUM *) ctrl->privateValue;
    size_t        size;

    SND_SOC_DBG("enter info: %s\n", info->id.name);

    if (pEnum == NULL)
        {
        return ERROR;
        }

    info->type  = VX_SND_CTL_DATA_TYPE_ENUMERATED;
    info->count = pEnum->shiftLeft == pEnum->shiftRight ? 1 : 2;
    info->value.enumerated.itemSum = pEnum->itemNum;
    if (pEnum->itemNum == 0)
        {
        return OK;
        }

    if (info->value.enumerated.itemSelected >= pEnum->itemNum)
        {
        info->value.enumerated.itemSelected = pEnum->itemNum - 1;
        }

    size = strlen (pEnum->textList[info->value.enumerated.itemSelected]);
    if (size >= sizeof (info->value.enumerated.name))
        {
        SND_SOC_ERR ("item name '%s' is too long\n",
                     pEnum->textList[info->value.enumerated.itemSelected]);
        }

    /* copy selected item text */

    size = strlen(pEnum->textList[info->value.enumerated.itemSelected]);

    (void) strncpy_s (info->value.enumerated.name,
                      SND_DEV_NAME_LEN,
                      pEnum->textList[info->value.enumerated.itemSelected],
                      size);

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlEnumGet - get enum item value
*
* This routine gets the specified enum item value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_get_enum_double(struct snd_kcontrol *kcontrol,
//                            struct snd_ctl_elem_value *ucontrol)

STATUS snd_soc_get_enum_double
    (
    VX_SND_CONTROL *       ctrl,
    VX_SND_CTRL_DATA_VAL * data
    )
    {
    VX_SND_ENUM *          pEnum = (VX_SND_ENUM *) ctrl->privateValue;
    VX_SND_SOC_COMPONENT * cmpt  = (VX_SND_SOC_COMPONENT *) ctrl->privateData;
    uint32_t               regVal;
    uint32_t               value;
    uint32_t               itemVal;

    if (pEnum == NULL || cmpt == NULL)
        {
        return ERROR;
        }

    pEnum->mask = roundupPowerOfTwoPri (pEnum->itemNum) - 1;

    /* get current selected item from hardware register */

    regVal  = vxSndSocComponentRead (cmpt, pEnum->reg);
    value   = (regVal >> pEnum->shiftLeft) & pEnum->mask;
    itemVal = vxSndCtrlEnumValToItem (pEnum, value);
    data->value.enumerated.item[0] = itemVal;

    if (pEnum->shiftLeft != pEnum->shiftRight)
        {
        value =  (regVal >> pEnum->shiftRight) & pEnum->mask;
        itemVal = vxSndCtrlEnumValToItem (pEnum, value);
        data->value.enumerated.item[1] = itemVal;
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlEnumGet - set enum item
*
* This routine gets the specified enum item.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_soc_put_enum_double(struct snd_kcontrol *kcontrol,
//                            struct snd_ctl_elem_value *ucontrol)
STATUS snd_soc_put_enum_double
    (
    VX_SND_CONTROL *       ctrl,
    VX_SND_CTRL_DATA_VAL * data
    )
    {
    VX_SND_ENUM *          pEnum = (VX_SND_ENUM *) ctrl->privateValue;
    VX_SND_SOC_COMPONENT * cmpt  = (VX_SND_SOC_COMPONENT *) ctrl->privateData;
    uint32_t *             item  = data->value.enumerated.item;
    uint32_t               regVal;
    uint32_t               mask;

    if (pEnum == NULL || cmpt == NULL)
        {
        return ERROR;
        }

    if (item[0] >= pEnum->itemNum)
        {
        SND_SOC_ERR ("item[0]=%d is greater than items count %d \n",
                     item[0], pEnum->itemNum);
        return ERROR;
        }

    pEnum->mask = roundupPowerOfTwoPri (pEnum->itemNum) - 1;

    regVal = vxSndCtrlItemToEnumVal (pEnum, item[0]) << pEnum->shiftLeft;
    mask   = pEnum->mask << pEnum->shiftLeft;

    if (pEnum->shiftLeft != pEnum->shiftRight)
        {
        if (item[1] >= pEnum->itemNum)
            {
            SND_SOC_ERR ("item[1]=%d is greater than items count %d \n",
                         item[1], pEnum->itemNum);
            return ERROR;
            }
        regVal |= vxSndCtrlItemToEnumVal (pEnum, item[1]) << pEnum->shiftRight;
        mask   |= pEnum->mask << pEnum->shiftRight;
        }

    if (vxSndSocComponentUpdate (cmpt, pEnum->reg, mask, regVal) != OK)
        {
        SND_SOC_ERR ("failed to put enum item\n");
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* sndSocCardrtdFindByName - find specifed VX_SND_SOC_PCM_RUNTIME
*
* This routine finds the specified VX_SND_SOC_PCM_RUNTIME of soc card by name.
*
* RETURNS: VX_SND_SOC_PCM_RUNTIME pointer, or NULL if failed to find
*
* ERRNO: N/A
*/

VX_SND_SOC_PCM_RUNTIME * sndSocCardrtdFindByName
    (
    VX_SND_SOC_CARD * socCard,
    const char *      rtdName
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtime;

    if (rtdName == NULL)
        {
        SND_SOC_ERR ("rtdName should not be NULL\n");
        return NULL;
        }

    FOR_EACH_SOC_CARD_RUNTIME (socCard, runtime)
        {
        if (runtime->daiLink == NULL)
            {
            SND_SOC_ERR ("runtime->daiLink should not be NULL\n");
            return NULL;
            }
        if (strstr (runtime->daiLink->name, rtdName) != NULL)
            {
            SND_SOC_DBG ("find VX_SND_SOC_PCM_RUNTIME, name is %s\n",
                         runtime->daiLink->name);
            return runtime;
            }
        }

    return NULL;
    }

/*******************************************************************************
*
* sndSocBeConnectByName - connect two VX_SND_SOC_PCM_RUNTIMEs
*
* This routine connects a VX_SND_SOC_PCM_RUNTIME to another
* VX_SND_SOC_PCM_RUNTIME. If 'nameBack' is NULL, it will connect the first to
* NULL.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

STATUS sndSocBeConnectByName
    (
    VX_SND_SOC_CARD * socCard,
    const char *      nameFront,
    const char *      nameBack,
    STREAM_DIRECT     direct
    )
    {
    VX_SND_SOC_PCM_RUNTIME * runtimeFront;
    VX_SND_SOC_PCM_RUNTIME * runtimeBack;

    if (socCard == NULL || nameFront == NULL || direct >= SNDRV_PCM_STREAM_MAX)
        {
        SND_SOC_ERR ("invalid arguments\n");
        return ERROR;
        }

    /* find two VX_SND_SOC_PCM_RUNTIME */

    runtimeFront = sndSocCardrtdFindByName (socCard, nameFront);
    if (runtimeFront == NULL)
        {
        SND_SOC_ERR ("failed to find front runtime:%s\n",
                     nameFront);
        return ERROR;
        }

    if (nameBack != NULL)
        {
        runtimeBack = sndSocCardrtdFindByName (socCard, nameBack);
        if (runtimeBack == NULL)
            {
            SND_SOC_ERR ("failed to find back runtime:%s\n",
                         nameBack);
            return ERROR;
            }
        }
    else
        {
        runtimeBack = NULL;
        }

    /* connect Front -> Back */

    runtimeFront->pBe[direct] = runtimeBack;

    SND_SOC_DBG ("connect %s(Front) -> %s(Back)\n", nameFront,
                 nameBack == NULL ? "NULL" : nameBack);
    return OK;
    }

//int snd_soc_dai_set_fmt(VX_SND_SOC_DAI *dai, uint32_t fmt)

int32_t vxSndSocDaiFmtSet
    (
    VX_SND_SOC_DAI * dai,
    uint32_t         fmt
    )
    {
    int32_t ret = -ENOTSUP;

    if (dai->driver->ops != NULL &&  dai->driver->ops->setFmt != NULL)
        {
        if (dai->driver->ops->setFmt (dai, fmt) != OK)
            {
            ret = -1;
            }
        else
            {
            ret = 0;
            }
        }

    return ret;
    }

STATUS snd_soc_dai_set_tristate
    (
    VX_SND_SOC_DAI * dai,
    int              tristate
    )
    {
    STATUS ret = ERROR;

    if (dai->driver->ops != NULL &&
        dai->driver->ops->set_tristate != NULL)
        {
        ret = dai->driver->ops->set_tristate (dai, tristate);
        }

    return ret;
    }

STATUS snd_soc_dai_set_tdm_slot
    (
    VX_SND_SOC_DAI * dai,
    uint32_t         tx_mask,
    uint32_t         rx_mask,
    int32_t          slots,
    int32_t          slot_width
    )
    {
    STATUS ret = ERROR;

    if (slots == 0)
        {
        return ERROR;
        }

    if (dai->driver->ops != NULL &&
        dai->driver->ops->xlate_tdm_slot_mask != NULL)
        {
        (void) dai->driver->ops->xlate_tdm_slot_mask (slots, &tx_mask,
                                                      &rx_mask);
        }
    else
        {
        if (tx_mask == 0 && rx_mask == 0)
            {
            if (slots == 0)
                {
                SND_SOC_DBG ("error: tx_mask, rx_mask and slots are all 0\n");
                return ERROR;
                }
            tx_mask = (1 << slots) - 1;
            rx_mask = (1 << slots) - 1;
            }
        }

    dai->tdmTxMask = tx_mask;
    dai->tdmRxMask = rx_mask;

    if (dai->driver->ops != NULL &&
        dai->driver->ops->set_tdm_slot != NULL)
        {
        ret = dai->driver->ops->set_tdm_slot (dai, tx_mask, rx_mask, slots,
                                              slot_width);
        }

    return ret;
    }

