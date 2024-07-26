/* pcm.c - Pulse Code Modulation device driver */

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

//#include <vxSndCore.h>
#include <cacheLib.h>
#include <iosLib.h>
#include <sysLib.h>
#include <scMemVal.h>
#include <subsys/timer/vxbTimerLib.h>
#include <private/iosLibP.h>
#include <pmapLib.h>
#include <vxSoundCore.h>

#include <pcm.h>

#define DEBUG_VX_SND_PCM

#ifdef DEBUG_VX_SND_PCM
#include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_PCM_DBG_OFF          0x00000000U
#define VX_SND_PCM_DBG_ERR          0x00000001U
#define VX_SND_PCM_DBG_INFO         0x00000002U
#define VX_SND_PCM_DBG_VERBOSE      0x00000004U
#define VX_SND_PCM_DBG_ALL          0xffffffffU
UINT32 vxSndPcmDbgMask = VX_SND_PCM_DBG_ERR;

#define VX_SND_PCM_MSG(mask, fmt, ...)                                  \
    do                                                                  \
        {                                                               \
        if ((vxSndPcmDbgMask & (mask)))                                 \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_PCM : [%s,%d] "fmt, __func__,    \
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define SND_PCM_MODE_INFO(...)                                          \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define SND_PCM_ERR(fmt, ...)    \
                  VX_SND_PCM_MSG(VX_SND_PCM_DBG_ERR,     fmt, ##__VA_ARGS__)
#define SND_PCM_INFO(fmt, ...)   \
                  VX_SND_PCM_MSG(VX_SND_PCM_DBG_INFO,    fmt, ##__VA_ARGS__)
#define SND_PCM_DBG(fmt, ...)    \
                  VX_SND_PCM_MSG(VX_SND_PCM_DBG_VERBOSE, fmt, ##__VA_ARGS__)
#else /* DEBUG_VX_SND_PCM */
#define SND_PCM_ERR(x...)      do { } while (FALSE)
#define SND_PCM_INFO(x...)     do { } while (FALSE)
#define SND_PCM_DBG(x...)      do { } while (FALSE)
#define SND_PCM_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_PCM */

/* defines */

#define SND_PCM_TIMEOUT                       5 /* 5 sec */
#define PCM_BUF_DIV_FACTOR                    4

#define IS_FRAME_ALIGNED(r,n)                 (((n) % (r)->byteAlign) == 0)
#define BYTES_TO_FRAMES(r,n)                  (((n)*8) / (r)->frameBits)
#define FRAMES_TO_BYTES(r,n)                  ((n) * (((r)->frameBits) / 8))
#define STREAM_DIRECTION(stream)                        \
        (stream)->direct == SNDRV_PCM_STREAM_PLAYBACK ? \
        "PLAYBACK" : "CAPTURE"
#define SUBSTREAM_RUNTIME_CHECK(substream, ret)                 \
        do                                                      \
            {                                                   \
            if (substream == NULL)                              \
                SND_PCM_ERR ("substream is NULL\n");            \
            else if (substream->runtime == NULL)                \
                SND_PCM_ERR ("substream->runtime is NULL\n");   \
            else                                                \
                break;                                          \
            return ret;                                         \
            } while(0)

#define SUBSTREAM_IS_RUNNING(substream)                                       \
        ((substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_RUNNING) || \
        (substream->runtime->status.state == SNDRV_PCM_SUBSTREAM_DRAINING &&  \
        substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK))

#define SND_INTERVAL_EMPTY(t) ((t)->min > (t)->max || ((t)->min == (t)->max   \
                               && ((t)->openMin == 1 || (t)->openMax == 1)))

/* typedefs */

/* forward declarations */

void   sndPcmInit (void);
STATUS sndPcmDevRegister (SND_DEVICE * sndDev);
STATUS sndPcmDevUnregister (SND_DEVICE * sndDev);
LOCAL void * sndPcmOpen (SND_PCM_STREAM * pData, const char * name,
                         int flag, int mode);
LOCAL int sndPcmClose (SND_PCM_STREAM * pData);
LOCAL ssize_t sndPcmTransfer (SND_PCM_STREAM * stream, uint8_t * pBuf,
                              size_t bytes);

#ifdef _WRS_CONFIG_RTP
LOCAL STATUS pcmScInit (void);
LOCAL STATUS pcmScIoctl (SND_PCM_STREAM * pData, int function,
                         void * pIoctlArg);
#endif

LOCAL STATUS sndPcmKernelIoctl (SND_PCM_STREAM * pData, int function,
                                void * pIoctlArg);
LOCAL STATUS sndPcmIoctl (SND_PCM_STREAM * pData, int function,
                          void * pIoctlArg, BOOL sysCall);

LOCAL void  sndPcmBufDataCopy (SND_PCM_SUBSTREAM * substream, uint8_t * pBuf,
                               SND_FRAMES_T appOff, SND_FRAMES_T bufFmOff,
                               SND_FRAMES_T xferCnt, BOOL isPlayback);

LOCAL uint32_t clearBitsFromLsb32 (uint32_t num);
LOCAL uint64_t clearBitsFromLsb64 (uint64_t num);

LOCAL BOOL intervalOverlapCalculate (VX_SND_INTERVAL * T1,
                                     VX_SND_INTERVAL * T2);

LOCAL void vxSndPcmSupportedHwParamsInit (SND_PCM_SUBSTREAM * substream);
LOCAL void vxSndPcmContructSupportHwParams (
                                        VX_SND_PCM_SUPPORT_HW_PARAMS * hwParams,
                                        HW_PARAM_INTERVAL_IDX idx,
                                        uint32_t min, uint32_t max);
LOCAL void vxSndPcmSupportedHwParamsGet (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmHwParamsRefine (SND_PCM_SUBSTREAM * substream,
                                     VX_SND_PCM_HW_PARAMS * userParams);
LOCAL STATUS vxSndPcmHwParams (SND_PCM_SUBSTREAM * substream,
                               VX_SND_PCM_HW_PARAMS * Params);
LOCAL STATUS vxSndPcmHwFree (SND_PCM_SUBSTREAM * substream);

LOCAL STATUS vxSndPcmStart (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmStop (SND_PCM_SUBSTREAM * substream,
                           SND_PCM_SUBSTREAM_STATE state);
LOCAL STATUS vxSndPcmPause (SND_PCM_SUBSTREAM * substream, BOOL isPause);
LOCAL STATUS vxSndPcmDrop (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmDrain (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmPrepare (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmReset (SND_PCM_SUBSTREAM * substream);

LOCAL STATUS vxSndPcmOpsReset (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmStopSync (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS vxSndPcmOpsIoctl (SND_PCM_SUBSTREAM * substream, int cmd,
                               void * arg);

LOCAL void   vxSndPcmXrun  (SND_PCM_SUBSTREAM * substream);

LOCAL STATUS sndPcmSubstreamOpen (SND_PCM_STREAM * stream);
LOCAL STATUS sndPcmSubstreamRelease (SND_PCM_STREAM * stream);
LOCAL SND_FRAMES_T sndPcmSubstreamXfer (SND_PCM_SUBSTREAM * substream,
                                        uint8_t * pBuf, size_t frames);

LOCAL SND_FRAMES_T sndPcmPlaybackBufWritableSizeGet (
                                                  VX_SND_PCM_RUNTIME * runtime);
LOCAL SND_FRAMES_T sndPcmPlaybackBufDataSizeGet (VX_SND_PCM_RUNTIME * runtime);
LOCAL SND_FRAMES_T sndPcmCaptureBufReadableSizeGet (
                                                  VX_SND_PCM_RUNTIME * runtime);
LOCAL SND_FRAMES_T validXferFramesGet (SND_PCM_SUBSTREAM * substream);
LOCAL STATUS       sndSubstreamAppPtrUpdate (SND_PCM_SUBSTREAM * substream,
                                             SND_FRAMES_T appPtr);
LOCAL STATUS       sndPcmDmaBufferPtrUpdate (SND_PCM_SUBSTREAM * substream,
                                             BOOL inIsr);

LOCAL STATUS sndPcmSubstreamStateUpdate (SND_PCM_SUBSTREAM * substream);

LOCAL uint64_t sndPcmWaitTime (VX_SND_PCM_RUNTIME * runtime, SND_FRAMES_T size);

LOCAL STATUS vxSndSubstreamCreate (VX_SND_PCM * pcm, STREAM_DIRECT direct);

/* locals */

LOCAL SEM_ID  sndPcmListSem;    /* protect sndPcmDeviceList access */
LOCAL DL_LIST sndPcmDeviceList;
LOCAL int     pcmDrvNum = -1;
LOCAL const uint32_t vxSndValidRates[] =
    {
    5512, 8000, 11025, 16000, 22050, 32000, 44100, 48000, 64000, 88200,
    96000, 176400, 192000, 352800, 384000
    };

/* pcm_formats */

LOCAL const VX_SND_PCM_FMT_INFO vxPcmFormats[VX_SND_FMT_MAX+1] =
    {
    [VX_SND_FMT_S8] =
    {
    .validWidth = 8, .phyWidth = 8, .littleEndian = -1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U8] =
    {
    .validWidth = 8, .phyWidth = 8, .littleEndian = -1, .signd = 0,
    .silence = { 0x80 },
    },
    [VX_SND_FMT_S16_LE] =
    {
    .validWidth = 16, .phyWidth = 16, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S16_BE] =
    {
    .validWidth = 16, .phyWidth = 16, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U16_LE] =
    {
    .validWidth = 16, .phyWidth = 16, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x80 },
    },
    [VX_SND_FMT_U16_BE] =
    {
    .validWidth = 16, .phyWidth = 16, .littleEndian = 0, .signd = 0,
    .silence = { 0x80, 0x00 },
    },
    [VX_SND_FMT_S24_LE] =
    {
    .validWidth = 24, .phyWidth = 32, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S24_BE] =
    {
    .validWidth = 24, .phyWidth = 32, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U24_LE] =
    {
    .validWidth = 24, .phyWidth = 32, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x80 },
    },
    [VX_SND_FMT_U24_BE] =
    {
    .validWidth = 24, .phyWidth = 32, .littleEndian = 0, .signd = 0,
    .silence = { 0x00, 0x80, 0x00, 0x00 },
    },
    [VX_SND_FMT_S32_LE] =
    {
    .validWidth = 32, .phyWidth = 32, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S32_BE] =
    {
    .validWidth = 32, .phyWidth = 32, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U32_LE] =
    {
    .validWidth = 32, .phyWidth = 32, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x00, 0x80 },
    },
    [VX_SND_FMT_U32_BE] =
    {
    .validWidth = 32, .phyWidth = 32, .littleEndian = 0, .signd = 0,
    .silence = { 0x80, 0x00, 0x00, 0x00 },
    },
    [VX_SND_FMT_S20_LE] =
    {
    .validWidth = 20, .phyWidth = 32, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S20_BE] =
    {
    .validWidth = 20, .phyWidth = 32, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U20_LE] =
    {
    .validWidth = 20, .phyWidth = 32, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x08, 0x00 },
    },
    [VX_SND_FMT_U20_BE] =
    {
    .validWidth = 20, .phyWidth = 32, .littleEndian = 0, .signd = 0,
    .silence = { 0x00, 0x08, 0x00, 0x00 },
    },
    [VX_SND_FMT_S24_3LE] =
    {
    .validWidth = 24, .phyWidth = 24, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S24_3BE] =
    {
    .validWidth = 24, .phyWidth = 24, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U24_3LE] =
    {
    .validWidth = 24, .phyWidth = 24, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x80 },
    },
    [VX_SND_FMT_U24_3BE] =
    {
    .validWidth = 24, .phyWidth = 24, .littleEndian = 0, .signd = 0,
    .silence = { 0x80, 0x00, 0x00 },
    },
    [VX_SND_FMT_S20_3LE] =
    {
    .validWidth = 20, .phyWidth = 24, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S20_3BE] =
    {
    .validWidth = 20, .phyWidth = 24, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U20_3LE] =
    {
    .validWidth = 20, .phyWidth = 24, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x08 },
    },
    [VX_SND_FMT_U20_3BE] =
    {
    .validWidth = 20, .phyWidth = 24, .littleEndian = 0, .signd = 0,
    .silence = { 0x08, 0x00, 0x00 },
    },
    [VX_SND_FMT_S18_3LE] =
    {
    .validWidth = 18, .phyWidth = 24, .littleEndian = 1, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_S18_3BE] =
    {
    .validWidth = 18, .phyWidth = 24, .littleEndian = 0, .signd = 1,
    .silence = {},
    },
    [VX_SND_FMT_U18_3LE] =
    {
    .validWidth = 18, .phyWidth = 24, .littleEndian = 1, .signd = 0,
    .silence = { 0x00, 0x00, 0x02 },
    },
    [VX_SND_FMT_U18_3BE] =
    {
    .validWidth = 18, .phyWidth = 24, .littleEndian = 0, .signd = 0,
    .silence = { 0x02, 0x00, 0x00 },
    },
    };

#ifdef _WRS_CONFIG_RTP
LOCAL const SC_IOCTL_TBL_ENTRY  ioctlPcmTbl[] =
    {
    {SC_IOCTL_ENABLE_READ,  sizeof (int)                      }, /* 0 */
    {SC_IOCTL_ENABLE_WRITE, sizeof (int)                      }, /* 1 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_PCM_HW_PARAMS)     }, /* 2 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_PCM_HW_PARAMS)     }, /* 3 */
    {SC_IOCTL_ENABLE,       0                                 }, /* 4 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_PCM_SW_PARAMS)     }, /* 5 */
    {SC_IOCTL_ENABLE_READ,  sizeof (VX_SND_SUBSTREAM_STATUS)  }, /* 6 */
    {SC_IOCTL_ENABLE_READ,  sizeof (SND_FRAMES_T)             }, /* 7 */
    {SC_IOCTL_ENABLE,       0                                 }, /* 8 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_SUBSTREAM_SYNC_PTR)}, /* 9 */
    {SC_IOCTL_ENABLE,       0                                 }, /* a */
    {SC_IOCTL_ENABLE,       0                                 }, /* b */
    {SC_IOCTL_ENABLE,       0                                 }, /* c */
    {SC_IOCTL_ENABLE,       0                                 }, /* d */
    {SC_IOCTL_ENABLE,       0                                 }, /* e */
    {SC_IOCTL_ENABLE_WRITE, sizeof (int)                      }, /* f */
    {SC_IOCTL_ENABLE_WRITE, sizeof (SND_FRAMES_T)             }, /* 10 */
    {SC_IOCTL_ENABLE,       0                                 }, /* 12 */
    {SC_IOCTL_ENABLE_WRITE, sizeof (SND_FRAMES_T)             }, /* 13 */
    };
#endif /* _WRS_CONFIG_RTP */

/*******************************************************************************
*
* sndPcmInit - initialize the PCM
*
* This routine installs the PCM device framework.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sndPcmInit (void)
    {
    sndPcmListSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (sndPcmListSem == SEM_ID_NULL)
        {
        SND_PCM_ERR ("failed to create sndPcmListSem\n");
        return;
        }

    (void) dllInit (&sndPcmDeviceList);

#ifdef _WRS_CONFIG_RTP
    if (pcmScInit () == ERROR)
        {
        SND_PCM_ERR ("pcmScInit() failed\n");
        goto error;
        }
#endif /* _WRS_CONFIG_RTP */

    /* install the driver */

    pcmDrvNum = iosDrvInstall (
                               NULL,                            /* creat()  */
                               NULL,                            /* remove() */
                               (DRV_OPEN_PTR)  sndPcmOpen,      /* open()   */
                               (DRV_CLOSE_PTR) sndPcmClose,     /* close()  */
                               (DRV_READ_PTR)  sndPcmTransfer,  /* read()   */
                               (DRV_WRITE_PTR) sndPcmTransfer,  /* write()  */
                               (DRV_IOCTL_PTR) sndPcmKernelIoctl); /* ioctl() */
    if (pcmDrvNum == -1)
        {
        SND_PCM_ERR ("failed to install PCM IOS driver\n");
        goto error;
        }

#ifdef _WRS_CONFIG_RTP
    /*
     * Register a different ioctl function pcmScIoctl() for system call
     * from RTP. It validates the address range and access permission.
     */

    if (iosDrvIoctlMemValSet (pcmDrvNum, (DRV_IOCTL_PTR) pcmScIoctl) == ERROR)
        {
        SND_PCM_ERR ("failed to set memory validation\n");
        goto error;
        }
#endif /* _WRS_CONFIG_RTP */

    return;

error:
    (void) semDelete (sndPcmListSem);
    }

/*******************************************************************************
*
* vxSndSubstreamCreate - create substream
*
* This routine creates a playback or capture substream and initializes it.
*
* RETURNS: OK, or ERROR if failed to create
*
* ERRNO: N/A
*
* \NOMANUAL
*/
//snd_pcm_new_stream(struct snd_pcm *pcm, int stream, int substream_count)
LOCAL STATUS vxSndSubstreamCreate
    (
    VX_SND_PCM *  pcm,
    STREAM_DIRECT direct
    )
    {
    SND_PCM_STREAM *    stream;

    if (direct >= SNDRV_PCM_STREAM_MAX)
        {
        return ERROR;
        }

    stream           = &pcm->stream[direct];
    stream->pcm      = pcm;
    stream->direct   = direct;
    stream->isOpened = FALSE;

    stream->substream = (SND_PCM_SUBSTREAM *)
                                   vxbMemAlloc (sizeof (SND_PCM_SUBSTREAM));
    if (stream->substream == NULL)
        {
        SND_PCM_ERR ("failed to alloc stream.substream\n");
        return ERROR;
        }

    stream->substream->pcm = pcm;
    stream->substream->stream = stream;
    SPIN_LOCK_ISR_INIT (&stream->substream->substreamSpinlockIsr, 0);

    stream->ioSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (stream->ioSem == SEM_ID_NULL)
        {
        SND_PCM_ERR ("failed to create stream->ioSem\n");
        goto errOut;
        }

    if (pcm->internal)
        {

        /* internal pcm substream uses pcm->name (dai link substream name) */

        (void) snprintf_s ((char *) stream->substream->name, SND_DEV_NAME_LEN,
                           pcm->name);
        }
    else
        {
        /*
         * register PCM stream device to IOS
         * '/dev/snd/pcmCxDyp' or '/dev/snd/pcmCxDyc':
         * x   - card number
         * y   - sound device number
         * p/c - playback/capture
         */

        (void) snprintf_s ((char *) stream->substream->name, SND_DEV_NAME_LEN,
                           "%s/pcmC%dD%d%c", SND_DEV_PREFIX,
                           pcm->card->cardNum, pcm->pcmDevNum,
                           direct == SNDRV_PCM_STREAM_PLAYBACK ? 'p' : 'c');
        }

    return OK;

errOut:
    vxbMemFree (stream->substream);

    return ERROR;
    }

/*******************************************************************************
*
* vxSndPcmCreate - create PCM
*
* This routine creates a PCM and initializes it. It also creates corresponding
* substreams and sound device for this PCM.
*
* RETURNS: OK, or ERROR if failed to create
*
* ERRNO: N/A
*
* \NOMANUAL
*/
// soc_create_pcm() = snd_pcm_new_internal () + _snd_pcm_new () mixer
STATUS vxSndPcmCreate
    (
    SND_CARD *    card,
    char *        name,
    int           devNum,
    BOOL          hasPlayback,
    BOOL          hasCapture,
    BOOL          internal,
    VX_SND_PCM ** ppPcm
    )
    {
    VX_SND_PCM * pcm;

    if (name == NULL)
        {
        return ERROR;
        }

    pcm = (VX_SND_PCM *) vxbMemAlloc (sizeof (VX_SND_PCM));
    if (pcm == NULL)
        {
        SND_PCM_ERR ("failed to alloc VX_SND_PCM\n");
        return ERROR;
        }

    pcm->card      = card;
    pcm->pcmDevNum = devNum;
    pcm->internal  = internal;

    (void) strncpy_s (pcm->name, SND_DEV_NAME_LEN, name, SND_DEV_NAME_LEN);

    if (hasPlayback &&
        vxSndSubstreamCreate (pcm, SNDRV_PCM_STREAM_PLAYBACK) != OK)
        {
        SND_PCM_ERR ("failed to create playback substream\n");
        goto errOut;
        }

    if (hasCapture &&
        vxSndSubstreamCreate (pcm, SNDRV_PCM_STREAM_CAPTURE) != OK)
        {
        SND_PCM_ERR ("failed to create capture substream\n");
        goto errOut;
        }

    if (internal)
        {

        /* internal PCM doesn't register to IOS */

        if (vxSndDevCreate (card, VX_SND_DEV_PCM, (void *) pcm,
                            (SND_DEV_REG_PTR) sndDummyDevRegister,
                            &pcm->sndDevice) != OK)
            {
            SND_PCM_ERR ("failed to create sound device for pcm %s\n", pcm->name);
            goto errOut;
            }
        }
    else
        {
        if (vxSndDevCreate (card, VX_SND_DEV_PCM, (void *) pcm,
                            (SND_DEV_REG_PTR) sndPcmDevRegister,
                            &pcm->sndDevice) != OK)
            {
            SND_PCM_ERR ("failed to create sound device for pcm %s\n", pcm->name);
            goto errOut;
            }
        }

    if (ppPcm != NULL)
        {
        *ppPcm = pcm;
        }

    return OK;

errOut:
    vxbMemFree (pcm);

    return ERROR;
    }

/*******************************************************************************
*
* sndPcmDevRegister - register a PCM IOS device
*
* This routine registers a PCM device to pcm list and IOS.
*
* RETURNS: OK, or ERROR if the device failed to register
*
* ERRNO: N/A
*
* \NOMANUAL
*/
//static int snd_pcm_dev_register(struct snd_device *device)
//called by  snd_device_register_all()
STATUS sndPcmDevRegister
    (
    SND_DEVICE * sndDev
    )
    {
    DL_NODE *    pNode;
    VX_SND_PCM * pcm;
    BOOL         inserted = FALSE;
    uint32_t     streamIdx;

    SND_PCM_DBG ("register sound device\n");

    if (sndDev == NULL || sndDev->data == NULL)
        {
        return ERROR;
        }
    pcm = (VX_SND_PCM *) sndDev->data;

    if (semTake (sndPcmListSem, (sysClkRateGet () * SND_PCM_TIMEOUT)) != OK)
        {
        return ERROR;
        }

    /* add PCM device to sndPcmDeviceList */

    pNode = DLL_FIRST (&sndPcmDeviceList);
    while (pNode != NULL)
        {
        VX_SND_PCM * temp = (VX_SND_PCM *) pNode;
        if (pcm->card == temp->card &&
            pcm->pcmDevNum == temp->pcmDevNum)
            {
            (void) semGive (sndPcmListSem);
            SND_PCM_ERR ("pcm %s already exists\n", pcm->name);
            return ERROR;
            }
        else
            {

            /*
             * the sequence of pcm on sndPcmDeviceList:
             * pcmC0D0<->pcmC0D1<->...<->pcmC0Dn<->
             * pcmC1D0<->...<->pcmC1Dn<->
             * ...
             * pcmCmD0<->...<->pcmCmDn
             */

            if ((pcm->card->cardNum < temp->card->cardNum) ||
                (pcm->card->cardNum == temp->card->cardNum &&
                 pcm->pcmDevNum < temp->pcmDevNum))
                {
                dllInsert (&sndPcmDeviceList, temp->node.previous, &pcm->node);
                inserted = TRUE;
                break;
                }
            }

        pNode = DLL_NEXT (pNode);
        }
    if (!inserted)
        {
        dllAdd (&sndPcmDeviceList, &pcm->node);
        }
    (void) semGive (sndPcmListSem);

    SND_PCM_DBG("pcm-%s is inserted to sndPcmDeviceList\n", pcm->name);

    /*
     * register PCM stream device to IOS
     * '/dev/snd/pcmCxDyp' or '/dev/snd/pcmCxDyc':
     * x   - card number
     * y   - sound device number
     * p/c - playback/capture
     */

    for (streamIdx = 0; streamIdx < SNDRV_PCM_STREAM_MAX; streamIdx++)
        {
        if (pcm->stream[streamIdx].substream != NULL)
            {
            if (sndDevRegister (&pcm->stream[streamIdx].devHdr,
                                pcm->stream[streamIdx].substream->name,
                                pcmDrvNum) != OK)
                {
                return ERROR;
                }
            SND_PCM_DBG("pcm-%s:substream-%s is registered to IOS\n",
            pcm->name, pcm->stream[streamIdx].substream->name);
            }
        }

    return OK;
    }

/*******************************************************************************
*
* sndPcmDevUnregister - unregister a PCM device
*
* This routine unregisters a PCM device.
*
* RETURNS: OK, or ERROR if failed to unregister
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sndPcmDevUnregister
    (
    SND_DEVICE * sndDev
    )
    {
    return OK;
    }

/*******************************************************************************
*
* sndPcmOpen - open the PCM device
*
* This routine opens the PCM device.
*
* RETURNS: PCM device structure when successful; otherwise ERROR
*
* ERRNO: N/A
*/

//static int snd_pcm_open(struct file *file, struct snd_pcm *pcm, int stream)
LOCAL void * sndPcmOpen
    (
    SND_PCM_STREAM * stream,
    const char *     name,
    int              flag,
    int              mode
    )
    {
    SND_PCM_DBG ("open device %s, substream: %s \n",
                 name, stream->substream->name);

    if (semTake (stream->ioSem, NO_WAIT) != OK)
        {
        return (void *) ERROR;
        }

    if (stream->isOpened)
        {
        (void) semGive (stream->ioSem);
        return (void *) ERROR;
        }

    if (sndPcmSubstreamOpen (stream) != OK)
        {
        SND_PCM_ERR ("sndPcmSubstreamOpen error\n");
        (void) semGive (stream->ioSem);
        return (void *) ERROR;
        }

    stream->isOpened = TRUE;
    stream->rtpId = MY_CTX_ID ();
    (void) semGive (stream->ioSem);
    return (void *) stream;
    }

/*******************************************************************************
*
* sndPcmClose - close PCM device
*
* This routine closes PCM device.
*
* RETURNS: OK, or ERROR if failed to close
*
* ERRNO: N/A
*/
//snd_pcm_release
LOCAL int sndPcmClose
    (
    SND_PCM_STREAM * stream
    )
    {
    SND_PCM_SUBSTREAM *  substream = stream->substream;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    SND_PCM_DBG ("close substream: %s\n",
                 stream->substream->name);
    if (semTake (stream->ioSem, WAIT_FOREVER) != OK)
        {
        return ERROR;
        }

    if (!stream->isOpened)
        {
        (void) semGive (stream->ioSem);
        return ERROR;
        }

    if (sndPcmSubstreamRelease (stream) != OK)
        {
        SND_PCM_ERR ("sndPcmSubstreamRelease error\n");
        (void) semGive (stream->ioSem);
        return ERROR;
        }

    stream->isOpened = FALSE;
    (void) semGive (stream->ioSem);

    return OK;
    }

/*******************************************************************************
*
* sndPcmTransfer - transfer PCM data
*
* This routine transfers PCM data from the PCM device.
*
* RETURNS: number of bytes transferred
*
* ERRNO: N/A
*/
//snd_pcm_read  snd_pcm_write
LOCAL ssize_t sndPcmTransfer
    (
    SND_PCM_STREAM * stream,
    uint8_t *        pBuf,        /* location to store read data */
    size_t           bytes       /* number of bytes to read */
    )
    {
    SND_PCM_SUBSTREAM *  substream = stream->substream;
    VX_SND_PCM_RUNTIME * runtime;
    uint64_t             count;

    SND_PCM_DBG ("%s %ld bytes\n", STREAM_DIRECTION (stream), bytes);

    SUBSTREAM_RUNTIME_CHECK (substream, -1);

    runtime = substream->runtime;

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_OPEN)
        {
        SND_PCM_ERR ("hardware parameters haven't been configured\n");
        return -1;
        }

    if (!IS_FRAME_ALIGNED (runtime, bytes))
        {
        SND_PCM_ERR ("read %ld bytes, not aligned to frame size %d\n",
                     bytes, runtime->byteAlign);
        return -1;
        }

    count = sndPcmSubstreamXfer (substream, pBuf,
                                 BYTES_TO_FRAMES (runtime, bytes));

    count = FRAMES_TO_BYTES (runtime, count);

    SND_PCM_DBG ("%s %lld bytes actually\n", STREAM_DIRECTION (stream), count);

    return count;
    }

#ifdef _WRS_CONFIG_RTP
/*******************************************************************************
*
* pcmScInit - PCM ioctl() command validation initialization
*
* This routine implements the initialization of ioctl() common validation for
* PCM driver.
*
* RETURNS: OK, or ERROR if the registration is already done or the supplied
* input data are invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS pcmScInit (void)
    {
    return scIoctlGroupRegister (VX_IOCG_AUDIO_PCM, ioctlPcmTbl,
                                 NELEMENTS (ioctlPcmTbl));
    }

/*******************************************************************************
*
* pcmScIoctl - handle ioctl call from system RTP
*
* This is a wrapper routine of sndPcmIoctl() to handle ioctl from RTP.
*
* RETURNS: OK, or ERROR if failed to call sndPcmIoctl()
*
* ERRNO: N/A
*/

LOCAL STATUS pcmScIoctl
    (
    SND_PCM_STREAM * stream,
    int              function,           /* function to perform */
    void *           pIoctlArg           /* function argument */
    )
    {
    return sndPcmIoctl (stream, function, pIoctlArg, TRUE);
    }
#endif

/*******************************************************************************
*
* sndPcmKernelIoctl - handle ioctl call not from system RTP
*
* This is a wrapper routine of sndPcmIoctl() to handle ioctl not from RTP.
*
* RETURNS: OK, or ERROR if failed to call sndPcmIoctl()
*
* ERRNO: N/A
*/

