/* audioLibCore.h - Audio Driver Framework Core Library Header File */

/*
 * Copyright (c) 2013-2014, 2016-2017, 2019, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file defines the audio functions and structures for the audio module
*/

#ifndef __INCaudioLibCoreh
#define __INCaudioLibCoreh

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <semLib.h>
#include <stdio.h>
#include <sysLib.h>
#include <msgQLib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus  */

/* defines */

#define AUDIO_DEV_PREFIX                    "/audio/"
#define AUDIO_DEFAULT_DEV                   "/audio/0"
#define AUDIO_DEV_NAME_LEN                  (32)
#define AUDIO_DEV_MAX                       (32)

/* audio out path */

#define AUDIO_OUT_MASK                      (0xFFFF)
#define AUDIO_LINE_OUT                      (1 << 0)
#define AUDIO_SPEAKER_OUT                   (1 << 1)
#define AUDIO_HP_OUT                        (1 << 2)

/* audio in path */

#define AUDIO_IN_MASK                       (0xFFFF << 16)
#define AUDIO_LINE_IN                       (1 << 16)
#define AUDIO_MIC_IN                        (1 << 17)

#define AUDIO_DEV_INIT_TASK_PRIO            200
#define AUDIO_DEV_INIT_TASK_STACK_SIZE      (1024 * 8)

/* typedefs */

typedef int AUDIO_IO_CTRL;

typedef struct audioDataInfo
    {
    UINT32  sampleRate;
    UINT8   sampleBits;         /* valid data bits in data buffer */
    UINT8   sampleBytes;        /* size of sample in data buffer */
    UINT8   channels;
    BOOL    useMsb;             /* valid data stores as MSB or LSB */
    } AUDIO_DATA_INFO;

typedef struct audioVolume
    {
    UINT32  path;
    UINT8   left;               /* left volume */
    UINT8   right;              /* right volume */
    } AUDIO_VOLUME;

typedef struct audioDevInfo
    {
    char    name[AUDIO_DEV_NAME_LEN + 1];
    UINT32  availPaths;     /* available audio paths */
    UINT32  defPaths;       /* default audio in and out paths */
    UINT8   bytesPerSample; /* size of sample in device buffer */
    UINT8   maxChannels;    /* device maximum channels */
    BOOL    useMsb;         /* device data is MSB first */
    } AUDIO_DEV_INFO;

typedef union audioIoctlArg
    {
    AUDIO_VOLUME    volume;
    AUDIO_DEV_INFO  devInfo;
    AUDIO_DATA_INFO dataInfo;
    UINT32          path;
    UINT32          bufTime;            /* buffer time (ms) */
    ssize_t         bufSize;
    } AUDIO_IOCTL_ARG;

#ifdef _WRS_KERNEL
typedef struct audioDev
    {
    DL_NODE         node;
    AUDIO_DEV_INFO  devInfo;
    UINT32          unit;
    UINT32          attTransUnit;   /* the transceiver which codec device attached to */
    void *          extension;      /* optional driver extensions */
    FUNCPTR         open;
    FUNCPTR         close;
    FUNCPTR         read;
    FUNCPTR         write;
    FUNCPTR         ioctl;
    } AUDIO_DEV;
#endif /* _WRS_KERNEL */

/* ioctl commands */

#ifndef VX_IOCG_AUDIO
#define VX_IOCG_AUDIO 'A'
#endif

#define AUDIO_FIONREAD      _IOW(VX_IOCG_AUDIO,   0, union audioIoctlArg)
#define AUDIO_FIONFREE      _IOW(VX_IOCG_AUDIO,   1, union audioIoctlArg)
#define AUDIO_DEV_ENABLE    _IO(VX_IOCG_AUDIO,    2)
#define AUDIO_DEV_DISABLE   _IO(VX_IOCG_AUDIO,    3)
#define AUDIO_START         _IO(VX_IOCG_AUDIO,    4)
#define AUDIO_STOP          _IO(VX_IOCG_AUDIO,    5)
#define AUDIO_SET_DATA_INFO _IOR(VX_IOCG_AUDIO,   6, union audioIoctlArg)
#define AUDIO_SET_VOLUME    _IOR(VX_IOCG_AUDIO,   7, union audioIoctlArg)
#define AUDIO_SET_BUFTIME   _IOR(VX_IOCG_AUDIO,   8, union audioIoctlArg)
#define AUDIO_SET_PATH      _IOR(VX_IOCG_AUDIO,   9, union audioIoctlArg)
#define AUDIO_GET_DEV_INFO  _IOW(VX_IOCG_AUDIO,  10, union audioIoctlArg)
#define AUDIO_GET_DATA_INFO _IOW(VX_IOCG_AUDIO,  11, union audioIoctlArg)
#define AUDIO_GET_VOLUME    _IOWR(VX_IOCG_AUDIO, 12, union audioIoctlArg)
#define AUDIO_GET_BUFTIME   _IOW(VX_IOCG_AUDIO,  13, union audioIoctlArg)
#define AUDIO_GET_PATH      _IOW(VX_IOCG_AUDIO,  14, union audioIoctlArg)

/* function declarations */

#ifdef _WRS_KERNEL
STATUS  audioCoreRegCodec (AUDIO_DEV * pCodecsInfo);
STATUS  audioCoreUnregCodec (AUDIO_DEV * pCodecsInfo);
STATUS  audioCoreRegTransceiver (AUDIO_DEV * pTransceiverInfo, int codecUnit);
STATUS  audioCoreUnregTransceiver (AUDIO_DEV * pTransceiverInfo);
#endif /* _WRS_KERNEL */

#if __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCaudioLibCoreh */
