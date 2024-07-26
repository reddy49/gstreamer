/* vxSoundDemoPlayback.c - abox playback demo */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file provides a playback demo for for Samsung ExynosAuto V920.

The playback procedures are as below:
    1. configure audio path.
       -> amixer(0)
    2. play wav file to specified PCM output.
       -> vxSndPlayDemo(pcm_number, "wav file name")

In this demo, there are four PCMs, the related pcm_number is 0, 1, 2 and 3.
They correspond to different auido output interfaces on V920 audio extend board.

The wav file should be added to project via file system, for example, use romfs
to include wav files. This demo can only support wav files using below audio
parameters:
    rate:    48000/96000,
    format:  S32_LE/S24_LE/S16_LE
    channel: 1/2 (I2S) or 8 (TDM)
*/

/* includes */

#include <vxWorks.h>
#include <iosLib.h>
#include <vxSoundCore.h>
#include <audioLibWav.h>
#include <pcm.h>

/* defines */

#define TEST_AUD_DATA_SIZE      ((48000 * 16 * 2) / 8 * 2) //2 seconds data
#define AUD_TEST_RECORD_CHANNEL        2
#define AUD_TEST_FORMAT                VX_SND_FMTBIT_S16_LE
#define AUD_TEST_RECORD_SAMPLE_BIT     16
#define AUD_TEST_RECORD_PERIOD         (1024*32)
#define AUD_TEST_RECORD_SAMPLE_RATE    SNDRV_PCM_RATE_48000
#define AUD_TEST_AUD_DMA_BUFF_SIZE     (64*1024)


typedef struct tmAudioDataInfo
    {
    UINT32   sampleRate;         /* rate bit */
    UINT32   sampleBits;         /* valid data bits in data buffer */
    UINT32   sampleBytes;        /* size of sample in data buffer */
    UINT32   format;
    UINT32   channels;
    BOOL     useMsb;             /* valid data stores as MSB or LSB */
    } TEST_AUD_DATA_INFO;

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

    TEST_AUD_DATA_INFO audioData;
    }TEST_AUDIO_INFO;

typedef struct fileHeader
    {
    UINT32 channels;
    UINT32 recPeriod;
    UINT32 sampleBits;
    UINT32 sampleRate;
    UINT32 format;
    }TEST_FILE_HEADER_PARA;

typedef enum
    {
    FILE_HEAD_OPT0 = 0,
    FILE_HEAD_OPT1,
    FILE_HEAD_OPT2,
    FILE_HEAD_OPT3,
    FILE_HEAD_BOTT
    }AUD_TEST_F_HEAD;

/* function declarations */

/* locals */
LOCAL unsigned char g_buffer[16*1024];
//LOCAL uint8_t testAudData[TEST_AUD_DATA_SIZE];

LOCAL TEST_FILE_HEADER_PARA g_testFilePara[FILE_HEAD_BOTT] =
    {
        {
        .channels =     AUD_TEST_RECORD_CHANNEL,
        .recPeriod =    AUD_TEST_RECORD_PERIOD,
        .sampleBits =   AUD_TEST_RECORD_SAMPLE_BIT,
        .sampleRate =   AUD_TEST_RECORD_SAMPLE_RATE,
        .format     =   AUD_TEST_FORMAT,
        },
    };


LOCAL VX_SND_INTERVAL testInterval[VX_SND_HW_PARAM_IDX_MAX] =
    {
    [VX_SND_HW_PARAM_CHANNELS]=
        {
        .min = 0,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_PERIOD_BYTES]=
        {
        .min = 2560,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_PERIODS]=
        {
        .min = 1,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_BUFFER_BYTES]=
        {
        .min = 1,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    };

/* globals */

LOCAL STATUS audioOpenReadFile
    (
    TEST_AUDIO_INFO *  pAudInfo
    )
    {
    int     sampleBits;
    UINT32  size = 0;
    int     channels;
    UINT32  sampleRate;
    UINT32  samples;
    UINT32  dataStart;
    UINT32  readBytes;
    VX_SND_PCM_HARDWARE hw;

    /* open the audio file */

    pAudInfo->wavFd = open ((char *)pAudInfo->wavfilename, O_RDONLY, 0666);
    if (pAudInfo->wavFd < 0)
        {
        kprintf("Error opening file %s\n", pAudInfo->wavfilename);
        return ERROR;
        }

    /* assume a wav file and read the wav form header file */

    if (audioWavHeaderRead (pAudInfo->wavFd, &channels, &sampleRate, &sampleBits,
                            &samples, &dataStart) != OK)
        {
        return ERROR;
        }

    if ((16 != sampleBits) && (24 != sampleBits) & (32 != sampleBits))
        {
        sampleBits = 8;
        }

    size = samples * channels * (sampleBits >> 3);

    /* print characteristics of the sound data stream  */

    kprintf ("File name:   %s\n", pAudInfo->wavfilename);
    kprintf ("Channels:    %d\n", channels);
    kprintf ("Sample Rate: %d\n", sampleRate);
    kprintf ("Sample Bits: %d\n", sampleBits);
    kprintf ("Samples:     %d\n", samples);

    hw.rates = 0;
    hw.rateMin = sampleRate;
    hw.rateMax = sampleRate;
    vxSndPcmHwRateToBitmap(&hw);

    pAudInfo->audioData.channels     = (UINT8)channels;
    pAudInfo->audioData.sampleBits   = (UINT8)sampleBits;
    pAudInfo->audioData.sampleBytes  = (UINT8)(sampleBits >> 3);
    pAudInfo->audioData.sampleRate   = hw.rates;
    pAudInfo->audioData.useMsb       = 0;
    pAudInfo->dataStart              = dataStart;
    pAudInfo->dataSize               = size;

    switch (sampleBits)
        {
        case 8:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S8;
            break;
        case 16:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S16_LE;
            break;
        case 24:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S24_LE;
            break;
        case 32:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S32_LE;
            break;
        default:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S16_LE;
            break;
        }
    kprintf ("Formats:     %d\n", pAudInfo->audioData.format);
    kprintf ("dataStart:     %d\n", pAudInfo->dataStart);
    kprintf ("dataSize:     %d\n", pAudInfo->dataSize);


    pAudInfo->wavData = (char*) malloc(pAudInfo->dataSize);
    if (pAudInfo->wavData == NULL)
        {
        kprintf("malloc wavData buffer failed\n");
        return ERROR;
        }

    /* Position to data start of file */

    if (ERROR == lseek (pAudInfo->wavFd, dataStart, SEEK_SET))
        {
        return ERROR;
        }

    /* Read enough data from the file */

    readBytes = (UINT32)read (pAudInfo->wavFd,
                                pAudInfo->wavData,
                                pAudInfo->dataSize);
    if (readBytes < pAudInfo->dataSize)
        {
        kprintf("read %u bytes, expected %u bytes\n",
                     readBytes, pAudInfo->dataSize);
        return ERROR;
        }
    close (pAudInfo->wavFd);

    return OK;
    }


LOCAL STATUS getWavData
    (
    TEST_AUDIO_INFO *audioInfo,
    BOOL fromfile,
    char* wavfile
    )
    {
    STATUS ret;
    unsigned int i;

    if (fromfile)
        {
        if (wavfile == NULL)
            {
            return ERROR;
            }
        kprintf ("wavefile: %s\n", wavfile);
        audioInfo->wavfilename = wavfile;
        ret = audioOpenReadFile (audioInfo);
        if (ret == ERROR)
            {
            kprintf("get audio data info fail\n");
            return ERROR;
            }
        }
    else
        {
        /* generate some random data */
        for (i = 0; i < sizeof(g_buffer); i++)
            {
            g_buffer[i] = rand() & 0xff;
            }

        audioInfo->wavData = (char *)&g_buffer[0];
        audioInfo->dataSize = sizeof (g_buffer);

        audioInfo->audioData.channels = g_testFilePara[FILE_HEAD_OPT0].channels;
        audioInfo->audioData.sampleBits = g_testFilePara[FILE_HEAD_OPT0].sampleBits;
        audioInfo->audioData.sampleBytes = (g_testFilePara[FILE_HEAD_OPT0].sampleBits >> 3);
        audioInfo->audioData.sampleRate = SNDRV_PCM_RATE_48000;
        audioInfo->audioData.format = g_testFilePara[FILE_HEAD_OPT0].format;

        audioInfo->dataSize = sizeof (g_buffer);
        audioInfo->dataStart = 0;

        kprintf ("File name:   random data\n");
        kprintf ("Channels:    %d\n", audioInfo->audioData.channels);
        kprintf ("Sample Rate: %d\n", audioInfo->audioData.sampleRate);
        kprintf ("Sample Bits: %d\n", audioInfo->audioData.sampleBits);
        kprintf ("samples:     %d\n", audioInfo->dataSize / audioInfo->audioData.sampleBytes);
        kprintf ("format:      %d\n", audioInfo->audioData.format);

        }

    return OK;
    }

LOCAL STATUS sendDataToPlayback(TEST_AUDIO_INFO *audioInfo)
    {
    char *  buff = NULL;
    UINT32  buffSize = 0;
    long    ret;
    UINT32  i = 0;
    UINT32  n, leftBytes;

    buffSize = audioInfo->periodBytes;
    buff = (char *)malloc(buffSize);
    if (buff == NULL)
        {
        kprintf("malloc buff for write failed\n");
        return ERROR;
        }
    n = audioInfo->dataSize / buffSize;
    leftBytes = audioInfo->dataSize % buffSize;

    /* write to playback */
    for (i = 0; i < n; i ++)
        {
        memcpy (buff,
                audioInfo->wavData + (i*buffSize),
                buffSize);
        ret= write (audioInfo->devFd, buff, buffSize);
        if (ret < 0)
            {
            kprintf("to transfer data to PCM error\n");
            return ERROR;
            }
        }

    if (leftBytes != 0)
        {
        memset (buff, 0, buffSize);
        memcpy (buff, audioInfo->wavData + (n*buffSize), leftBytes);
        ret= write (audioInfo->devFd, buff, buffSize);
        if (ret < 0)
            {
            kprintf("to transfer data to PCM error\n");
            free(buff);
            return ERROR;
            }
        }
    free(buff);

    return OK;
    }

#ifdef _WRS_KERNEL
LOCAL STATUS _vxSndPlayDemo
    (
    uint32_t                devId,
    char *                  wavFile
    )
    {
    char                    pcmDevName[64] = {0};
    int32_t                 pcmFd;
    VX_SND_PCM_HW_PARAMS    hwPara;
    int32_t                 i;
    int32_t                 ret;
    uint32_t                selRate = 0;
    uint32_t                selChannels = 0;
    uint32_t                selPeriodBytes = 0;
    uint32_t                selPeriods = 0;
    TEST_AUDIO_INFO         audioInfo;
    STATUS retVal;

    sprintf (pcmDevName, "/dev/snd/pcmC0D%dp", devId);

    kprintf ("demo on PCM device:%s\n", pcmDevName);

    pcmFd = open (pcmDevName, O_RDWR);
    if (pcmFd < 0)
        {
        kprintf ("open PCM device error\n");
        return ERROR;
        }

    memset (&audioInfo, 0, sizeof(TEST_AUDIO_INFO));
    retVal = getWavData (&audioInfo, TRUE, wavFile);
    if (retVal != OK)
        {
        kprintf ("get wav file audio info failed\n");
        return ERROR;
        }

    audioInfo.devFd = pcmFd;
    hwPara.accessTypeBits = SNDRV_PCM_ACCESS_INTERLEAVED;

    hwPara.info = 0;
    for (i = VX_SND_HW_PARAM_CHANNELS; i < VX_SND_HW_PARAM_IDX_MAX; i++)
        {
        hwPara.intervals[i].min = testInterval[i].min;
        hwPara.intervals[i].max = testInterval[i].max;
        hwPara.intervals[i].openMin = testInterval[i].openMin;
        hwPara.intervals[i].openMax = testInterval[i].openMax;
        hwPara.intervals[i].integer = testInterval[i].integer;
        hwPara.intervals[i].empty = testInterval[i].empty;
        }
    hwPara.formatBits = audioInfo.audioData.format;
    hwPara.intervals[VX_SND_HW_PARAM_CHANNELS].min = audioInfo.audioData.channels;
    hwPara.intervals[VX_SND_HW_PARAM_CHANNELS].max = audioInfo.audioData.channels;
    hwPara.rates = audioInfo.audioData.sampleRate;

    kprintf ("\taccessTypeBits is:\t0x%08x\n", hwPara.accessTypeBits);
    kprintf ("\tformatBits is:\t0x%016llx\n", hwPara.formatBits);
    kprintf ("\tratesBits is:\t0x%08x\n", hwPara.rates);
    kprintf ("\tinfo is:\t0x%08llx\n", hwPara.info);

    kprintf ("ioctl VX_SND_CMD_HW_REFINE\n");

    ret = ioctl (pcmFd, VX_SND_CMD_HW_REFINE, &hwPara);
    if (ret < 0)
        {
        kprintf ("refine PCM hw_para error\n");
        goto errOut;
        }

    kprintf ("refined Rate: %d\n", PARAMS_RATE (&hwPara));
    kprintf ("refined Channels: %d\n", PARAMS_CHANNELS(&hwPara));
    kprintf ("refined PeriodBytes: %d\n", PARAMS_PERIOD_BYTES (&hwPara));
    kprintf ("refined Periods: %d\n", PARAMS_PERIODS (&hwPara));

    kprintf ("ioctl VX_SND_CMD_HW_PARAMS\n");

    ret = ioctl (pcmFd, VX_SND_CMD_HW_PARAMS, &hwPara);
    if (ret < 0)
        {
        kprintf ("set PCM hw_para error\n");
        goto errOut;
        }

    selRate = PARAMS_RATE (&hwPara);
    selChannels = PARAMS_CHANNELS(&hwPara);
    selPeriodBytes = PARAMS_PERIOD_BYTES (&hwPara);
    selPeriods = PARAMS_PERIODS (&hwPara);

    audioInfo.periodBytes = selPeriodBytes;
    audioInfo.rate = selRate;
    audioInfo.channels = selChannels;
    audioInfo.periods =selPeriods;

    kprintf ("selRate: %d\n", selRate);
    kprintf ("selChannels: %d\n", selChannels);
    kprintf ("selPeriodBytes: %d\n", selPeriodBytes);
    kprintf ("selPeriods: %d\n", selPeriods);

    kprintf ("ioctl VX_SND_CMD_PREPARE\n");

    ret = ioctl (pcmFd, VX_SND_CMD_PREPARE);
    if (ret < 0)
        {
        kprintf ("set PCM state to prepare failed\n");
        goto errOut;
        }

    /* send wav data */

    kprintf("audio device is ready, start to send data to PCM\n");
    retVal = sendDataToPlayback(&audioInfo);
    if (retVal != OK)
        {
        kprintf("sent data to playback failed\n");
        return ERROR;
        }

    /* clean up PCM device */

    kprintf ("ioctl VX_SND_CMD_DRAIN\n");

    ret = ioctl (pcmFd, VX_SND_CMD_DRAIN);
    if (ret < 0)
        {
        kprintf ("set PCM drain failed\n");
        }

    kprintf ("close PCM device\n");

    close(pcmFd);
    if (audioInfo.wavData != NULL)
        free(audioInfo.wavData);

    return OK;

errOut:
    close (pcmFd);
    if (audioInfo.wavData != NULL)
        free(audioInfo.wavData);
    return ERROR;
    }

STATUS vxSndPlayDemo
    (
    uint32_t    devId,
    char *      wavFile
    )
    {
    TASK_ID     playTask;

    playTask = taskCreate ("aboxPcmPTask",
                          120,
                          0,
                          128 * 1024,
                          (FUNCPTR) _vxSndPlayDemo,
                          (_Vx_usr_arg_t) devId,
                          (_Vx_usr_arg_t)wavFile, 0, 0, 0, 0, 0, 0, 0, 0);
    if (playTask == TASK_ID_ERROR)
        {
        kprintf ("create abox PCM playback task failed\n");
        return ERROR;
        }

    if (taskActivate (playTask) != OK)
        {
        kprintf ("active abox PCM playback task failed\n");
        return ERROR;
        }

    return OK;
    }
#else

/*******************************************************************************
*
* main - start of the demo program in RTP mode
*
* This is the RTP mode entry point.
*
* RETURNS: EXIT_SUCCESS, or non-zero if the routine had to abort
*
* ERRNO: N/A
*
* \NOMANUAL
*/

int main
    (
    int     argc,
    char *  argv[]
    )
    {

    return 0;
    }
#endif /* _WRS_KERNEL */

