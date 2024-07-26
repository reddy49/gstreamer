/* vxSoundCore.h - VxWorks Sound Core Header File */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file defines the camera functions and structures for the camera module
*/

#ifndef __INCvxSoundCoreh
#define __INCvxSoundCoreh

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <semLib.h>
#include <stdio.h>
#include <sysLib.h>
#ifdef _WRS_KERNEL
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#endif
#include <msgQLib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <string.h>
#include <sys/ioctl.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus  */

/* defines */

#define SND_DEV_PREFIX                    "/dev/snd"
#define SND_DEV_NAME_LEN                  (64)
#define VX_SND_MODULE_VERSION(a, b, c)    (((a) << 16) | ((a) << 8) | (c))

#define SNDRV_PCM_ACCESS_MMAP           0x00000001  /* hardware supports mmap */
#define SNDRV_PCM_ACCESS_MMAP_VALID     0x00000002  /* period data are valid during transfer */
#define SNDRV_PCM_ACCESS_INTERLEAVED    0x00000004  /* channels are interleaved */
#define SNDRV_PCM_ACCESS_NONINTERLEAVED 0x00000008  /* channels are not interleaved */

/* typedefs */

struct vxSndPcmStreamOps;
struct vxSndPcm;
struct vxSndPcmSubStream;
struct vxSndPcmStream;
struct vxSndPcmRuntime;
struct vxSndPcmMmapStaus;
struct vxSndPcmMmapControl;
struct vxSndDevice;

typedef signed long SND_FRAMES_T;
typedef signed long SND_FRAMES_S_T;

typedef unsigned long SND_FRAMES_U_T;

#define SNDRV_PCM_PTR_XRUN  ((SND_FRAMES_T)-1)

typedef enum vxSndDevType
    {
    VX_SND_DEV_UNKNOWN = 0,
    VX_SND_DEV_CODEC,
    VX_SND_DEV_PCM,
    VX_SND_DEV_HWDEP,
    VX_SND_DEV_JACK,
    VX_SND_DEV_CONTROL,
    VX_SND_DEV_MAX,
    } VX_SND_DEV_TYPE;

typedef enum vxSndFormatIndex
    {
    VX_SND_FMT_S8 = 0,    /* in one byte */
    VX_SND_FMT_U8,
    VX_SND_FMT_S16_LE,    /* in two bytes */
    VX_SND_FMT_S16_BE,
    VX_SND_FMT_U16_LE,
    VX_SND_FMT_U16_BE,
    VX_SND_FMT_S24_LE,    /* in four bytes */
    VX_SND_FMT_S24_BE,
    VX_SND_FMT_U24_LE,
    VX_SND_FMT_U24_BE,
    VX_SND_FMT_S32_LE,
    VX_SND_FMT_S32_BE,
    VX_SND_FMT_U32_LE,
    VX_SND_FMT_U32_BE,

    VX_SND_FMT_S20_LE,    /* in four bytes, LSB justified */
    VX_SND_FMT_S20_BE,    /* in four bytes, LSB justified */
    VX_SND_FMT_U20_LE,    /* in four bytes, LSB justified */
    VX_SND_FMT_U20_BE,    /* in four bytes, LSB justified */

    VX_SND_FMT_S24_3LE,   /* in three bytes */
    VX_SND_FMT_S24_3BE,   /* in three bytes */
    VX_SND_FMT_U24_3LE,   /* in three bytes */
    VX_SND_FMT_U24_3BE,   /* in three bytes */
    VX_SND_FMT_S20_3LE,   /* in three bytes */
    VX_SND_FMT_S20_3BE,   /* in three bytes */
    VX_SND_FMT_U20_3LE,   /* in three bytes */
    VX_SND_FMT_U20_3BE,   /* in three bytes */
    VX_SND_FMT_S18_3LE,   /* in three bytes */
    VX_SND_FMT_S18_3BE,   /* in three bytes */
    VX_SND_FMT_U18_3LE,   /* in three bytes */
    VX_SND_FMT_U18_3BE,   /* in three bytes */
    VX_SND_FMT_MAX,
    } VX_SND_FORMAT_IDX;

#define VX_SND_FMTBIT_S8        (1ull << VX_SND_FMT_S8)
#define VX_SND_FMTBIT_U8        (1ull << VX_SND_FMT_U8)
#define VX_SND_FMTBIT_S16_LE    (1ull << VX_SND_FMT_S16_LE)
#define VX_SND_FMTBIT_S16_BE    (1ull << VX_SND_FMT_S16_BE)
#define VX_SND_FMTBIT_U16_LE    (1ull << VX_SND_FMT_U16_LE)
#define VX_SND_FMTBIT_U16_BE    (1ull << VX_SND_FMT_U16_BE)
#define VX_SND_FMTBIT_S24_LE    (1ull << VX_SND_FMT_S24_LE)
#define VX_SND_FMTBIT_S24_BE    (1ull << VX_SND_FMT_S24_BE)
#define VX_SND_FMTBIT_U24_LE    (1ull << VX_SND_FMT_U24_LE)
#define VX_SND_FMTBIT_U24_BE    (1ull << VX_SND_FMT_U24_BE)
#define VX_SND_FMTBIT_S32_LE    (1ull << VX_SND_FMT_S32_LE)
#define VX_SND_FMTBIT_S32_BE    (1ull << VX_SND_FMT_S32_BE)
#define VX_SND_FMTBIT_U32_LE    (1ull << VX_SND_FMT_U32_LE)
#define VX_SND_FMTBIT_U32_BE    (1ull << VX_SND_FMT_U32_BE)

#define VX_SND_FMTBIT_S20_LE    (1ull << VX_SND_FMT_S20_LE)
#define VX_SND_FMTBIT_S20_BE    (1ull << VX_SND_FMT_S20_BE)
#define VX_SND_FMTBIT_U20_LE    (1ull << VX_SND_FMT_U20_LE)
#define VX_SND_FMTBIT_U20_BE    (1ull << VX_SND_FMT_U20_BE)

#define VX_SND_FMTBIT_S24_3LE   (1ull << VX_SND_FMT_S24_3LE)
#define VX_SND_FMTBIT_S24_3BE   (1ull << VX_SND_FMT_S24_3BE)
#define VX_SND_FMTBIT_U24_3LE   (1ull << VX_SND_FMT_U24_3LE)
#define VX_SND_FMTBIT_U24_3BE   (1ull << VX_SND_FMT_U24_3BE)
#define VX_SND_FMTBIT_S20_3LE   (1ull << VX_SND_FMT_S20_3LE)
#define VX_SND_FMTBIT_S20_3BE   (1ull << VX_SND_FMT_S20_3BE)
#define VX_SND_FMTBIT_U20_3LE   (1ull << VX_SND_FMT_U20_3LE)
#define VX_SND_FMTBIT_U20_3BE   (1ull << VX_SND_FMT_U20_3BE)
#define VX_SND_FMTBIT_S18_3LE   (1ull << VX_SND_FMT_S18_3LE)
#define VX_SND_FMTBIT_S18_3BE   (1ull << VX_SND_FMT_S18_3BE)
#define VX_SND_FMTBIT_U18_3LE   (1ull << VX_SND_FMT_U18_3LE)
#define VX_SND_FMTBIT_U18_3BE   (1ull << VX_SND_FMT_U18_3BE)

typedef enum hwParamIntervalIdx
    {
    VX_SND_HW_PARAM_CHANNELS,        /* Channels */
    //VX_SND_HW_PARAM_RATE,            /* Approx rate */
    VX_SND_HW_PARAM_PERIOD_BYTES,    /* Approx bytes between interrupts */
    VX_SND_HW_PARAM_PERIODS,         /* Approx interrupts per buffer */
   //VX_SND_HW_PARAM_BUFFER_FRAMES,   /* Size of buffer in frames */
    VX_SND_HW_PARAM_BUFFER_BYTES,    /* Size of buffer in bytes */
    VX_SND_HW_PARAM_IDX_MAX,
    } HW_PARAM_INTERVAL_IDX;

typedef enum streamDirect
    {
    SNDRV_PCM_STREAM_PLAYBACK = 0,
    SNDRV_PCM_STREAM_CAPTURE,
    SNDRV_PCM_STREAM_MAX,
    } STREAM_DIRECT;

typedef enum sndPcmSubstreamState
    {
    SNDRV_PCM_SUBSTREAM_OPEN = 0,
    SNDRV_PCM_SUBSTREAM_SETUP,
    SNDRV_PCM_SUBSTREAM_PREPARED,
    SNDRV_PCM_SUBSTREAM_RUNNING,
    SNDRV_PCM_SUBSTREAM_XRUN,
    SNDRV_PCM_SUBSTREAM_DRAINING,
    SNDRV_PCM_SUBSTREAM_PAUSED,
    SNDRV_PCM_SUBSTREAM_SUSPENDED,
    SNDRV_PCM_SUBSTREAM_DISCONNECTED,
    SNDRV_PCM_SUBSTREAM_LAST,
    } SND_PCM_SUBSTREAM_STATE;

typedef enum sndPcmSubstreamIoctlCmd
    {
    SNDRV_PCM_IOCTL1_RESET= 0,
    SNDRV_PCM_IOCTL1_RESERVED_1,
    SNDRV_PCM_IOCTL1_CHANNEL_INFO,
    SNDRV_PCM_IOCTL1_RESERVED_2,
    SNDRV_PCM_IOCTL1_FIFO_SIZE,
    } SND_PCM_SUBSTREAM_IOCTL_CMD;

typedef enum sndPcmTriggerState
    {
    SNDRV_PCM_TRIGGER_STOP = 0,
    SNDRV_PCM_TRIGGER_START,
    SNDRV_PCM_TRIGGER_PAUSE_PUSH,
    SNDRV_PCM_TRIGGER_PAUSE_RELEASE,
    //SNDRV_PCM_TRIGGER_SUSPEND,
    //SNDRV_PCM_TRIGGER_RESUME,
    SNDRV_PCM_TRIGGER_DRAIN,
    } SND_PCM_TRIGGER_STATE;

typedef struct vxSoundCard
    {
    DL_LIST  deviceList;
    VXB_DEV_ID pDev;
    int      cardNum;
    char     name[SND_DEV_NAME_LEN];
    DL_LIST  controlList;
    int      controlCnt;
    SEM_ID   controlListSem;
    SEM_ID   cardMutex;
    DL_LIST  filesList;     /* all files associated to this card */

    BOOL     instantiated;
    } SND_CARD;

typedef STATUS (*SND_DEV_REG_PTR) (struct vxSndDevice * sndDev);

typedef struct vxSndDevice
    {
    DL_NODE         node; /* node should be the first member */
    SND_CARD *      card;
    void *          data;
    VX_SND_DEV_TYPE type;
    SND_DEV_REG_PTR regFunc;
    } SND_DEVICE;

typedef struct vxSndPcmStream
    {
    DEV_HDR                    devHdr; /* devHdr should be the first member */
    STREAM_DIRECT              direct;
    struct vxSndPcm *          pcm;
    struct vxSndPcmSubStream * substream;
    SEM_ID                     ioSem;
    BOOL                       isOpened;
    uint32_t                   appVersion;
    RTP_ID                     rtpId;
    //snd_kcontrol
    } SND_PCM_STREAM;

typedef struct vxSndPcm
    {
    DL_NODE              node; /* node should be the first member */
    char                 name[SND_DEV_NAME_LEN];
    SND_CARD *           card;
    SND_DEVICE *         sndDevice;
    int                  pcmDevNum;
    BOOL                 internal;

    /* stream[0] -playback, stream[1] - capture */

    struct vxSndPcmStream stream[2];
    void *               privateData;
    } VX_SND_PCM;

typedef struct vxSndCtrlDev
    {
    DEV_HDR              devHdr; /* devHdr should be the first member */
    char                 name[SND_DEV_NAME_LEN];
    SEM_ID               ioSem;
    BOOL                 isOpened;
    SND_CARD *           card;
    uint32_t             appVersion;
    RTP_ID               rtpId;
    SND_DEVICE *         sndDevice;
    } SND_CONTROL_DEV;

typedef struct vxSndSubstreamDmaBuf
    {
    unsigned char * area;
    PHYS_ADDR       phyAddr;
    unsigned long   bytes;
    void *          private_data;
    } VX_SND_SUBSTREAM_DMA_BUF;

typedef struct vxSndPcmSubStream
    {
    DL_NODE                     node; /* node should be the first member */
    char                        name[SND_DEV_NAME_LEN];
    VX_SND_PCM *                pcm;
    SND_PCM_STREAM *            stream;
    struct vxSndPcmRuntime *    runtime;
    struct vxSndPcmStreamOps *  ops;

    VX_SND_SUBSTREAM_DMA_BUF    dmaBuf;

    BOOL                        isOpened;
    void *                      privateData;
    spinlockIsr_t               substreamSpinlockIsr;
    } SND_PCM_SUBSTREAM;

typedef struct vxSndInterval
    {
    uint32_t min;
    uint32_t max;
    uint32_t openMin:1,
             openMax:1,
             integer:1,
             empty:1;
    } VX_SND_INTERVAL;

typedef struct vxSndPcmHwParams
    {
    uint32_t        accessTypeBits;
    uint32_t        rates;
    uint64_t        formatBits;
    VX_SND_INTERVAL intervals[VX_SND_HW_PARAM_IDX_MAX];
    uint32_t        info;

    } VX_SND_PCM_HW_PARAMS;

typedef struct vxSndPcmSupportHwParams
    {
    uint32_t        accessTypeBits;
    uint32_t        rates;
    uint64_t        formatBits;
    VX_SND_INTERVAL intervals[VX_SND_HW_PARAM_IDX_MAX];
    } VX_SND_PCM_SUPPORT_HW_PARAMS;


typedef struct vxSndPcmSwParams
    {
    uint32_t period_step;

    SND_FRAMES_T avail_min;      /* min avail frames for wakeup */
    SND_FRAMES_T xfer_align;     /* obsolete: xfer size need to be a multiple */
    SND_FRAMES_T start_threshold;/* min hw_avail frames for automatic start */
    SND_FRAMES_T stop_threshold; /* min avail frames for automatic stop */
    } VX_SND_PCM_SW_PARAMS;

typedef struct vxSndSubstreamStatus
    {
    //to do
    } VX_SND_SUBSTREAM_STATUS;

typedef struct vxSndPcmStreamOps
    {
    STATUS (*open) (SND_PCM_SUBSTREAM * substream);
    STATUS (*close) (SND_PCM_SUBSTREAM * substream);
    STATUS (*ioctl) (SND_PCM_SUBSTREAM * substream, uint32_t cmd, void * arg);
    STATUS (*hwParams) (SND_PCM_SUBSTREAM *substream, VX_SND_PCM_HW_PARAMS * params);
    STATUS (*hwFree) (SND_PCM_SUBSTREAM * substream);
    STATUS (*prepare) (SND_PCM_SUBSTREAM * substream);
    STATUS (*trigger) (SND_PCM_SUBSTREAM * substream, int cmd);
    STATUS (*syncStop) (SND_PCM_SUBSTREAM * substream);
    unsigned long (*pointer) (SND_PCM_SUBSTREAM * substream);
    STATUS (*copy) (SND_PCM_SUBSTREAM * substream, int channel,
                    size_t pos, void * buf, size_t bytes);
    struct page *(*page) (SND_PCM_SUBSTREAM * substream, unsigned long offset);
    STATUS (*ack) (SND_PCM_SUBSTREAM * substream);
    } VX_SND_PCM_STREAM_OPS;

typedef struct vxSndPcmMmapStaus
    {
    SND_PCM_SUBSTREAM_STATE state;
    SND_FRAMES_T            hwPtr;
    SND_FRAMES_T            availMin;
    } VX_SND_PCM_MMAP_STATUS;

typedef struct vxSndPcmMmapControl
    {
    SND_FRAMES_T appPtr;
    } VX_SND_PCM_MMAP_CTRL;

typedef struct vxSndSubstreamSyncPtr
    {
    uint32_t flags;
    uint32_t pad1;
    union
        {
        VX_SND_PCM_MMAP_STATUS status;
        } s;
    union
        {
        VX_SND_PCM_MMAP_CTRL control;
        } c;
} VX_SND_SUBSTREAM_SYNC_PTR;

typedef struct vxSndPcmHardware
    {
    uint32_t info;              /* SNDRV_PCM_INFO_* */
    uint64_t formats;           /* SNDRV_PCM_FMTBIT_* */
    uint32_t rates;             /* SNDRV_PCM_RATE_* */
    uint32_t rateMin;           /* min rate */
    uint32_t rateMax;           /* max rate */
    uint32_t channelsMin;       /* min channels */
    uint32_t channelsMax;       /* max channels */
    uint32_t bufferBytesMax;    /* max buffer size */
    uint32_t periodBytesMin;    /* min period size */
    uint32_t periodBytesMax;    /* max period size */
    uint32_t periodsMin;        /* min # of periods */
    uint32_t periodsMax;        /* max # of periods */
    //size_t fifoSize;          /* fifo size in bytes */
    } VX_SND_PCM_HARDWARE;

typedef struct vxSndPcmRuntime
    {
    unsigned long hwPtrUpdateTimestamp; /* Time when hw_ptr is updated */
    SND_FRAMES_T delay; /* extra delay; typically FIFO size */

    /* -- HW params -- */

    uint32_t access;    /* access mode */
    uint32_t format;    /* SNDRV_PCM_FORMAT_* */

    uint32_t rate;      /* rate in Hz */
    uint32_t channels;      /* channels */
    uint32_t periodBytes;   /* period bytes */
    uint32_t periodSize;    /* period size (frames) */
    uint32_t periods;       /* periods */
    SND_FRAMES_T bufferSize;    /* buffer size (frames) */
    SND_FRAMES_T minAlign;  /* Min alignment for the format */
    size_t byteAlign;
    uint32_t frameBits;
    uint32_t sampleBits;
    uint32_t info;

    /* -- mmap -- */

    VX_SND_PCM_MMAP_STATUS status;
    VX_SND_PCM_MMAP_CTRL   control;

    BOOL stopSyncRequired;      /* sync_stop will be called */

    /* -- hardware description -- */
    VX_SND_PCM_HARDWARE hw;
    VX_SND_PCM_SUPPORT_HW_PARAMS supportHwParams;

    /* -- DMA -- */
    uint8_t * dmaArea;    /* DMA buffer address used in audio playback/capture*/
    PHYS_ADDR dmaPhyAddr; /* physical bus address (not accessible from main CPU) */
    size_t dmaAreaBytes;  /* size of DMA buffer */

    VX_SND_SUBSTREAM_DMA_BUF * dmaBufPtr;   /* allocated buffer */
    uint32_t buffer_changed:1;  /* buffer allocation changed; set only in managed mode */

    /* -- SW params -- */

    SND_FRAMES_T startThreshold;
    SND_FRAMES_T stopThreshold;
    } VX_SND_PCM_RUNTIME;

/* function declarations */

#ifdef _WRS_KERNEL

STATUS sndDevRegister (DEV_HDR * pDevHdr, char * name, int drvNum);
STATUS vxSndDevCreate (SND_CARD * card, VX_SND_DEV_TYPE type, void * data,
                       SND_DEV_REG_PTR func, SND_DEVICE ** ppSndDev);
STATUS vxSndDevScMemValidate (const void * addr, int cmd, BOOL syscall);
STATUS snd_device_register_all (SND_CARD * card);
STATUS sndDummyDevRegister (SND_DEVICE * sndDev);
STATUS snd_card_new (VXB_DEV_ID  pDev, SND_CARD ** ppCard);
STATUS snd_card_register (SND_CARD * card);
void   snd_card_free (SND_CARD * card);
uint32_t roundupPowerOfTwoPri (uint32_t n);

#endif /* _WRS_KERNEL */

#if __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCvxSoundCoreh */

