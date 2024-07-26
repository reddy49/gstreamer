/* aboxDemoPlayback.c - abox playback demo */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file provides a capture demo for for Samsung ExynosAuto V920.

The capture procedures are as below:
    1. configure audio path.
       -> amixer(0)
    2. set hardware parameters
       -> vxSndCapDemoHwparaSet(rate, bit_width, channel_number, 1024)
    3. start to capture 10 seconds audio stream from MICs and then play to
       specified speaker
       -> vxSndCapDemo(capture_pcm_number, playback_pcm_number)

Please note, according to limitations of hardware and the configuration in
control demo, the supported parameters are as below:
    rate:                48000/96000
    bit_width:           16/24/32
    channel_number:      2
    capture_pcm_number:  16
    playback_pcm_number: 0/1/2/3
*/

/* includes */

#include <vxWorks.h>
#include <iosLib.h>
#include <vxSoundCore.h>
#include <pcm.h>

/* defines */

#define RECORD_TIME_SEC     (10)    //record 10s sound data for demo

/* function declarations */

/* locals */

LOCAL uint8_t * pCapTestAudData = NULL;
LOCAL uint32_t capTestAudDataSize = 0;
LOCAL VX_SND_PCM_HW_PARAMS testHwPara = {0};
LOCAL BOOL testHwParaIsSet = FALSE;

/* functions */

#ifdef _WRS_KERNEL
void vxSndCapDemoDataDump (void)
    {
    uint32_t    i;

    for (i = 0; i < capTestAudDataSize; i++)
        {
        kprintf ("0x%02x, ", pCapTestAudData[i]);
        if (((i + 1) % 16) == 0)
            {
            kprintf ("\n");
            }
        }
    }

STATUS vxSndCapDemoHwparaSet
    (
    uint32_t    rate,
    uint32_t    bitwidth,
    uint32_t    channel,
    uint32_t    minPeriodBytes
    )
    {
    testHwPara.accessTypeBits = SNDRV_PCM_ACCESS_INTERLEAVED;
    testHwPara.info = 0;

    switch (rate)
        {
        case 44100:
            testHwPara.rates = SNDRV_PCM_RATE_44100;
            break;

        case 48000:
            testHwPara.rates = SNDRV_PCM_RATE_48000;
            break;

        default:
            kprintf ("unsupported sample rate:%d\n", rate);
            return ERROR;
        }

    switch (bitwidth)
        {
        case 16:
            testHwPara.formatBits = VX_SND_FMTBIT_S16_LE;
            break;

        case 24:
            testHwPara.formatBits = VX_SND_FMTBIT_S24_LE;
            break;

        case 32:
            testHwPara.formatBits = VX_SND_FMTBIT_S32_LE;
            break;

        default:
            kprintf ("unsupported bitwidth:%d\n", bitwidth);
            return ERROR;
        }

    if (channel < 2 || channel > 4)
        {
        kprintf ("unsupported channel number:%d\n", channel);
        return ERROR;
        }

    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].min = channel;
    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].max = channel;
    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].openMin = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].openMax = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].integer = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_CHANNELS].empty = 0;

    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].min = minPeriodBytes;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].max = 0xFFFFFFFF;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].openMin = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].openMax = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].integer = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIOD_BYTES].empty = 0;

    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].min = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].max = 0xFFFFFFFF;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].openMin = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].openMax = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].integer = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_PERIODS].empty = 0;

    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].min = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].max = 0xFFFFFFFF;
    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].openMin = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].openMax = 0;
    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].integer = 1;
    testHwPara.intervals[VX_SND_HW_PARAM_BUFFER_BYTES].empty = 0;

    testHwParaIsSet = TRUE;

    return OK;
    }

LOCAL STATUS _vxSndCapDemoPlay
    (
    uint32_t                playId
    )
    {
    char                    pcmPlayDevName[64] = {0};
    int32_t                 pcmPlayFd;
    VX_SND_PCM_HW_PARAMS    hwPara;
    int32_t                 i;
    int32_t                 ret;
    ssize_t                 len;
    uint32_t                selRate = 0;
    uint32_t                selBitwidth = 0;
    uint32_t                selChannels = 0;
    uint32_t                selPeriodBytes = 0;
    uint32_t                selPeriods = 0;
    uint32_t                n;
    uint32_t                leftBytes;
    uint8_t *               pBuf;

    memcpy (&hwPara, &testHwPara, sizeof (VX_SND_PCM_HW_PARAMS));

    sprintf (pcmPlayDevName, "/dev/snd/pcmC0D%dp", playId);
    kprintf ("demo playback on PCM device:%s\n", pcmPlayDevName);

    pcmPlayFd = open (pcmPlayDevName, O_RDWR);
    if (pcmPlayFd < 0)
        {
        kprintf ("open PCM device error\n");
        return ERROR;
        }

    kprintf ("ioctl VX_SND_CMD_HW_REFINE\n");

    ret = ioctl (pcmPlayFd, VX_SND_CMD_HW_REFINE, &hwPara);
    if (ret < 0)
        {
        kprintf ("refine PCM hw_para error\n");
        goto errOut;
        }

    kprintf ("refined Rate: %d\n", PARAMS_RATE (&hwPara));
    kprintf ("refined bitwidth: %d\n", PARAMS_WIDTH (&hwPara));
    kprintf ("refined Channels: %d\n", PARAMS_CHANNELS (&hwPara));
    kprintf ("refined PeriodBytes: %d\n", PARAMS_PERIOD_BYTES (&hwPara));
    kprintf ("refined Periods: %d\n", PARAMS_PERIODS (&hwPara));

    kprintf ("ioctl VX_SND_CMD_HW_PARAMS\n");

    ret = ioctl (pcmPlayFd, VX_SND_CMD_HW_PARAMS, &hwPara);
    if (ret < 0)
        {
        kprintf ("set PCM hw_para error\n");
        goto errOut;
        }

    selRate = PARAMS_RATE (&hwPara);
    selBitwidth = PARAMS_WIDTH (&hwPara);
    selChannels = PARAMS_CHANNELS (&hwPara);
    selPeriodBytes = PARAMS_PERIOD_BYTES (&hwPara);
    selPeriods = PARAMS_PERIODS (&hwPara);

    kprintf ("selected Rate: %d\n", selRate);
    kprintf ("selected bitwidth: %d\n", selBitwidth);
    kprintf ("selected Channels: %d\n", selChannels);
    kprintf ("selected PeriodBytes: %d\n", selPeriodBytes);
    kprintf ("selected Periods: %d\n", selPeriods);

    kprintf ("ioctl VX_SND_CMD_PREPARE\n");

    ret = ioctl (pcmPlayFd, VX_SND_CMD_PREPARE);
    if (ret < 0)
        {
        kprintf ("set PCM state to prepare failed\n");
        goto errOut;
        }

    /* play test data */

    n = capTestAudDataSize / selPeriodBytes;
    leftBytes = capTestAudDataSize % selPeriodBytes;
    pBuf = pCapTestAudData;

    kprintf ("start %d rounds write() for %d bytes data\n",
             leftBytes == 0 ? n : n + 1, capTestAudDataSize);

    for (i = 0; i < n; i++)
        {
        len= write (pcmPlayFd, pBuf + (i * selPeriodBytes), selPeriodBytes);
        if (len <= 0)
            {
            kprintf ("PCM transfer data buf:0x%llx size:%d error\n",
                     (uint64_t) (pBuf + (i * selPeriodBytes)), selPeriodBytes);
            goto errOut;
            }
        }
    if (leftBytes != 0)
        {
        len= write (pcmPlayFd, pBuf + (n * selPeriodBytes), leftBytes);
        if (len < 0)
            {
            kprintf ("PCM transfer data buf:0x%llx size:%d error\n",
                     (uint64_t) (pBuf + (n * selPeriodBytes)), leftBytes);
            goto errOut;
            }
        }

    /* clean up PCM device */

    kprintf ("ioctl VX_SND_CMD_DRAIN\n");

    ret = ioctl (pcmPlayFd, VX_SND_CMD_DRAIN);
    if (ret < 0)
        {
        kprintf ("set PCM drain failed\n");
        goto errOut;
        }

    kprintf ("close PCM device:%s\n", pcmPlayDevName);

    close (pcmPlayFd);
    return OK;

errOut:
    close (pcmPlayFd);
    return ERROR;
    }

LOCAL STATUS _vxSndCapDemo
    (
    uint32_t                capId,
    uint32_t                playId
    )
    {
    char                    pcmCapDevName[64] = {0};
    int32_t                 pcmCapFd;
    VX_SND_PCM_HW_PARAMS    hwPara;
    int32_t                 i;
    int32_t                 ret;
    ssize_t                 len;
    uint32_t                selRate = 0;
    uint32_t                selBitwidth = 0;
    uint32_t                selChannels = 0;
    uint32_t                selPeriodBytes = 0;
    uint32_t                selPeriods = 0;
    uint32_t                n;
    uint32_t                leftBytes;
    uint8_t *               pBuf;
    BOOL                    push;

    if (!testHwParaIsSet)
        {
        kprintf ("Please set the HW parameters by vxSndCapDemoHwparaSet() first\n");
        return ERROR;
        }

    memcpy (&hwPara, &testHwPara, sizeof (VX_SND_PCM_HW_PARAMS));

    sprintf (pcmCapDevName, "/dev/snd/pcmC0D%dc", capId);
    kprintf ("demo capture on PCM device:%s\n", pcmCapDevName);

    pcmCapFd = open (pcmCapDevName, O_RDWR);
    if (pcmCapFd < 0)
        {
        kprintf ("open PCM device:%s error\n", pcmCapDevName);
        return ERROR;
        }

    kprintf ("ioctl VX_SND_CMD_HW_REFINE\n");

    ret = ioctl (pcmCapFd, VX_SND_CMD_HW_REFINE, &hwPara);
    if (ret < 0)
        {
        kprintf ("refine PCM hw_para error\n");
        goto errOut;
        }

    kprintf ("refined Rate: %d\n", PARAMS_RATE (&hwPara));
    kprintf ("refined bitwidth: %d\n", PARAMS_WIDTH (&hwPara));
    kprintf ("refined Channels: %d\n", PARAMS_CHANNELS(&hwPara));
    kprintf ("refined PeriodBytes: %d\n", PARAMS_PERIOD_BYTES (&hwPara));
    kprintf ("refined Periods: %d\n", PARAMS_PERIODS (&hwPara));

    kprintf ("ioctl VX_SND_CMD_HW_PARAMS\n");

    ret = ioctl (pcmCapFd, VX_SND_CMD_HW_PARAMS, &hwPara);
    if (ret < 0)
        {
        kprintf ("set PCM hw_para error\n");
        goto errOut;
        }

    selRate = PARAMS_RATE (&hwPara);
    selBitwidth = PARAMS_WIDTH (&hwPara);
    selChannels = PARAMS_CHANNELS(&hwPara);
    selPeriodBytes = PARAMS_PERIOD_BYTES (&hwPara);
    selPeriods = PARAMS_PERIODS (&hwPara);

    kprintf ("selected Rate: %d\n", selRate);
    kprintf ("selected bitwidth: %d\n", selBitwidth);
    kprintf ("selected Channels: %d\n", selChannels);
    kprintf ("selected PeriodBytes: %d\n", selPeriodBytes);
    kprintf ("selected Periods: %d\n", selPeriods);

    kprintf ("ioctl VX_SND_CMD_PREPARE\n");

    ret = ioctl (pcmCapFd, VX_SND_CMD_PREPARE);
    if (ret < 0)
        {
        kprintf ("set PCM state to prepare failed\n");
        goto errOut;
        }

    /* alloc buf */

    if (pCapTestAudData != NULL)
        {
        free (pCapTestAudData);
        pCapTestAudData = NULL;
        }

    capTestAudDataSize = (selRate * selBitwidth * selChannels) / 8 * RECORD_TIME_SEC;
    pCapTestAudData = malloc (capTestAudDataSize);
    if (pCapTestAudData == NULL)
        {
        kprintf ("alloc %d memory for recording failed\n", capTestAudDataSize);
        goto errOut;
        }

    /* capture test data */

    n = capTestAudDataSize / selPeriodBytes;
    leftBytes = capTestAudDataSize % selPeriodBytes;
    pBuf = pCapTestAudData;

    kprintf ("start %d rounds read() for %d bytes data\n",
             leftBytes == 0 ? n : n + 1, capTestAudDataSize);

    for (i = 0; i < n; i++)
        {
        len= read (pcmCapFd, pBuf + (i * selPeriodBytes), selPeriodBytes);
        if (len <= 0)
            {
            kprintf ("PCM transfer data buf:0x%llx size:%d error\n",
                     (uint64_t) (pBuf + (i * selPeriodBytes)), selPeriodBytes);
            goto errOut;
            }
        }
    if (leftBytes != 0)
        {
        len= read (pcmCapFd, pBuf + (n * selPeriodBytes), leftBytes);
        if (len < 0)
            {
            kprintf ("PCM transfer data buf:0x%llx size:%d error\n",
                     (uint64_t) (pBuf + (n * selPeriodBytes)), leftBytes);
            goto errOut;
            }
        }

    /* clean up PCM device */

    kprintf ("ioctl VX_SND_CMD_PAUSE\n");

    push = TRUE;
    ret = ioctl (pcmCapFd, VX_SND_CMD_PAUSE, &push);
    if (ret < 0)
        {
        kprintf ("set PCM state to pause failed\n");
        goto errOut;
        }

    kprintf ("close PCM device:%s\n", pcmCapDevName);

    close (pcmCapFd);

    /* play the captured pcm data */

    kprintf ("\n\n");
    (void) _vxSndCapDemoPlay (playId);

    return OK;

errOut:
    close (pcmCapFd);
    return ERROR;
    }

STATUS vxSndCapDemo
    (
    uint32_t    capId,
    uint32_t    playId
    )
    {
    TASK_ID     capTask;

    capTask = taskCreate ("aboxPcmCTask",
                          120,
                          0,
                          128 * 1024,
                          (FUNCPTR) _vxSndCapDemo,
                          (_Vx_usr_arg_t) capId,
                          (_Vx_usr_arg_t) playId,
                          0, 0, 0, 0, 0, 0, 0, 0);
    if (capTask == TASK_ID_ERROR)
        {
        kprintf ("create abox PCM capture task failed\n");
        return ERROR;
        }

    if (taskActivate (capTask) != OK)
        {
        kprintf ("active abox PCM capture task failed\n");
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

