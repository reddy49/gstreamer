/* tmAudioNew.h - test audio new header file */

#ifndef __INCtestAudioNewh
#define __INCtestAudioNewh

#define TM_DAI_CPU_NAME         "testDaiDriver-cpu"
#define TM_DAI_PLAT_NAME        "testDaiDriver-platform"
#define TM_DAI_CODEC_NAME       "testDaiDriver-codec"

#define AUDIO_TEST_CARD_NAME                "testSndCard"


#define DEBUG_VX_SND_TEST 1
#ifdef DEBUG_VX_SND_TEST
#include <private/kwriteLibP.h>    /* _func_kprintf */

#define TEST_SND_AUD_DBG_OFF          0x00000000U
#define TEST_SND_AUD_DBG_ERR          0x00000001U
#define TEST_SND_AUD_DBG_INFO         0x00000002U
#define TEST_SND_AUD_DBG_VERBOSE      0x00000004U
#define TEST_SND_AUD_DBG_ALL          0xffffffffU

#define TEST_SND_DEG_MSG(mask, fmt, ...)                                  \
    do                                                                  \
        {                                                               \
        if ((g_testSndPcmDbgMask & (mask)))                                \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_TEST : [%s,%d] "fmt, __func__,  \
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define TEST_AUD_MODE_INFO(...)                                           \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define TEST_AUD_ERR(fmt, ...)    \
                  TEST_SND_DEG_MSG(TEST_SND_AUD_DBG_ERR,     fmt, ##__VA_ARGS__)
#define TEST_AUD_INFO(fmt, ...)   \
                  TEST_SND_DEG_MSG(TEST_SND_AUD_DBG_INFO,    fmt, ##__VA_ARGS__)
#define TEST_AUD_DBG(fmt, ...)    \
                  TEST_SND_DEG_MSG(TEST_SND_AUD_DBG_VERBOSE, fmt, ##__VA_ARGS__)
#else /* DEBUG_VX_SND_TEST */
#define TEST_AUD_ERR(x...)      do { } while (FALSE)
#define TEST_AUD_INFO(x...)     do { } while (FALSE)
#define TEST_AUD_DBG(x...)      do { } while (FALSE)
#define TEST_AUD_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_TEST */

#define AUD_TEST_DEV_NUM                  0
#define AUD_TEST_PARA_UNKNOW              0

#define AUD_TEST_CTRL_MAX_NUM              512
#define AUD_TEST_CTRL_INFO_NUM             (2*4096)
#define AUD_TEST_TLV_BUF_LEN               64

/* 
 *the playback test need to prepare a wav file and update AUD_TEST_WAV_FILE
 */

#define AUD_TEST_WAV_FILE      "/romfs/01_song1__48kHz_16b_2ch_18s.wav"
#define AUD_TEST_RECORED_FILE  "/ram0/caputreWav.wav"

#define AUD_TEST_RECORD_CHANNEL        2
#define AUD_TEST_FORMAT                VX_SND_FMTBIT_S16_LE
#define AUD_TEST_RECORD_SAMPLE_BIT     16
#define AUD_TEST_RECORD_SECOND         (16)
#define AUD_TEST_RECORD_SAMPLE_RATE    SNDRV_PCM_RATE_48000
#define AUD_TEST_AUD_DMA_BUFF_SIZE     (64*1024)
#define AUD_TEST_RECORD_FACTOR         (1)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#if (_BYTE_ORDER == _BIG_ENDIAN)
#define MEM_SWAP_LONG(x)    LONGSWAP (x)
#define MEM_SWAP_WORD(x)    ((UINT16) (MSB (x) | LSB (x) << 8))
#define MEMLNBYTE(x)        LMSB (x)
#define MEMLSBYTE(x)        LNMSB (x)
#define MEMLTBYTE(x)        LNLSB (x)
#define MEMLFBYTE(x)        LLSB (x)
#define MEMLWORD(x)         MSW (x)
#define MEMMWORD(x)         LSW (x)
#else
#define MEM_SWAP_LONG(x)    (x)
#define MEM_SWAP_WORD(x)    (x)
#define MEMLNBYTE(x)        LLSB (x)
#define MEMLSBYTE(x)        LNLSB (x)
#define MEMLTBYTE(x)        LNMSB (x)
#define MEMLFBYTE(x)        LMSB (x)
#define MEMLWORD(x)         LSW (x)
#define MEMMWORD(x)         MSW (x)
#endif /* _BYTE_ORDER == _BIG_ENDIAN */

#define TEST_CTRL_CONFIG_ENUM(name, a)            \
    {                                                \
    .ctrlName = name,                                \
    .type = VX_SND_CTL_DATA_TYPE_ENUMERATED,         \
    .value = {.enumerated = {.value = a}}       }

#define TEST_CTRL_CONFIG_INT(name, b)            \
    {                                                \
    .ctrlName = name,                                \
    .type = VX_SND_CTL_DATA_TYPE_INTEGER,            \
    .value = {.integer32 = {.value = b}}        }

#define TEST_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xitems, xtexts) \
    {   .reg = xreg, .shiftLeft = xshift_l, .shiftRight = xshift_r, \
        .itemNum = xitems, .textList = xtexts, \
        .mask = xitems}

#define TEST_SOC_ENUM_EXT_MULTI(xname, xenum, xinfo, xget, xput) \
{   .id.name = xname, \
    .info = xinfo, \
    .get = xget, \
    .put = xput, \
    .privateValue = (unsigned long)&xenum }


#define TEST_DOUBLE_VALUE(xreg, shift_left, shift_right, xmax, xinvert, xautodisable) \
            ((unsigned long)&(struct vxSndMixerControl) \
            {.reg = xreg, .rreg = xreg, .shift = shift_left, \
            .shiftRight = shift_right, .max = xmax, .platformMax = xmax, \
            .invert = xinvert, .autodisable = xautodisable})
#define TEST_SINGLE_VALUE(xreg, xshift, xmax, xinvert, xautodisable) \
        TEST_DOUBLE_VALUE(xreg, xshift, xshift, xmax, xinvert, xautodisable)

#define TEST_SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) \
{   .id.iface = SNDRV_CTL_TYPE_MIXER, \
    .id.name = xname, \
    .id.access = VX_SND_CTRL_ACCESS_TLV_READ | VX_SND_CTRL_ACCESS_READWRITE, \
    .tlv.p = (tlv_array), \
    .info = vxSndCtrlVolumeInfo, \
    .get = vxSndCtrlVolumeGet, \
    .put = vxSndCtrlVolumePut, \
    .privateValue = TEST_SINGLE_VALUE(reg, shift, max, invert, 0) }

/* PCM waveform format */

typedef struct tmPcmWaveFormat
    {
    UINT16  formatTag;          /* type of wave form data */
    UINT16  channels;           /* number of audio channels */
    UINT32  samplesPerSec;      /* audio samples per second */
    UINT32  avgBytesPerSec;     /* average transfer rate */
    UINT16  blockAlign;         /* bytes required for a single sample */
    UINT16  bitsPerSample;      /* bits per sample */
    } TEST_PCM_WAVE_FORMAT;

typedef struct wavFileHeader
    {
    UINT8           tagRIFF [4];
    UINT32          lenRIFF;
    UINT8           tagWAVE [4];
    UINT8           tagFMT [4];
    UINT32          lenFMT;
    TEST_PCM_WAVE_FORMAT pcmFormat;
    UINT8           tagDATA [4];
    UINT32          lenDATA;
    } WAV_FILE_HEADER;

typedef enum
    {
    FILE_HEAD_OPT0 = 0,
    FILE_HEAD_OPT1,
    FILE_HEAD_OPT2,
    FILE_HEAD_OPT3,
    FILE_HEAD_BOTT
    }AUD_TEST_F_HEAD;

typedef struct tmAudioDataInfo
    {
    UINT32   sampleRate;         /* rate bit */
    UINT32   sampleBits;         /* valid data bits in data buffer */
    UINT32   sampleBytes;        /* size of sample in data buffer */
    UINT32   format;
    UINT32   channels;
    BOOL     useMsb;             /* valid data stores as MSB or LSB */
    } TEST_AUD_DATA_INFO;

typedef struct ctrlConfigInfo
    {
    char * ctrlName;
    VX_SND_CTRL_DATA_TYPE type;
    union
        {
        struct
            {
            UINT32 value;
            } integer32;
        struct
            {
            UINT64 value;
            } integer64;
        struct
            {
            char *value;
            } enumerated;
//        unsigned char reserved[128];
        } value;
    }TEST_CTRL_CONFIG_INFO;

typedef struct audioInfo
    {
    INT32 devFd;               /* for PCM p */
    char  playbackName[SND_DEV_NAME_LEN];
    INT32 devFdCapture;        /* for PCM c */
    char  captureName[SND_DEV_NAME_LEN];
    BOOL  playback;            /* to describe the test for playback or capture */

    INT32 ctrlFd;

    INT32 wavFd;
    char *wavfilename;

    char *recordfilename;
    INT32 recordFd;
    BOOL  needRemove;

    char *wavData;
    UINT32 dataSize;
    UINT32  dataStart;
    
    UINT32 rate;
    UINT32 channels;
    UINT32 periodBytes;
    UINT32 periods;

    UINT32 numCtrlConf;
    TEST_CTRL_CONFIG_INFO * ctrlConfInfo;

    TEST_AUD_DATA_INFO audioData;
    }TEST_AUDIO_INFO;

typedef struct fileHeader
    {
    UINT32 channels;
    UINT32 recSec;
    UINT32 sampleBits;
    UINT32 sampleRate;
    UINT32 format;
    }TEST_FILE_HEADER_PARA;

typedef struct ctrlInfoAll
    {
    UINT32 cnt;
    VX_SND_CTRL_INFO ctrlInfo[AUD_TEST_CTRL_INFO_NUM];
    }TEST_CTRL_INFO_ALL;

#ifndef VXB_DEV
typedef struct vxbDev
    {
    SL_NODE         vxbNode;
    SL_NODE         vxbAttachNode;

    UINT32          vxbUnit;
    VXB_BUSTYPE_ID  vxbClass;

    /* device name */

    char *          pName;
    void *          pVxbIvars;

    /* per-driver info */

    void *          pVxbSoftc;
    void *          pVxbDrvData;
    SL_LIST         vxbList;
    SEM_ID          pVxbDevSem;
    struct vxbDev * pVxbParent;
    int             vxbToggle;
    UINT32          vxbFlags;
    int             vxbRefCnt;
    VXB_DRV *       pVxbDriver;
    VXB_KEY         vxbSerial;
    void *          pVxbParams;
    UINT32          level;
    char *          pNameAddr;
    char *          pNameOrphan;
    } VXB_DEV;
#endif

extern UINT32 g_testSndPcmDbgMask;
extern SND_CONTROL_DEV * g_testCtrl;
extern STATUS getWavData (TEST_AUDIO_INFO *audioInfo, BOOL fromfile, char* wavfile);
extern STATUS setupAudioDev (TEST_AUDIO_INFO *audioInfo);
extern STATUS sendDataToPlayback(TEST_AUDIO_INFO *audioInfo);
extern STATUS recoredWavData(TEST_AUDIO_INFO *audioInfo, char* wavfile);
extern STATUS cleanupAudioDev(TEST_AUDIO_INFO *audioInfo);
extern STATUS cleanupAudioInfo(TEST_AUDIO_INFO *audioInfo);

#endif