LOCAL STATUS sndPcmKernelIoctl
    (
    SND_PCM_STREAM * stream,
    int              function,           /* function to perform */
    void *           pIoctlArg           /* function argument */
    )
    {
    return sndPcmIoctl (stream, function, pIoctlArg, FALSE);
    }

/*******************************************************************************
*
* sndPcmIoctl - handle ioctls for PCM
*
* This routine handles ioctls to the PCM.
*
* RETURNS: OK when operation was successful; otherwise ERROR
*
* ERRNO: N/A
*/
//snd_pcm_ioctl
LOCAL STATUS sndPcmIoctl
    (
    SND_PCM_STREAM * stream,
    int              cmd,                /* cmd to perform   */
    void *           pIoctlArg,          /* cmd argument     */
    BOOL             sysCall             /* system call flag */
    )
    {
    SND_PCM_SUBSTREAM * substream = stream->substream;

    if (substream == NULL)
        {
        SND_PCM_ERR ("substream is NULL\n");
        return ERROR;
        }

    if (vxSndDevScMemValidate ((const void *) pIoctlArg, cmd, sysCall) != OK)
        {
        return ERROR;
        }

    if (cmd != FIOSHUTDOWN && cmd != FIOFSTATGET)
        {
        if (IOCPARM_LEN (cmd) != sizeof (0) && pIoctlArg == NULL)
            {
            SND_PCM_ERR ("cmd idx 0x%x should have a argument\n", cmd & 0xff);
            return ERROR;
            }
        }

    switch (cmd)
        {
        case VX_SND_CMD_PCM_CORE_VER:
            *((uint32_t *) pIoctlArg) = VX_SND_PCM_CORE_VERSION;
            return OK;
        case VX_SND_CMD_PCM_APP_VER:
            stream->appVersion = *((uint32_t *) pIoctlArg);
            return OK;
        case VX_SND_CMD_HW_REFINE:
            return vxSndPcmHwParamsRefine (substream,
                                           (VX_SND_PCM_HW_PARAMS *) pIoctlArg);
        case VX_SND_CMD_HW_PARAMS:
            return vxSndPcmHwParams (substream,
                                     (VX_SND_PCM_HW_PARAMS *) pIoctlArg);
        case VX_SND_CMD_HW_FREE:
            return vxSndPcmHwFree (substream);
        case VX_SND_CMD_SW_PARAMS:
            break;
        case VX_SND_CMD_STATUS:
            break;
        case VX_SND_CMD_DELAY:
            break;
        case VX_SND_CMD_HWSYNC:
            break;
        case VX_SND_CMD_SYNC_PTR64:
            break;
        case VX_SND_CMD_PREPARE:
            return vxSndPcmPrepare (substream);
        case VX_SND_CMD_RESET:
            return vxSndPcmReset (substream);
        case VX_SND_CMD_START:
            return vxSndPcmStart (substream);
        case VX_SND_CMD_DROP:
            return vxSndPcmDrop (substream);
        case VX_SND_CMD_DRAIN:
            return vxSndPcmDrain (substream);
        case VX_SND_CMD_PAUSE:
            return vxSndPcmPause (substream, *((BOOL *) pIoctlArg));
        case VX_SND_CMD_REWIND:
            break;
        case VX_SND_CMD_XRUN:
            vxSndPcmXrun (substream);
            return OK;
        case VX_SND_CMD_FORWARD:
            break;
        case FIOSHUTDOWN:
        case FIOFSTATGET:
            SND_PCM_DBG ("FIOSHUTDOWN or FIOFSTATGET\n");
            break;
        default:
            SND_PCM_ERR ("unknown PCM cmd 0x%x\n", cmd);
            return ERROR;
        }

    SND_PCM_DBG ("cmd 0x%x is not supported now\n", cmd);
    return OK;
    }

/*******************************************************************************
*
* sndPcmSubstreamOpen - open the substream
*
* This routine initializes the substream and calls its open().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

//int snd_pcm_open_substream(struct snd_pcm *pcm, int stream, struct file *file,
//                           struct snd_pcm_substream **rsubstream)
LOCAL STATUS sndPcmSubstreamOpen
    (
    SND_PCM_STREAM * stream
    )
    {
    SND_PCM_SUBSTREAM *  substream = stream->substream;
    VX_SND_PCM_RUNTIME * runtime;

    if (substream == NULL)
        {
        SND_PCM_ERR ("%s has no substream\n",
                     stream->direct == SNDRV_PCM_STREAM_PLAYBACK ?
                     "PLAYBACK" : "CAPTURE");
        return ERROR;
        }

    runtime = (VX_SND_PCM_RUNTIME *) vxbMemAlloc (sizeof (VX_SND_PCM_RUNTIME));
    if (runtime == NULL)
        {
        SND_PCM_ERR ("failed to alloc VX_SND_PCM_RUNTIME\n");
        return ERROR;
        }

    /*
     * If need to mmap status/control to userspace, status/control should be
     * pointer in constructure, and they should be allocated here.
     */

    runtime->status.state = SNDRV_PCM_SUBSTREAM_OPEN;
    substream->runtime = runtime;

    /* pcm->privateData may store VX_SND_SOC_PC_RUNTIME */

    substream->privateData = stream->pcm->privateData;

    vxSndPcmSupportedHwParamsInit (substream);

    SND_PCM_DBG ("open substream: %s", substream->name);

    if (substream->ops->open (substream) != OK)
        {
        SND_PCM_ERR ("substream %s open() failed\n", substream->name);
        }

    substream->isOpened = TRUE;

    vxSndPcmSupportedHwParamsGet (substream);

    return OK;
    }

/*******************************************************************************
*
* sndPcmSubstreamRelease - release the substream
*
* This routine releases the substream and calls its close().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//void snd_pcm_release_substream(struct snd_pcm_substream *substream)
LOCAL STATUS sndPcmSubstreamRelease
    (
    SND_PCM_STREAM * stream
    )
    {
    SND_PCM_SUBSTREAM *  substream = stream->substream;
    VX_SND_PCM_RUNTIME * runtime   = substream->runtime;

    if (substream == NULL)
        {
        SND_PCM_ERR ("%s has no substream\n", STREAM_DIRECTION (stream));
        return ERROR;
        }

    if (vxSndPcmDrop (substream) == ERROR)
        {
        return ERROR;
        }

    if (substream->isOpened)
        {
        if (runtime->status.state != SNDRV_PCM_SUBSTREAM_OPEN)
            {
            (void) vxSndPcmHwFree (substream);
            }
        if (substream->ops->close (substream) != OK)
            {
            SND_PCM_ERR ("substream %s close() failed\n", substream->name);
            }
        substream->isOpened = FALSE;
        }

    /* if staus/control are allocated, they should be freed first */

    vxbMemFree (substream->runtime);

    substream->runtime = NULL;

    SND_PCM_DBG ("release substream: %s", substream->name);

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmStart - start PCM substream
*
* This routine starts the PCM substream.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_pcm_start(struct snd_pcm_substream *substream)
LOCAL STATUS vxSndPcmStart
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;

    /* make sure substream is prepared */

    if (runtime->status.state != SNDRV_PCM_SUBSTREAM_PREPARED)
        {
        SND_PCM_ERR ("before starting, substream %s is not in "
                     "SNDRV_PCM_SUBSTREAM_PREPARED state\n", substream->name);
        return ERROR;
        }

    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        if (sndPcmPlaybackBufDataSizeGet (runtime) < runtime->startThreshold)
            {
            SND_PCM_ERR ("before starting, substream %s has less auido data "
                         "than startThreshold %ld\n",
                         substream->name, runtime->startThreshold);
            return ERROR;
            }
        }

    if (substream->ops->trigger (substream, SNDRV_PCM_TRIGGER_START) != OK)
        {
        SND_PCM_ERR ("substream(%s)->ops->trigger(SNDRV_PCM_TRIGGER_START) "
                     "error\n", substream->name);
        return ERROR;
        }

    /*
     * to do:
     * snd_pcm_playback_silence()
     * Insert silence data to specifed pointer. This will not overwrite the
     * existed playback audio data in DMA buffer.
     */

    runtime->status.state = SNDRV_PCM_SUBSTREAM_RUNNING;

    /* initialize timestamp value, to check DMA work state later */

    runtime->hwPtrUpdateTimestamp = sysTimestamp ();

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmStop - stop PCM substream
*
* This routine stops the running PCM substream and set the PCM to specified
* state. It cannot stop paused PCM substream and still keeps its paused state.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_pcm_stop(struct snd_pcm_substream *substream, snd_pcm_state_t state)
LOCAL STATUS vxSndPcmStop
    (
    SND_PCM_SUBSTREAM *     substream,
    SND_PCM_SUBSTREAM_STATE state
    )
    {
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;

     /* make sure substream is running */

     if (SUBSTREAM_IS_RUNNING (substream))
         {
         if (substream->ops->trigger (substream, SNDRV_PCM_TRIGGER_STOP) != OK)
             {
             SND_PCM_ERR ("substream(%s)->ops->trigger(SNDRV_PCM_TRIGGER_STOP) "
                          "error\n", substream->name);
             return ERROR;
             }
         runtime->stopSyncRequired = TRUE;
         }
     else
        {
         SND_PCM_DBG ("substream %s is not running\n", substream->name);
        }

     runtime->status.state = state;

     return OK;
    }

/*******************************************************************************
*
* vxSndPcmPause - pause or resume PCM substream
*
* This routine pauses or resumes the PCM substream.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_pcm_pause(struct snd_pcm_substream *substream, bool push)
LOCAL STATUS vxSndPcmPause
    (
    SND_PCM_SUBSTREAM * substream,
    BOOL                isPause
    )
    {
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;

    /* to do: check if substream supports PAUSE */

    if (isPause && runtime->status.state != SNDRV_PCM_SUBSTREAM_RUNNING)
        {
        SND_PCM_ERR ("substream %s is not running, cannot pause push\n",
                     substream->name);
        return ERROR;
        }
    else if (!isPause && runtime->status.state != SNDRV_PCM_SUBSTREAM_PAUSED)
        {
        SND_PCM_ERR ("substream %s is not pause pushed, cannot pause release\n",
                     substream->name);
        return ERROR;
        }

    /* update hwPtr when pause pushed */

    if (isPause)
        {
        (void) sndPcmDmaBufferPtrUpdate (substream, FALSE);
        }

    if (substream->ops->trigger (substream,
                                 isPause ?
                                 SNDRV_PCM_TRIGGER_PAUSE_PUSH :
                                 SNDRV_PCM_TRIGGER_PAUSE_RELEASE) != OK)
        {
        SND_PCM_ERR ("substream(%s)->ops->trigger(%s) error\n",
                     substream->name,
                     isPause ? "PAUSE_PUSH" : "PAUSE_RELEASE");
        return ERROR;
        }

     runtime->status.state = isPause ?
                             SNDRV_PCM_SUBSTREAM_PAUSED :
                             SNDRV_PCM_SUBSTREAM_RUNNING;

     return OK;
    }

/*******************************************************************************
*
* vxSndPcmStopSync - sync PCM substream after stop
*
* This routine syncs PCM substream after stop.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//void snd_pcm_sync_stop(struct snd_pcm_substream *substream, bool sync_irq)
LOCAL STATUS vxSndPcmStopSync
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    if (runtime->stopSyncRequired)
        {
        runtime->stopSyncRequired = FALSE;

        if (substream->ops != NULL && substream->ops->syncStop != NULL)
            {
            return substream->ops->syncStop (substream);
            }
        }

     return OK;
    }

/*******************************************************************************
*
* vxSndPcmOpsIoctl - PCM operation ioctl
*
* This routine implements the PCM operation ioctl().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_pcm_ops_ioctl(struct snd_pcm_substream *substream,
//                             unsigned cmd, void *arg)
LOCAL STATUS vxSndPcmOpsIoctl
    (
    SND_PCM_SUBSTREAM * substream,
    int                 cmd,
    void *              arg
    )
    {
    if (substream->ops->ioctl != NULL)
        {
        return substream->ops->ioctl (substream, cmd, arg);
        }
    else
        {
        return sndPcmDefaultIoctl (substream, cmd, arg);
        }
    }

/*******************************************************************************
*
* sndPcmDefaultIoctl - default PCM operation ioctl
*
* This routine implements the default PCM operation ioctl().
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_pcm_lib_ioctl(struct snd_pcm_substream *substream,
//                      unsigned int cmd, void *arg)
STATUS sndPcmDefaultIoctl
    (
    SND_PCM_SUBSTREAM * substream,
    int                 cmd,
    void *              arg
    )
    {
    VX_SND_PCM_HARDWARE * params = (VX_SND_PCM_HARDWARE *) arg;
    SND_FRAMES_T          frameSize;

    (void) params;
    (void) frameSize;

    switch (cmd)
        {
        case SNDRV_PCM_IOCTL1_RESET:
            if (SUBSTREAM_IS_RUNNING (substream))
                {
                sndPcmDmaBufferPtrUpdate (substream, 0);
                }
            else
                {
                SPIN_LOCK_ISR_TAKE (&substream->substreamSpinlockIsr);
                substream->runtime->status.hwPtr = 0;
                SPIN_LOCK_ISR_GIVE (&substream->substreamSpinlockIsr);
                }
            return OK;
        case SNDRV_PCM_IOCTL1_CHANNEL_INFO:
            SND_PCM_ERR ("cmd %d not support now\n", cmd);
            return ERROR;
        case SNDRV_PCM_IOCTL1_FIFO_SIZE:
            SND_PCM_ERR ("cmd %d not support now\n", cmd);
            return ERROR;
        }

    SND_PCM_ERR ("invalid cmd %d\n", cmd);
    return ERROR;
    }

/*******************************************************************************
*
* vxSndPcmOpsReset - PCM operation reset
*
* This routine resets PCM through its operation.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_pcm_do_reset(struct snd_pcm_substream *substream,
//                            snd_pcm_state_t state)
LOCAL STATUS vxSndPcmOpsReset
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (vxSndPcmOpsIoctl (substream, SNDRV_PCM_IOCTL1_RESET, NULL) != OK)
        {
        return ERROR;
        }

     return OK;
    }

/*******************************************************************************
*
* vxSndPcmReset - reset PCM substream
*
* This routine resets PCM substream.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_pcm_reset(struct snd_pcm_substream *substream)
LOCAL STATUS vxSndPcmReset
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    if (runtime->status.state != SNDRV_PCM_SUBSTREAM_RUNNING  &&
        runtime->status.state != SNDRV_PCM_SUBSTREAM_PREPARED &&
        runtime->status.state != SNDRV_PCM_SUBSTREAM_PAUSED)
        {
        SND_PCM_ERR ("state is not correct before reset\n");
        return ERROR;
        }

    if (vxSndPcmOpsReset (substream) != OK)
        {
        return ERROR;
        }

     /* reset DMA buffer as empty */

     runtime->control.appPtr = runtime->status.hwPtr;

    /* if support scilence playback, update pointer and fill scilence data */

     return OK;
    }

/*******************************************************************************
*
* vxSndPcmSupportedHwParamsInit - initialize supported hardware parameters
*
* This routine initializes supported hardware parameters as default values.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//static int snd_pcm_hw_constraints_init(struct snd_pcm_substream *substream)
LOCAL void vxSndPcmSupportedHwParamsInit
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME *           runtime  = substream->runtime;
    VX_SND_PCM_SUPPORT_HW_PARAMS * hwParams = &runtime->supportHwParams;
    uint32_t                       idx;
    VX_SND_INTERVAL *              pInterval;

    /* enable all access types */

    hwParams->accessTypeBits = 0xffffffff;

    /* enable all formats */

    hwParams->formatBits = ~0ull;

    /* enable all rates */

    hwParams->rates = SNDRV_PCM_ALL_RATES_MASK;

    /* set maximum interval, all defalut integer */

    for (idx = 0; idx < VX_SND_HW_PARAM_IDX_MAX; idx++)
        {
        pInterval          = &hwParams->intervals[idx];
        pInterval->empty   = 0;
        pInterval->min     = 0;
        pInterval->max     = 0xffffffff;
        pInterval->openMin = 0;
        pInterval->openMax = 0;
        pInterval->integer = 1; /* may not integer in future */
        }
    }

/*******************************************************************************
*
* intervalOverlapCalculate - calculate two interval overlap
*
* This routine calculates the overlap of two interval T1 and T2, and stores the
* result to T1.
*
* RETURNS: TRUE if T1 is changed, or FALSE if T1 is not changed
*
* ERRNO: N/A
*/
//int snd_interval_refine(struct snd_interval *i, const struct snd_interval *v)
LOCAL BOOL intervalOverlapCalculate
    (
    VX_SND_INTERVAL * T1,
    VX_SND_INTERVAL * T2
    )
    {
    BOOL changed = FALSE;

    /* calculate min */

    if (T1->min < T2->min)
        {
        T1->min     = T2->min;
        T1->openMin = T2->openMin;
        changed = TRUE;
        }
    else if (T1->min == T2->min && T1->openMin == 0 && T2->openMin == 1)
        {
        T1->openMin = 1;
        changed     = TRUE;
        }

    /* calculate max */

    if (T1->max > T2->max)
        {
        T1->max     = T2->max;
        T1->openMax = T2->openMax;
        changed = TRUE;
        }
    else if (T1->max == T2->max && T1->openMax == 0 && T2->openMax == 1)
        {
        T1->openMax = 1;
        changed     = TRUE;
        }

    /* calculate integer */

    if (T1->integer != 0 && T2->integer == 0)
        {
        T1->integer = 0;
        changed     = TRUE;
        }

    /* calculate openMin/openMax */

    if (T1->integer == 1)
        {
        if (T1->openMin == 1)
            {
            T1->min++;
            T1->openMin = 0;
            }
        if (T1->openMax == 1)
            {
            T1->openMax = 0;
            T1->max--;
            }
        }
    else if (T1->openMax == 0 && T1->openMin == 0 && T1->min == T1->max)
        {
        T1->integer = 1;
        }

    if (SND_INTERVAL_EMPTY (T1))
        {
        T1->empty = 1;
        }

    return changed;
    }

/*******************************************************************************
*
* vxSndPcmContructSupportHwParams - construct supported intervals
*
* This routine constructs specified supported intervals.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//int snd_pcm_hw_constraint_minmax
LOCAL void vxSndPcmContructSupportHwParams
    (
    VX_SND_PCM_SUPPORT_HW_PARAMS * hwParams,
    HW_PARAM_INTERVAL_IDX          idx,
    uint32_t                       min,
    uint32_t                       max
    )
    {
    VX_SND_INTERVAL tempInterval;

    tempInterval.min     = min;
    tempInterval.max     = max;
    tempInterval.openMin = 0;
    tempInterval.openMax = 0;
    tempInterval.empty   = 0;
    tempInterval.integer = 0;
    intervalOverlapCalculate (&hwParams->intervals[idx], &tempInterval);
    }

/*******************************************************************************
*
* vxSndPcmSupportedHwParamsGet - get supported hardware parameters
*
* This routine gets supported hardware parameters from cpu/platform/codec.
* It is called in substream open() after vxSndPcmContructSupportHwParams() and
* getting hardware parameters from hardware/driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
//static int snd_pcm_hw_constraints_complete(struct snd_pcm_substream *substream)
LOCAL void vxSndPcmSupportedHwParamsGet
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME *           runtime  = substream->runtime;
    VX_SND_PCM_SUPPORT_HW_PARAMS * hwParams = &runtime->supportHwParams;
    VX_SND_PCM_HARDWARE *          hw       = &runtime->hw;

    /* hw->info is not used yet, just comment out this line right now */
    //hwParams->accessTypeBits &= hw->info;
    hwParams->formatBits &= hw->formats;

    /* channels */

    vxSndPcmContructSupportHwParams (hwParams, VX_SND_HW_PARAM_CHANNELS,
                                     hw->channelsMin, hw->channelsMax);

    /* rate */

    hwParams->rates &= hw->rates;


    //vxSndPcmContructSupportHwParams (hwParams, VX_SND_HW_PARAM_RATE,
     //                                hw->rateMin, hw->rateMax);

    /* period bytes */

    vxSndPcmContructSupportHwParams (hwParams, VX_SND_HW_PARAM_PERIOD_BYTES,
                                     hw->periodBytesMin,
                                     hw->periodBytesMax);

    /* periods */

    vxSndPcmContructSupportHwParams (hwParams, VX_SND_HW_PARAM_PERIODS,
                                     hw->periodsMin, hw->periodsMax);

    /* buffer bytes */

    vxSndPcmContructSupportHwParams (hwParams, VX_SND_HW_PARAM_BUFFER_BYTES,
                                     hw->periodBytesMin, hw->bufferBytesMax);
    }

/*******************************************************************************
*
* vxSndPcmHwParamsRefine - refine hardware parameters
*
* This routine refines user defined hardware parameters.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//int snd_pcm_hw_refine(struct snd_pcm_substream *substream,
//                      struct snd_pcm_hw_params *params)
LOCAL STATUS vxSndPcmHwParamsRefine
    (
    SND_PCM_SUBSTREAM *    substream,
    VX_SND_PCM_HW_PARAMS * userParams
    )
    {
    VX_SND_PCM_RUNTIME *           runtime;
    VX_SND_PCM_SUPPORT_HW_PARAMS * hwParams;
    uint32_t                       idx;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime  = substream->runtime;
    hwParams = &runtime->supportHwParams;

    userParams->accessTypeBits &= hwParams->accessTypeBits;
    userParams->formatBits     &= hwParams->formatBits;
    userParams->rates          &= hwParams->rates;

    for (idx = 0; idx < VX_SND_HW_PARAM_IDX_MAX; idx++)
        {
        intervalOverlapCalculate (&userParams->intervals[idx],
                                  &hwParams->intervals[idx]);
        }

    return OK;
    }

/*******************************************************************************
*
* clearBitsFromLsb32 - clear all 1s except the LSB 1
*
* This routine clears all 1s except the LSB 1 of a 32bit value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

LOCAL uint32_t clearBitsFromLsb32
    (
    uint32_t num
    )
    {
    uint32_t mask = 1;
    uint32_t cnt = 0;

    while (((num & mask) == 0) && (cnt < 32))
        {
        mask <<= 1;
        cnt++;
        }

    if (cnt == 32)
        {
        return 0;
        }

    return mask;
    }

/*******************************************************************************
*
* clearBitsFromLsb64 - clear all 1s except the LSB 1
*
* This routine clears all 1s except the LSB 1 of a 64bit value.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

LOCAL uint64_t clearBitsFromLsb64
    (
    uint64_t num
    )
    {
    uint32_t low32 = (uint32_t) num;
    uint32_t high32 = (uint32_t) (num >> 32);
    uint32_t val;

    val = clearBitsFromLsb32 (low32);
    if (val != 0)
        {
        return  (uint64_t) val;
        }
    else
        {
        val = clearBitsFromLsb32 (high32);
        return ((uint64_t) val) << 32;
        }
    }

/*******************************************************************************
*
* vxSndPcmFmtValidWidthGet - get frame valid width
*
* This routine gets frame valid width.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

//int snd_pcm_format_width(snd_pcm_format_t format)
uint32_t vxSndPcmFmtValidWidthGet
    (
    uint32_t formatIdx
    )
    {
    if (formatIdx >= VX_SND_FMT_MAX)
        {
        SND_PCM_ERR ("unsupported format index %d\n", formatIdx);
        return 0;
        }
    return vxPcmFormats[formatIdx].validWidth;
    }

uint32_t vxSndGetRateFromBitmap
    (
    uint32_t rates
    )
    {
    return vxSndValidRates[ffs32Lsb(rates) - 1];
    }

/*******************************************************************************
*
* vxSndPcmFmtPhyWidthGet - get frame physical width
*
* This routine gets frame physical width.
*
* RETURNS: physical width, or 0 if failed
*
* ERRNO: N/A
*/
//int snd_pcm_format_physical_width(snd_pcm_format_t format)
uint32_t vxSndPcmFmtPhyWidthGet
    (
    uint32_t formatIdx
    )
    {
    if (formatIdx >= VX_SND_FMT_MAX)
        {
        SND_PCM_ERR ("unsupported format index %d\n", formatIdx);
        return 0;
        }
    return vxPcmFormats[formatIdx].phyWidth;
    }


#ifdef DEBUG_VX_SND_PCM

LOCAL void printHwParams
    (
    VX_SND_PCM_HW_PARAMS * params,
    VX_SND_PCM_RUNTIME *   runtime,
    const char *           info
    )
    {
    VX_SND_INTERVAL * pInv;
    char              openMinC;
    char              openMaxC;

    SND_PCM_MODE_INFO("\n%s:\n", info);

    if (runtime != NULL)
        {
        SND_PCM_MODE_INFO ("\taccess index is:\t%d\n", runtime->access);
        SND_PCM_MODE_INFO ("\tformat index is:\t0x%08x\n", runtime->format);
        SND_PCM_MODE_INFO ("\tinfo is:\t0x%08x\n", runtime->info);
        SND_PCM_MODE_INFO ("\trate is:\t%d\n", runtime->rate);
        SND_PCM_MODE_INFO ("\tchannels is:\t%d\n", runtime->channels);
        SND_PCM_MODE_INFO ("\tperiodBytes is:\t%d bytes\n",
                           runtime->periodBytes);
        SND_PCM_MODE_INFO ("\tperiod number is:\t%d\n", runtime->periods);
        SND_PCM_MODE_INFO ("\tbufferSize is:\t%d frames\n",
                           runtime->bufferSize);
        SND_PCM_MODE_INFO ("\tminAlign is:\t%d frame(s)\n", runtime->minAlign);
        SND_PCM_MODE_INFO ("\tbyteAlign is:\t%d bytes\n", runtime->byteAlign);
        SND_PCM_MODE_INFO ("\tframeBits is:\t%d bits\n", runtime->frameBits);
        SND_PCM_MODE_INFO ("\tsampleBits is:\t%d bits\n", runtime->sampleBits);
        SND_PCM_MODE_INFO ("\tstartThreshold is:\t%d frame(s)\n",
                           runtime->startThreshold);
        SND_PCM_MODE_INFO ("\tstopThreshold is:\t%d frames\n",
                           runtime->stopThreshold);
        return;
        }

    SND_PCM_MODE_INFO ("\taccessTypeBits is:\t0x%08x\n",
                       params->accessTypeBits);
    SND_PCM_MODE_INFO ("\tformatBits is:\t0x%016llx\n", params->formatBits);
    SND_PCM_MODE_INFO ("\tratesBits is:\t0x%08x\n", params->rates);
    SND_PCM_MODE_INFO ("\tinfo is:\t0x%08llx\n", params->info);

    /* print channels */

    pInv = &params->intervals[VX_SND_HW_PARAM_CHANNELS];
    openMinC = pInv->openMin == 1 ? '(' : '[';
    openMaxC = pInv->openMax == 1 ? ')' : ']';

    SND_PCM_MODE_INFO ("\tchannels is:\t %c%d, %d%c, integer:%s, empty:%s\n",
                       openMinC, pInv->min, pInv->max, openMaxC,
                       pInv->integer == 1 ? "True" : "False",
                       pInv->empty == 1 ? "True" : "False");

    /* print period bytes */

    pInv = &params->intervals[VX_SND_HW_PARAM_PERIOD_BYTES];
    openMinC = pInv->openMin == 1 ? '(' : '[';
    openMaxC = pInv->openMax == 1 ? ')' : ']';

    SND_PCM_MODE_INFO ("\tperiod bytes is:\t %c%d, %d%c, integer:%s, empty:%s\n",
                       openMinC, pInv->min, pInv->max, openMaxC,
                       pInv->integer == 1 ? "True" : "False",
                       pInv->empty == 1 ? "True" : "False");

    /* print period */

    pInv = &params->intervals[VX_SND_HW_PARAM_PERIODS];
    openMinC = pInv->openMin == 1 ? '(' : '[';
    openMaxC = pInv->openMax == 1 ? ')' : ']';

    SND_PCM_MODE_INFO ("\tperiod is:\t %c%d, %d%c, integer:%s, empty:%s\n",
                       openMinC, pInv->min, pInv->max, openMaxC,
                       pInv->integer == 1 ? "True" : "False",
                       pInv->empty == 1 ? "True" : "False");

    /* print buffer bytes */

    pInv = &params->intervals[VX_SND_HW_PARAM_BUFFER_BYTES];
    openMinC = pInv->openMin == 1 ? '(' : '[';
    openMaxC = pInv->openMax == 1 ? ')' : ']';

    SND_PCM_MODE_INFO ("\tbuffer bytes is:\t %c%d, %d%c, integer:%s, empty:%s\n",
                       openMinC, pInv->min, pInv->max, openMaxC,
                       pInv->integer == 1 ? "True" : "False",
                       pInv->empty == 1 ? "True" : "False");

    SND_PCM_MODE_INFO("\n");
    }

#endif
/*******************************************************************************
*
* vxSndPcmHwParams - set PCM substream hardware parameters
*
* This routine sets PCM substream hardware parameters and sets state as
* SNDRV_PCM_STATE_SETUP.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

//static int snd_pcm_hw_params(struct snd_pcm_substream *substream,
//                             struct snd_pcm_hw_params *params)

LOCAL STATUS vxSndPcmHwParams
    (
    SND_PCM_SUBSTREAM *    substream,
    VX_SND_PCM_HW_PARAMS * Params
    )
    {
    VX_SND_PCM_RUNTIME * runtime;
    uint32_t             idx;
    uint32_t             size;
    uint32_t             phyWidth;
    uint32_t             alignMin = 1;
    uint32_t             frameBytes;

#ifdef DEBUG_VX_SND_PCM
    printHwParams (Params, NULL, "Parameters From Application");
#endif

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    if (runtime->status.state != SNDRV_PCM_SUBSTREAM_OPEN &&
        runtime->status.state != SNDRV_PCM_SUBSTREAM_SETUP &&
        runtime->status.state != SNDRV_PCM_SUBSTREAM_PREPARED)
        {
        SND_PCM_ERR ("substream should be OPEN/SETUP/PREPARED when hwParams\n");
        return ERROR;
        }

    /* sync stop in case just stopped */

    (void) vxSndPcmStopSync (substream);

    /* 1. refine to get approximate parameters */

    if (vxSndPcmHwParamsRefine (substream, Params) != OK)
        {
        SND_PCM_ERR ("substream %s refine error\n", substream->name);
        return ERROR;
        }

    /* 2. get parameters from "Params" */

    Params->accessTypeBits = clearBitsFromLsb32 (Params->accessTypeBits);
    if (Params->accessTypeBits == 0)
        {
        SND_PCM_ERR ("substream %s choose accessTypeBits error\n",
                     substream->name);
        return ERROR;
        }
    Params->formatBits = clearBitsFromLsb64  (Params->formatBits);
    if (Params->formatBits == 0ull)
        {
        SND_PCM_ERR ("substream %s choose formatBits error\n",
                     substream->name);
        return ERROR;
        }
    Params->rates = clearBitsFromLsb32 (Params->rates);
    if (Params->rates == 0)
        {
        SND_PCM_ERR ("substream %s choose rates error\n",
                     substream->name);
        return ERROR;
        }

    for (idx = 0; idx < VX_SND_HW_PARAM_IDX_MAX; idx++)
        {
        VX_SND_INTERVAL * interval = &Params->intervals[idx];
        if ((interval->empty == 1)  || SND_INTERVAL_EMPTY (interval))
            {
            SND_PCM_ERR ("substream %s intervals[%d] should not be empty\n",
                         substream->name, idx);
            return ERROR;
            }

        /*
         * 1. Assume all the interval values are integer.
         * 2. Just use max/min are not right, should also consider their
         *    interaction.
         */

        if (idx == VX_SND_HW_PARAM_BUFFER_BYTES)
            {
            interval->min = interval->max;
            }
        else
            {
            interval->max = interval->min;
            }
        interval->integer = 1;
        }

    runtime->access      = PARAMS_ACCESS (Params);
    runtime->format      = PARAMS_FORMAT (Params);
    runtime->rate        = PARAMS_RATE (Params);
    runtime->channels    = PARAMS_CHANNELS (Params);
    runtime->periodBytes = PARAMS_PERIOD_BYTES (Params);

    phyWidth = vxSndPcmFmtPhyWidthGet (runtime->format);
    if (phyWidth == 0)
        {
        return ERROR;
        }
    runtime->sampleBits = phyWidth;
    phyWidth *= runtime->channels;
    runtime->frameBits = phyWidth;
    frameBytes = runtime->frameBits / 8;

    runtime->bufferSize  = PARAMS_BUFFER_BYTES (Params) / frameBytes;

    /*
     * 3. calculate to get valid parameters:
     *   - To satisfy DMA burst size transmission, periodBytes should be a power
     *     of 2
     *   - To void DMA tansmission error, one period should not across the buffer
     *     boundary, that is, the (bufferSize % peroidSize) should be 0.
     */

    while (1)
        {
        runtime->periodBytes = roundupPowerOfTwoPri (runtime->periodBytes);
        if ((runtime->periodBytes % frameBytes) != 0)
            {
            runtime->periodBytes += 1;
            continue;
            }

        if ((runtime->periodBytes * 2) <= runtime->bufferSize * frameBytes)
            {
            runtime->periods = (uint32_t) (runtime->bufferSize * frameBytes /
                                           runtime->periodBytes);
            runtime->periodSize = runtime->periodBytes / frameBytes;
            runtime->bufferSize = runtime->periods * runtime->periodSize;
            break;
            }
        else
            {
            SND_PCM_ERR ("cannot get a appropriate period bytes\n");
            return ERROR;
            }
        }

    /* 4. update selected parameters to intervals of "Params" */

    Params->intervals[VX_SND_HW_PARAM_CHANNELS].min = runtime->channels;
    Params->intervals[VX_SND_HW_PARAM_CHANNELS].max = runtime->channels;

    Params->intervals[VX_SND_HW_PARAM_PERIOD_BYTES].min = runtime->periodBytes;
    Params->intervals[VX_SND_HW_PARAM_PERIOD_BYTES].max = runtime->periodBytes;

    Params->intervals[VX_SND_HW_PARAM_PERIODS].min = runtime->periods;
    Params->intervals[VX_SND_HW_PARAM_PERIODS].max = runtime->periods;

    Params->intervals[VX_SND_HW_PARAM_BUFFER_BYTES].min =
                                  (uint32_t) (runtime->bufferSize * frameBytes);
    Params->intervals[VX_SND_HW_PARAM_BUFFER_BYTES].max =
                                  (uint32_t) (runtime->bufferSize * frameBytes);

    size = (uint32_t) (runtime->bufferSize * frameBytes);
    if (snd_pcm_lib_malloc_pages (substream, size) != OK)
        {
        SND_PCM_ERR ("failed to malloc DMA buffer %s\n", substream->name);
        return ERROR;
        }

    runtime->info = Params->info;

    while (phyWidth % 8 != 0)
        {
        phyWidth *= 2;
        alignMin *= 2;
        }
    runtime->byteAlign = phyWidth / 8;
    runtime->minAlign = alignMin;

    runtime->status.availMin = runtime->periodBytes / frameBytes;

    /* 1 frame: trigger playback; buffer full: stop transfer */

    runtime->startThreshold = 1;
    runtime->stopThreshold  = runtime->bufferSize - 1;

#ifdef DEBUG_VX_SND_PCM
    printHwParams (Params, NULL, "Parameters selected and set to hardware "
                   "via hwParams():");
#endif

    if (substream->ops->hwParams != NULL &&
        substream->ops->hwParams (substream, Params) != OK)
        {
        SND_PCM_ERR ("substream %s hwParams() error\n", substream->name);
        return ERROR;
        }

    /* clear DMA buffer */

    if (runtime->dmaArea != NULL && substream->ops->copy == NULL)
        {
        (void) memset (runtime->dmaArea, 0, runtime->dmaAreaBytes);
        }

    runtime->status.state = SNDRV_PCM_SUBSTREAM_SETUP;

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmHwFree - free PCM substream hardware resource
*
* This routine frees PCM substream hardware resource and sets state as
* SNDRV_PCM_STATE_OPEN.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_pcm_hw_free(struct snd_pcm_substream *substream)
LOCAL STATUS vxSndPcmHwFree
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    if (runtime->status.state != SNDRV_PCM_SUBSTREAM_PREPARED &&
        runtime->status.state != SNDRV_PCM_SUBSTREAM_SETUP)
        {
        SND_PCM_ERR ("substream should be SETUP/PREPARED when hwFree\n");
        return ERROR;
        }

    vxSndPcmStopSync (substream);

    if (substream->ops->hwFree != NULL)
        {
        if (substream->ops->hwFree (substream) != OK)
            {
            SND_PCM_ERR ("substream %s hwFree() error\n", substream->name);
            return ERROR;
            }
        }

    /* if runtime->dmaBufPtr supports allocated by core, release it here */

    runtime->status.state = SNDRV_PCM_SUBSTREAM_OPEN;

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmPrepare - prepare to trigger
*
* This routine prepares to trigger and sets state as
* SNDRV_PCM_SUBSTREAM_PREPARED.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

//static int snd_pcm_prepare(struct snd_pcm_substream *substream,
//                           struct file *file)
LOCAL STATUS vxSndPcmPrepare
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_OPEN ||
        runtime->status.state == SNDRV_PCM_SUBSTREAM_DISCONNECTED ||
        SUBSTREAM_IS_RUNNING (substream))
        {
        SND_PCM_ERR ("substream state is not right before PREPARE\n");
        return ERROR;
        }

    /* resume paused substream first, and then stop it to setup */

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_PAUSED)
        {
        if (vxSndPcmPause (substream, FALSE) != OK)
            {
            return ERROR;
            }

        if (vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP) != OK)
            {
            SND_PCM_ERR ("failed to stop substream %s\n", substream->name);
            return ERROR;
            }
        }

    (void) vxSndPcmStopSync (substream);

    if (substream->ops->prepare (substream) != OK)
        {
        SND_PCM_ERR ("substream(%s)->ops->prepare() failed\n", substream->name);
        return ERROR;
        }

    if (vxSndPcmOpsReset (substream) != OK)
        {
        return ERROR;
        }

    /* reset DMA buffer as empty */

    runtime->control.appPtr = runtime->status.hwPtr;

    runtime->status.state = SNDRV_PCM_SUBSTREAM_PREPARED;

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmDrop - drop PCM substream
*
* This routine stops the running or paused PCM substream and sets it as
* SNDRV_PCM_STATE_SETUP state.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_pcm_drop
LOCAL STATUS vxSndPcmDrop
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;
    STATUS               status;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;

    /* resume paused substream */

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_PAUSED)
        {
        if (vxSndPcmPause (substream, FALSE) != OK)
            {
            return ERROR;
            }
        }

    status = vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP);
    if (status != OK)
        {
        SND_PCM_ERR ("failed to stop substream %s\n", substream->name);
        }

    return status;
    }

/*******************************************************************************
*
* vxSndPcmDrain - drain PCM substream
*
* This routine drains all the audio data in PCM substream DMA buffer.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_pcm_drain
LOCAL STATUS vxSndPcmDrain
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME *    runtime;
    SND_PCM_SUBSTREAM_STATE state;
    STREAM_DIRECT           direct;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime = substream->runtime;
    state = runtime->status.state;
    direct = substream->stream->direct;

    if (state == SNDRV_PCM_SUBSTREAM_OPEN ||
        state == SNDRV_PCM_SUBSTREAM_DISCONNECTED ||
        state == SNDRV_PCM_SUBSTREAM_SUSPENDED)
        {
        SND_PCM_ERR ("substream %s state (%d) error before draining\n",
                     substream->name, state);
        return ERROR;
        }

    /* resume paused substream */

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_PAUSED)
        {
        if (vxSndPcmPause (substream, FALSE) != OK)
            {
            return ERROR;
            }
        }

    if (direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        switch (state)
            {
            case SNDRV_PCM_SUBSTREAM_PREPARED:

                /*
                 * Check if some audio data have been written to buffer
                 * before running playback.
                 */

                if (sndPcmPlaybackBufDataSizeGet (runtime) > 0)
                    {
                    if (vxSndPcmStart (substream) != OK)
                        {

                        /* cannot start, drop the existed audio data */

                        runtime->status.state = SNDRV_PCM_SUBSTREAM_SETUP;
                        }
                    runtime->status.state = SNDRV_PCM_SUBSTREAM_DRAINING;
                    }

                runtime->status.state = SNDRV_PCM_SUBSTREAM_SETUP;
                break;
            case SNDRV_PCM_SUBSTREAM_RUNNING:
                runtime->status.state = SNDRV_PCM_SUBSTREAM_DRAINING;
                break;
            case SNDRV_PCM_SUBSTREAM_XRUN:
                runtime->status.state = SNDRV_PCM_SUBSTREAM_SETUP;
                break;
            default:
                break;
            }
        }
    else
        {
        if (state == SNDRV_PCM_SUBSTREAM_RUNNING)
            {
            STATUS status;

            if (sndPcmCaptureBufReadableSizeGet (runtime) > 0)
                {
                status = vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_DRAINING);
                }
            else
                {
                status = vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP);
                }
            if (status != OK)
                {
                SND_PCM_ERR ("failed to stop substream %s, "
                             "force to setup state\n", substream->name);
                runtime->status.state = SNDRV_PCM_SUBSTREAM_SETUP;
                }
            }
        }

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_DRAINING)
        {
        if (substream->ops->trigger (substream, SNDRV_PCM_TRIGGER_DRAIN) != OK)
            {
            SND_PCM_ERR ("substream(%s)->ops->trigger"
                         "(SNDRV_PCM_TRIGGER_DRAIN) error\n", substream->name);
            return ERROR;
            }
        }
    else
        {

        /* not drain state, just return */

        return OK;
        }

    /*
     * Wait some calculated time to finish playback draining. Drain state will
     * be changed when update hwPtr.
     * to do:
     * should determined by O_NONBLOCK flag.
     * Wait until drain completes now.
     * Seems only playback needs to wait.
     */

    if (direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        SND_FRAMES_T  leftFrames = sndPcmPlaybackBufDataSizeGet (runtime);
        unsigned long waitUs;
        unsigned long waitMs;
        unsigned long waitTicks;
        unsigned long tickPeriodMs = 1000 / sysClkRateGet();

        if (leftFrames == 0)
            {
            return OK;
            }

        waitUs = leftFrames * 1000000 / runtime->rate;
        waitMs = waitUs / 1000;
        waitUs %= 1000;

        SND_PCM_DBG ("substream %s, %ld frames left, drain waiting time is "
                     "%ld.%03ldms, is equal to %ld ticks + %ld.%03ldms\n",
                     substream->name, leftFrames, waitMs, waitUs,
                     waitMs / tickPeriodMs, waitMs % tickPeriodMs, waitUs);
        waitTicks = waitMs / tickPeriodMs;
        waitMs %= tickPeriodMs;

        if (waitTicks != 0)
            {
            taskDelay ((unsigned int) waitTicks);
            }

        if (waitMs != 0)
            {
            vxbMsDelay ((int) waitMs);
            }

        if (waitUs != 0)
            {
            vxbUsDelay ((int) waitUs);
            }

        if (vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP) != OK)
            {
            SND_PCM_ERR ("substream: %s failed to stop\n",
                         substream->name);
            vxSndPcmXrun (substream);
            return ERROR;
            }
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndPcmXrun - handle overrun and underrun issues
*
* This routine handles overrun and underrun issues.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void vxSndPcmXrun
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    SND_PCM_DBG ("substream %s XRUN!\n", substream->name);
    if (SUBSTREAM_IS_RUNNING (substream))
        {
        (void) vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_XRUN);
        }
    }

/*******************************************************************************
*
* sndPcmSubstreamXfer - transfer sound data
*
* This routine reads or writes audio stream data to or from PCM substream.
*
* RETURNS: byte numbers transferred, or 0 if failed
*
* ERRNO: N/A
*/
//snd_pcm_lib_write
LOCAL SND_FRAMES_T sndPcmSubstreamXfer
    (
    SND_PCM_SUBSTREAM * substream,
    uint8_t *           pBuf,        /* location to store read data */
    size_t              xferframes   /* number of frames to transfer */
    )
    {
    VX_SND_PCM_RUNTIME * runtime;
    BOOL                 isPlayback;
    SND_FRAMES_T         validFrames;
    SND_FRAMES_T         bufFrameOffset = 0;
    SND_FRAMES_T         xferSum        = 0;

    SUBSTREAM_RUNTIME_CHECK (substream, -1);

    if ((pBuf == NULL) || (xferframes == 0))
        {
        SND_PCM_ERR ("no valid audio data to transfer\n");
        return 0;
        }

    isPlayback = substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK;

    runtime = substream->runtime;

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_OPEN)
        {
        SND_PCM_ERR ("substream: %s has not been configured\n",
                     substream->name);
        return 0;
        }

    if ((runtime->dmaArea == NULL) || (runtime->dmaAreaBytes == 0))
        {
        SND_PCM_ERR ("substream: %s's DMA buffer has not been initialized\n",
                     substream->name);
        return 0;
        }

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_RUNNING)
        {
        sndPcmDmaBufferPtrUpdate (substream, FALSE);
        }

    if ((runtime->status.state == SNDRV_PCM_SUBSTREAM_PREPARED) &&
        !isPlayback && xferframes >= runtime->startThreshold)
        {
        if (vxSndPcmStart (substream) != OK)
            {
            SND_PCM_ERR ("substream: %s failed to start capturing\n",
                         substream->name);
            goto xferOut;
            }
        }

    validFrames = validXferFramesGet (substream);

    /* start to transfer audio data */

    while (xferframes > 0)
        {
        SND_FRAMES_T xferCnt;
        SND_FRAMES_T appOffset;
        SND_FRAMES_T appPtrToBufEnd;
        SND_FRAMES_T appPtrTemp;

        if (validFrames == 0)
            {
            uint64_t t;

            /* capture has drained all the audio data, stop it */

            if (!isPlayback &&
                runtime->status.state == SNDRV_PCM_SUBSTREAM_DRAINING)
                {
                if (vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP) != OK)
                    {
                    SND_PCM_ERR ("substream: %s failed to stop\n",
                                 substream->name);
                    vxSndPcmXrun (substream);
                    }
                goto xferOut;
                }

            t = sndPcmWaitTime (runtime, xferframes);
            vxbUsDelay ((int) t);
#if 0
            if (t < (1000000 / sysClkRateGet() / PCM_BUF_DIV_FACTOR))
                {
                /*
                 * Delay 1 tick to wait transfer.
                 * 196k, 32bit, 2-channel, tick=60, the buffer is 26.134KB at least.
                 * 48k, 32bit, 8-channel, tick=60, the buffer is 25.6KB at least.
                 */

                taskDelay (1);
                }
            else
                {
                vxbUsDelay ((int) t);
                }
#endif
            sndPcmDmaBufferPtrUpdate (substream, FALSE);
            validFrames = validXferFramesGet (substream);
            SND_PCM_DBG ("xferframes=%ld, delay %ld us, validFrames=%ld\n",
                          xferframes, t, validFrames);
            if (validFrames == 0)
                {
                SND_PCM_ERR ("DMA doesn't work now\n");
                vxSndPcmXrun (substream);
                goto xferOut;
                }
            }

        xferCnt = xferframes > validFrames ? validFrames : xferframes;
        appOffset = runtime->control.appPtr;
        appPtrToBufEnd = runtime->bufferSize - appOffset;

        /* read/write cannot cross the buffer boundary */

        if (xferCnt > appPtrToBufEnd)
            {
            xferCnt = appPtrToBufEnd;
            }

        /* copy audio data to/from DMA buffer */

        sndPcmBufDataCopy (substream, pBuf, appOffset, bufFrameOffset, xferCnt,
                           isPlayback);

        /* update all pointers */

        bufFrameOffset += xferCnt;  /* pBuf offset */
        xferframes     -= xferCnt;  /* left frames to transfer */
        xferSum        += xferCnt;  /* all frames transferred */
        validFrames    -= xferCnt;  /* left valid frames in buf */

        appPtrTemp = runtime->control.appPtr + xferCnt;
        if (appPtrTemp >= runtime->bufferSize)
            {

            /*
             * previous writeable xferCnt calculation makes sure appPtr will
             * not cross hwPtr
             */

            appPtrTemp -= runtime->bufferSize;
            }

        if (sndSubstreamAppPtrUpdate (substream, appPtrTemp) != OK)
            {
            vxSndPcmXrun (substream);
            goto xferOut;
            }

        /* trigger playback start */

        if (isPlayback &&
            runtime->status.state == SNDRV_PCM_SUBSTREAM_PREPARED &&
            sndPcmPlaybackBufDataSizeGet (runtime) >= runtime->startThreshold)
            {
            if (vxSndPcmStart (substream) != OK)
                {
                vxSndPcmXrun (substream);
                goto xferOut;
                }
            }
        }

    return xferSum;

xferOut:
    SND_PCM_ERR ("error: transfered 0 byte\n");
    return 0;
    }

/*******************************************************************************
*
* sndPcmBufDataCopy - copy audio data to buffer
*
* This routine copies audio data between application buffer and DMA buffer.
* The copying direction is determined by PCM stream direction.
* It only supports interleaved audio data copying.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sndPcmBufDataCopy
    (
    SND_PCM_SUBSTREAM * substream,
    uint8_t *           pBuf,
    SND_FRAMES_T        appOff,
    SND_FRAMES_T        bufFmOff,
    SND_FRAMES_T        xferCnt,
    BOOL                isPlayback
    )
    {
    VX_SND_PCM_RUNTIME * runtime     = substream->runtime;
    size_t               appOffByte  = FRAMES_TO_BYTES(runtime, appOff);
    size_t               bufOffByte  = FRAMES_TO_BYTES(runtime, bufFmOff);
    size_t               xferCntByte = FRAMES_TO_BYTES(runtime, xferCnt);

    if (substream->ops->copy != NULL)
        {

        /* use the copy() defined by machine */

        substream->ops->copy (substream, 0, appOffByte, pBuf + bufOffByte,
                              xferCntByte);
        }
    else
        {
        if (isPlayback)
            {
            (void) memcpy_s (runtime->dmaArea + appOffByte,
                             runtime->dmaAreaBytes - appOffByte,
                             pBuf + bufOffByte, xferCntByte);
            }
        else
            {
            (void) memcpy_s (pBuf + bufOffByte,
                             runtime->dmaAreaBytes - appOffByte,
                             runtime->dmaArea + appOffByte, xferCntByte);
            }
        }
    }

/*******************************************************************************
*
* sndPcmPlaybackBufWritableSizeGet - get writeable space of playback buffer
*
* This routine gets the playback buffer size (frames) that can be written to.
* It is used when hwPtr needs to be changed, such as before reading buffer,
* before writing buffer, reseting PCM substream, forwarding/rewinding hwPtr
* pausing/resuming audio and in playback/capture DMA completed ISR.
*
* RETURNS: frame numbers can be written to buffer
*
* ERRNO: N/A
*/
//snd_pcm_update_hw_ptr(struct snd_pcm_substream *substream)
//snd_pcm_update_hw_ptr0

LOCAL STATUS sndPcmDmaBufferPtrUpdate
    (
    SND_PCM_SUBSTREAM * substream,
    BOOL                inIsr
    )
    {
    VX_SND_PCM_RUNTIME * runtime  = substream->runtime;
    SND_FRAMES_T         oldHwPtr;
    SND_FRAMES_T         newHwPtr;
#if 0
    SND_FRAMES_T         deltaFrames;
    signed long          deltaTimestamp;
    unsigned long        deltaUs;
    unsigned long        deltaMs;
    unsigned long        calFrames;
#endif
    unsigned long        currentTimestamp;

    SPIN_LOCK_ISR_TAKE (&substream->substreamSpinlockIsr);

    currentTimestamp = sysTimestamp ();
    oldHwPtr         = runtime->status.hwPtr;
    newHwPtr         = substream->ops->pointer (substream);

    if (newHwPtr == SNDRV_PCM_PTR_XRUN)
        {
        SPIN_LOCK_ISR_GIVE (&substream->substreamSpinlockIsr);
        SND_PCM_ERR ("newHwPtr is SNDRV_PCM_PTR_XRUN\n");
        vxSndPcmXrun (substream);
        return ERROR;
        }

    if (newHwPtr > runtime->bufferSize)
        {
        SPIN_LOCK_ISR_GIVE (&substream->substreamSpinlockIsr);
        SND_PCM_ERR ("newHwPtr (%ld) is greater than buffersize (%ld)\n",
                     newHwPtr, runtime->bufferSize);
        vxSndPcmXrun (substream);
        return ERROR;
        }

#if 0 // have not debugged now. not be mandatory
    /* calculate time and delta frames to check the DMA transfer error */

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_RUNNING ||
        runtime->status.state == SNDRV_PCM_SUBSTREAM_DRAINING)
        {
        deltaFrames = newHwPtr - oldHwPtr;
        if (deltaFrames < 0)
            {
            deltaFrames += runtime->bufferSize;
            }

        deltaTimestamp = runtime->hwPtrUpdateTimestamp - currentTimestamp;
        if (deltaTimestamp < 0)
            {
            deltaTimestamp += sysTimestampPeriod();
            }

        deltaMs  = deltaTimestamp * 1000 / sysTimestampFreq();
        deltaUs  = deltaTimestamp * 1000000 / sysTimestampFreq();
        deltaUs %= 1000;

        if (deltaMs > 0)
            {
            calFrames  = deltaMs * runtime->rate / 1000;
            calFrames += deltaUs * runtime->rate / 1000000;
            }
        else if (deltaUs > 0)
            {
            calFrames = deltaUs * runtime->rate / 1000000;
            }
        else
            {
            calFrames = 0;
            }

        if ((deltaFrames < (calFrames / 2)) || (deltaFrames > (calFrames * 3 / 2)))
            {

            /* To avoid misjudgement, just print trace, not run XRUN now. */

            SND_PCM_ERR ("%ld.%03ld ms tansfer %ld frames. "
                         "It should transfer %ld frames\n",
                         deltaMs, deltaUs, deltaFrames, calFrames);
            }
        SND_PCM_DBG ("%ld.%03ld ms tansfer %ld frames. It is expected.\n",
                     deltaMs, deltaUs, deltaFrames);
        }
#endif
    /* update latest pointer */

    runtime->status.hwPtr         = newHwPtr;
    runtime->hwPtrUpdateTimestamp = currentTimestamp;

    SPIN_LOCK_ISR_GIVE (&substream->substreamSpinlockIsr);

    /* update PCM substream state */

    return sndPcmSubstreamStateUpdate (substream);
    }

/*******************************************************************************
*
* sndPcmSubstreamStateUpdate - update PCM substream
*
* This routine updates the PCM substream state. It is usually used after
* buffer pointer updating.
*
* RETURNS: OK, or ERROR if failed to update
*
* ERRNO: N/A
*/
//int snd_pcm_update_state(struct snd_pcm_substream *substream,
//                         struct snd_pcm_runtime *runtime)
LOCAL STATUS sndPcmSubstreamStateUpdate
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
#if 0  // not use now, may need to handl in future. do not delete it.
    VX_SND_PCM_RUNTIME * runtime    = substream->runtime;
    SND_FRAMES_T         validSpace = validXferFramesGet (substream);

    /* handle draining */

    if (runtime->status.state == SNDRV_PCM_SUBSTREAM_DRAINING)
        {
        if (((validSpace >= runtime->bufferSize - 1) &&
             substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK) ||
            (validSpace == 0 &&
             substream->stream->direct == SNDRV_PCM_STREAM_CAPTURE))
            {
            if (vxSndPcmStop (substream, SNDRV_PCM_SUBSTREAM_SETUP) != OK)
                {
                SND_PCM_ERR ("substream: %s failed to stop\n",
                             substream->name);
                vxSndPcmXrun (substream);
                return ERROR;
                }
            return OK;
            }
        }
    else
        {
        /*
         * if (validSpace >= runtime->stopThreshold)
         * set runtime->stopThreshold to be runtime->bufferSize - 1 (full size),
         * or not a full size?
         * playback: underrun happens
         * capture: overrun happens
         */
        if (validSpace >= runtime->bufferSize - 1)
            {
            SND_PCM_ERR ("substream: %s DMA buffer %s\n", substream->name,
                         substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK
                         ? "underrun" : "overrun");
            vxSndPcmXrun (substream);
            return ERROR;
            }
        }
#endif
    return OK;
    }

/*******************************************************************************
*
* sndPcmPlaybackBufWritableSizeGet - get writeable space of playback buffer
*
* This routine gets the playback buffer size (frames) that can be written to.
*
*   <playback buffer>
*  -----------------------------------------
*  |         |  valid data     |           |
*  -----------------------------------------
*            ^                 ^           ^
*            |                 |           |
*          hwPtr           appPtr   bufferSize
*
* When hwPtr is eaual to appPtr, there is no audio data in playback buffer.
* When (appPtr+1) % bufferSize == hwPtr, playback buffer is full, and it has
* buffersize - 1 frames audio data.
*
* RETURNS: frame numbers can be written to buffer
*
* ERRNO: N/A
*/

LOCAL SND_FRAMES_T sndPcmPlaybackBufWritableSizeGet
    (
    VX_SND_PCM_RUNTIME * runtime
    )
    {
    SND_FRAMES_T free = runtime->bufferSize -
                        (runtime->control.appPtr - runtime->status.hwPtr);
    if (free > runtime->bufferSize)
        {
        free -= runtime->bufferSize;
        }

    return free - 1;
    }

/*******************************************************************************
*
* sndPcmPlaybackBufDataSizeGet - get audio data size in playback buffer
*
* This routine gets the audio data size (frames) in playback buffer.
*
* RETURNS: audio data frame numbers
*
* ERRNO: N/A
*/

LOCAL SND_FRAMES_T sndPcmPlaybackBufDataSizeGet
    (
    VX_SND_PCM_RUNTIME * runtime
    )
    {
    return runtime->bufferSize - sndPcmPlaybackBufWritableSizeGet (runtime) - 1;
    }

/*******************************************************************************
*
* sndPcmCaptureBufReadableSizeGet - get readable space of capture buffer
*
* This routine gets the capture audio data size (frames) that can be read.
*
*  <capture buffer>
* -----------------------------------------
* |         |  valid data     |           |
* -----------------------------------------
*           ^                 ^           ^
*           |                 |           |
*         appPtr            hwPtr   bufferSize
*
* When hwPtr is eaual to appPtr, there is no audio data in playback buffer.
* When (hwPtr+1) % bufferSize == appPtr, capture buffer is full, and it has
* buffersize - 1 frames audio data.
*
* RETURNS: frame numbers can be read from buffer
*
* ERRNO: N/A
*/

LOCAL SND_FRAMES_T sndPcmCaptureBufReadableSizeGet
    (
    VX_SND_PCM_RUNTIME * runtime
    )
    {
    SND_FRAMES_T dataFrames =  runtime->status.hwPtr -
                               runtime->control.appPtr;
    if (dataFrames < 0)
        {
        dataFrames += runtime->bufferSize;
        }

    return dataFrames;
    }

/*******************************************************************************
*
* sndPcmAppPtrUpdate - update sustream application pointer
*
* This routine updates the application pointer of the substream.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/

LOCAL STATUS sndSubstreamAppPtrUpdate
    (
    SND_PCM_SUBSTREAM * substream,
    SND_FRAMES_T        appPtr
    )
    {
    VX_SND_PCM_RUNTIME * runtime = substream->runtime;

    if (appPtr == runtime->control.appPtr)
        {
        return OK;
        }
    runtime->control.appPtr = appPtr;

    if (substream->ops->ack)
        {
        return substream->ops->ack (substream);
        }

    return OK;
    }

/*******************************************************************************
*
* sndPcmWaitFrames - get the time to wait
*
* This routine gets the time (us) to wait before next transfer.
* PCM_BUF_DIV_FACTOR affects the wait time.
*
* RETURNS: microsecond number, or 0 if error
*
* ERRNO: N/A
*/

LOCAL uint64_t sndPcmWaitTime
    (
    VX_SND_PCM_RUNTIME * runtime,
    SND_FRAMES_T         size       /* the size need to transfer */
    )
    {
    uint64_t waitSize;
    uint64_t waitUs;

    /*
     * Wait 1/4 period time. By some test, if wait time is shorter, DSP may not
     * update DMA pointer in time. (Just for Samsugn ABOX)
     */

    waitSize = runtime->periodSize  / PCM_BUF_DIV_FACTOR * 1000 /
               (runtime->rate / 1000);

    if (waitSize < size)
        {
        waitSize = size;
        }

    if (runtime->rate == 0)
        {
        SND_PCM_ERR ("invalid runtime parameters\n");
        return 0;
        }

    waitUs = waitSize * 1000000 / runtime->rate;

    return waitUs != 0 ? waitUs : 1;
    }

/*******************************************************************************
*
* sndPcmWaitFrames - get the valid frame number
*
* This routine gets valid frame number in buffer to read from or write to.
*
* RETURNS: frame number
*
* ERRNO: N/A
*/

LOCAL SND_FRAMES_T validXferFramesGet
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (substream->stream->direct == SNDRV_PCM_STREAM_PLAYBACK)
        {
        return sndPcmPlaybackBufWritableSizeGet (substream->runtime);
        }
    else
        {
        return sndPcmCaptureBufReadableSizeGet (substream->runtime);
        }
    }

/*******************************************************************************
*
* snd_pcm_period_elapsed - update hwPtr in playback/capture ISR
*
* This routine updates the hwPtr in playback/capture ISR.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void snd_pcm_period_elapsed
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    if (!SUBSTREAM_IS_RUNNING(substream))
        {
        SND_PCM_ERR ("substream: %s is not running\n", substream->name);
        return;
        }

    if (sndPcmDmaBufferPtrUpdate (substream, TRUE) != OK)
        {
        SND_PCM_ERR ("substream: %s update hwPtr failed.\n", substream->name);
        }
    }

/*******************************************************************************
*
* snd_pcm_lib_malloc_pages - allocate substream DMA buffer
*
* This routine allocates substream DMA buffer.
*
* RETURNS: OK, or ERROR if failed to allocate
*
* ERRNO: N/A
*/

STATUS snd_pcm_lib_malloc_pages
    (
    SND_PCM_SUBSTREAM * substream,
    uint32_t            size
    )
    {
    VX_SND_PCM_RUNTIME *       runtime;
    VX_SND_SUBSTREAM_DMA_BUF * dmaBuf;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime =  substream->runtime;

    if (runtime->dmaBufPtr != NULL)
        {

        /*
         * If current used DMA buffer satisfies the size, use it directly,
         * or else, free current DMA buffer.
         */

        if (runtime->dmaBufPtr->bytes >= size)
            {
            runtime->dmaAreaBytes = size;
            return OK;
            }
        else
            {
            snd_pcm_lib_free_pages (substream);
            }
        }

    if (substream->dmaBuf.area != NULL && substream->dmaBuf.bytes >= size)
        {
        dmaBuf = &substream->dmaBuf;
        }
    else
        {
        SND_PCM_ERR ("Vx Sound substream->runtime doesn't support allocate "
                     "DMA buffer now. Please allocate buffer in cpu/platform"
                     " driver\n");
        return ERROR;
        }

    /* update playback/capture DMA info in runtime */

    runtime->dmaArea      = dmaBuf->area;
    runtime->dmaPhyAddr   = dmaBuf->phyAddr;
    runtime->dmaAreaBytes = size;
    runtime->dmaBufPtr    = dmaBuf;

    return OK;
    }

/*******************************************************************************
*
* snd_pcm_lib_free_pages - free allocated DMA buffer
*
* This routine frees allocated DMA buffer used in playback/capture.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

STATUS snd_pcm_lib_free_pages
    (
    SND_PCM_SUBSTREAM * substream
    )
    {
    VX_SND_PCM_RUNTIME * runtime;

    SUBSTREAM_RUNTIME_CHECK (substream, ERROR);

    runtime =  substream->runtime;

    if (runtime->dmaArea == NULL)
        {
        SND_PCM_ERR ("runtime->dmaArea has already been NULL\n");
        return OK;
        }

    if (runtime->dmaBufPtr != NULL && runtime->dmaBufPtr != &substream->dmaBuf)
        {

        /* runtime->dmaBufPtr is allocated, not used substream->dmaBuf */

        SND_PCM_ERR ("Vx Sound substream->runtime doesn't support "
                     "allocate/free DMA buffer now.\n");
        return ERROR;
        }

    /* update playback/capture DMA info in runtime */

    runtime->dmaArea      = NULL;
    runtime->dmaPhyAddr   = 0;
    runtime->dmaAreaBytes = 0;
    runtime->dmaBufPtr    = NULL;

    return OK;
    }

STATUS vxSndPcmHwRateToBitmap
    (
    VX_SND_PCM_HARDWARE * hw
    )
    {
    uint32_t idx;
    uint32_t lsbBit;
    uint32_t msbBit;

    if ((hw->rates & SNDRV_PCM_ALL_RATES_MASK) != 0)
        {
        hw->rates &= SNDRV_PCM_ALL_RATES_MASK;
        return OK;
        }

    hw->rates = 0;

    if (hw->rateMin == 0 || hw->rateMin > hw->rateMax)
        {
        SND_PCM_ERR ("hw->rateMin and hw->rateMax are invalid\n");
        return ERROR;
        }


    /* get rateMin */

    for (idx = 0; idx < NELEMENTS (vxSndValidRates); idx++)
        {
        if (hw->rateMin == vxSndValidRates[idx])
            {
            lsbBit = idx;
            }
        if (hw->rateMax == vxSndValidRates[idx])
            {
            msbBit = idx;
            }
        }

    if (lsbBit > msbBit)
        {
        SND_PCM_ERR ("hw->rateMin or hw->rateMax are invalid\n");
        return ERROR;
        }

    hw->rates = ((1 << (msbBit - lsbBit + 1)) - 1) << lsbBit;

    return OK;
    }

