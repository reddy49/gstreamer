/* pcm.h - PCM Header File */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file defines the PCM functions and structures for the PCM module
*/

#ifndef __INCsoundPcmh
#define __INCsoundPcmh

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
#include <ffsLib.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus  */

/* defines */

/* ioctl commands */

#ifndef VX_IOCG_AUDIO_PCM
#define VX_IOCG_AUDIO_PCM 'P'
#endif

#define VX_SND_CMD_PCM_CORE_VER _IOR(VX_IOCG_AUDIO_PCM,  0x00, int)
#define VX_SND_CMD_PCM_APP_VER  _IOW(VX_IOCG_AUDIO_PCM,  0x01, int)
#define VX_SND_CMD_HW_REFINE    _IOWR(VX_IOCG_AUDIO_PCM, 0x02,      \
                                      VX_SND_PCM_HW_PARAMS)
#define VX_SND_CMD_HW_PARAMS    _IOWR(VX_IOCG_AUDIO_PCM, 0x03,      \
                                      VX_SND_PCM_HW_PARAMS)
#define VX_SND_CMD_HW_FREE      _IO(VX_IOCG_AUDIO_PCM,   0x04)
#define VX_SND_CMD_SW_PARAMS    _IOWR(VX_IOCG_AUDIO_PCM, 0x05,      \
                                      VX_SND_PCM_SW_PARAMS)
#define VX_SND_CMD_STATUS       _IOR(VX_IOCG_AUDIO_PCM,  0x06,      \
                                     VX_SND_SUBSTREAM_STATUS)
#define VX_SND_CMD_DELAY        _IOR(VX_IOCG_AUDIO_PCM,  0x07, SND_FRAMES_T)
#define VX_SND_CMD_HWSYNC       _IO(VX_IOCG_AUDIO_PCM,   0x08)
#define VX_SND_CMD_SYNC_PTR64   _IOWR(VX_IOCG_AUDIO_PCM, 0x09,      \
                                      VX_SND_SUBSTREAM_SYNC_PTR)
#define VX_SND_CMD_PREPARE      _IO(VX_IOCG_AUDIO_PCM,   0x0a)
#define VX_SND_CMD_RESET        _IO(VX_IOCG_AUDIO_PCM,   0x0b)
#define VX_SND_CMD_START        _IO(VX_IOCG_AUDIO_PCM,   0x0c)
#define VX_SND_CMD_DROP         _IO(VX_IOCG_AUDIO_PCM,   0x0d)
#define VX_SND_CMD_DRAIN        _IO(VX_IOCG_AUDIO_PCM,   0x0e)
#define VX_SND_CMD_PAUSE        _IOW(VX_IOCG_AUDIO_PCM,  0x0f, int)
#define VX_SND_CMD_REWIND       _IOW(VX_IOCG_AUDIO_PCM,  0x10, SND_FRAMES_T)
#define VX_SND_CMD_XRUN         _IO(VX_IOCG_AUDIO_PCM,   0x11)
#define VX_SND_CMD_FORWARD      _IOW(VX_IOCG_AUDIO_PCM,  0x12, SND_FRAMES_T)

#ifdef _WRS_KERNEL

#define VX_SND_PCM_CORE_VERSION  VX_SND_MODULE_VERSION(0, 0, 1)

// inline snd_pcm_access_t params_access(const struct snd_pcm_hw_params *p)
#define PARAMS_ACCESS(p) (ffs32Lsb((p)->accessTypeBits) - 1)
// inline snd_pcm_format_t params_format(const struct snd_pcm_hw_params *p)
#define PARAMS_FORMAT(p) (ffs64Lsb((p)->formatBits) - 1)

#define PARAMS_CHANNELS(p)     (p)->intervals[VX_SND_HW_PARAM_CHANNELS].min
//#define PARAMS_RATE(p)         (p)->intervals[VX_SND_HW_PARAM_RATE].min
#define PARAMS_PERIOD_BYTES(p) (p)->intervals[VX_SND_HW_PARAM_PERIOD_BYTES].min
#define PARAMS_PERIODS(p)      (p)->intervals[VX_SND_HW_PARAM_PERIODS].min
//#define PARAMS_BUFFER_TIME(p)  (p)->intervals[VX_SND_HW_PARAM_BUFFER_TIME].max
//#define PARAMS_BUFFER_FRAMES(p) \
//                             (p)->intervals[VX_SND_HW_PARAM_BUFFER_FRAMES].max
#define PARAMS_BUFFER_BYTES(p) (p)->intervals[VX_SND_HW_PARAM_BUFFER_BYTES].min


/* If you change this don't forget to change rates[] table in pcm_native.c */
#define SNDRV_PCM_RATE_5512         (1<<0)      /* 5512Hz */
#define SNDRV_PCM_RATE_8000         (1<<1)      /* 8000Hz */
#define SNDRV_PCM_RATE_11025        (1<<2)      /* 11025Hz */
#define SNDRV_PCM_RATE_16000        (1<<3)      /* 16000Hz */
#define SNDRV_PCM_RATE_22050        (1<<4)      /* 22050Hz */
#define SNDRV_PCM_RATE_32000        (1<<5)      /* 32000Hz */
#define SNDRV_PCM_RATE_44100        (1<<6)      /* 44100Hz */
#define SNDRV_PCM_RATE_48000        (1<<7)      /* 48000Hz */
#define SNDRV_PCM_RATE_64000        (1<<8)      /* 64000Hz */
#define SNDRV_PCM_RATE_88200        (1<<9)      /* 88200Hz */
#define SNDRV_PCM_RATE_96000        (1<<10)     /* 96000Hz */
#define SNDRV_PCM_RATE_176400       (1<<11)     /* 176400Hz */
#define SNDRV_PCM_RATE_192000       (1<<12)     /* 192000Hz */
#define SNDRV_PCM_RATE_352800       (1<<13)     /* 352800Hz */
#define SNDRV_PCM_RATE_384000       (1<<14)     /* 384000Hz */

#define SNDRV_PCM_ALL_RATES_MASK    (0x7fff)

#define SNDRV_PCM_RATE_CONTINUOUS   (1<<30)     /* continuous range */
#define SNDRV_PCM_RATE_KNOT         (1<<31)    /* supports more non-continuos rates */

#define SNDRV_PCM_RATE_8000_44100   (SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_11025|\
                     SNDRV_PCM_RATE_16000|SNDRV_PCM_RATE_22050|\
                     SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100)
#define SNDRV_PCM_RATE_8000_48000   (SNDRV_PCM_RATE_8000_44100|SNDRV_PCM_RATE_48000)
#define SNDRV_PCM_RATE_8000_96000   (SNDRV_PCM_RATE_8000_48000|SNDRV_PCM_RATE_64000|\
                     SNDRV_PCM_RATE_88200|SNDRV_PCM_RATE_96000)
#define SNDRV_PCM_RATE_8000_192000  (SNDRV_PCM_RATE_8000_96000|SNDRV_PCM_RATE_176400|\
                     SNDRV_PCM_RATE_192000)
#define SNDRV_PCM_RATE_8000_384000  (SNDRV_PCM_RATE_8000_192000|\
                     SNDRV_PCM_RATE_352800|\
                     SNDRV_PCM_RATE_384000)

typedef struct vxSndPcmFormatInfo
    {
    uint8_t validWidth;/* valid data bit width */
    uint8_t phyWidth;      /* physical bit width   */
    int8_t  littleEndian;   /* 0 = big endian, 1 = little endian, -1 = others */
    int8_t  signd;          /* 0 = unsigned, 1 = signed, -1 = others */
    uint8_t silence[8];     /* silence data to fill */
    } VX_SND_PCM_FMT_INFO;

/* function declarations */

void sndPcmInit (void);
STATUS sndPcmDevRegister (SND_DEVICE * sndDev);
STATUS sndPcmDevUnregister (SND_DEVICE * sndDev);
void snd_pcm_period_elapsed (SND_PCM_SUBSTREAM * substream);
STATUS snd_pcm_lib_malloc_pages (SND_PCM_SUBSTREAM * substream,
                                 uint32_t size);
STATUS snd_pcm_lib_free_pages (SND_PCM_SUBSTREAM * substream);
STATUS sndPcmDefaultIoctl (SND_PCM_SUBSTREAM * substream, int cmd, void * arg);
STATUS vxSndPcmCreate (SND_CARD * card, char * name, int devNum,
                       BOOL hasPlayback, BOOL hasCapture, BOOL internal,
                       VX_SND_PCM ** ppPcm);
uint32_t vxSndPcmFmtPhyWidthGet (uint32_t formatIdx);
uint32_t vxSndPcmFmtValidWidthGet(uint32_t formatIdx);
STATUS vxSndPcmHwRateToBitmap (VX_SND_PCM_HARDWARE * hw);
uint32_t vxSndGetRateFromBitmap (uint32_t rates);

#endif

#define PARAMS_WIDTH(p)  vxSndPcmFmtValidWidthGet(PARAMS_FORMAT(p))
#define PARAMS_RATE(p)   vxSndGetRateFromBitmap((p)->rates)

#if __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCsoundPcmh */
