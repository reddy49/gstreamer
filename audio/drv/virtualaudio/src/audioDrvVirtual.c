/* audioDrvVirtual.c - Virtual Audio Driver */

/*
 * Copyright (c) 2014, 2016, 2019, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This is the virtual audio driver.
*/

/* includes */

#include <audioLibCore.h>

/* defines */

#define AUD_VIR_MSG(fmt, args...)  \
            if (audVirObserveEn) printf(fmt, ##args)

/* forward declarations */

LOCAL int       audVirOpen (void * pDev);
LOCAL int       audVirClose (void * pDev);
LOCAL int       audVirRead (void * pDev, char * pBuffer, int nbytes);
LOCAL int       audVirWrite (void * pDev, char * pBuffer, int nbytes);
LOCAL STATUS    audVirIoctl (void * pDev, AUDIO_IO_CTRL function,
                             AUDIO_IOCTL_ARG * pIoctlArg);

/* locals */

LOCAL AUDIO_DEV audVirTrans;
LOCAL BOOL      audVirObserveEn;

/* functions */

/*******************************************************************************
*
* audVirInit - initialize the virtual audio device
*
* This routine initializes the virtual audio device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void audVirInit
    (
    BOOL    observeEn,
    UINT8   maxChannels
    )
    {
    audVirObserveEn = observeEn;

    bzero ((char *)&audVirTrans, sizeof (AUDIO_DEV));
    snprintf ((char *)audVirTrans.devInfo.name, AUDIO_DEV_NAME_LEN,
              "virtual audio");

    audVirTrans.unit                    = 0;
    audVirTrans.open                    = audVirOpen;
    audVirTrans.close                   = audVirClose;
    audVirTrans.ioctl                   = audVirIoctl;
    audVirTrans.read                    = audVirRead;
    audVirTrans.write                   = audVirWrite;
    audVirTrans.devInfo.bytesPerSample  = sizeof (UINT32);
    audVirTrans.devInfo.maxChannels     = maxChannels;
    audVirTrans.devInfo.useMsb          = TRUE;
    audVirTrans.devInfo.availPaths      = AUDIO_MIC_IN | AUDIO_HP_OUT;
    audVirTrans.devInfo.defPaths        = AUDIO_MIC_IN | AUDIO_HP_OUT;

    if (ERROR == audioCoreRegTransceiver (&audVirTrans, -1))
        {
        AUD_VIR_MSG ("ERROR: Register virtual audio failed!\n");
        }
    }

/*******************************************************************************
*
* audVirOpen - open virtual audio device
*
* This routine opens virtual audio device.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL int audVirOpen
    (
    void *  pDev
    )
    {
    AUD_VIR_MSG ("Virtual audio device opened.\n");
    return OK;
    }

/*******************************************************************************
*
* audVirClose - close virtual audio device
*
* This routine closes virtual audio device.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL int audVirClose
    (
    void *  pDev
    )
    {
    AUD_VIR_MSG ("Virtual audio device closed.\n");
    return OK;
    }

/*******************************************************************************
*
* audVirRead - read audio data
*
* This routine reads audio data.
*
* RETURNS: number of bytes
*
* ERRNO: N/A
*/

LOCAL int audVirRead
    (
    void *  pDev,
    char *  pBuffer,    /* location of data buffer */
    int     nbytes      /* number of bytes to output */
    )
    {
    AUD_VIR_MSG ("Read : Data buffer address = 0x%p, bytes = %d\n", pBuffer,
                 nbytes);
    return nbytes;
    }

/*******************************************************************************
*
* audVirWrite - write audio data
*
* This routine writes audio data.
*
* RETURNS: number of bytes
*
* ERRNO: N/A
*/

LOCAL int audVirWrite
    (
    void *  pDev,
    char *  pBuffer,    /* location of data buffer */
    int     nbytes      /* number of bytes to output */
    )
    {
    AUD_VIR_MSG ("Write : Data buffer address = 0x%p, bytes = %d\n", pBuffer,
                 nbytes);
    return nbytes;
    }

/*******************************************************************************
*
* audVirIoctl - handle ioctls for virtual audio device
*
* This routine handles ioctls for virtual audio device.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS audVirIoctl
    (
    void *              pDev,
    AUDIO_IO_CTRL       function,   /* function to perform */
    AUDIO_IOCTL_ARG *   pIoctlArg   /* function argument */
    )
    {
    switch (function)
        {
        case AUDIO_SET_DATA_INFO:
            AUD_VIR_MSG ("Set data info:\n");
            AUD_VIR_MSG ("Sample Rate: %d\n", pIoctlArg->dataInfo.sampleRate);
            AUD_VIR_MSG ("Sample Bits: %d\n", pIoctlArg->dataInfo.sampleBits);
            AUD_VIR_MSG ("Sample Bytes: %d\n", pIoctlArg->dataInfo.sampleBytes);
            AUD_VIR_MSG ("Channels: %d\n", pIoctlArg->dataInfo.channels);
            AUD_VIR_MSG ("Use MSB: %d\n", pIoctlArg->dataInfo.useMsb);
            break;

        case AUDIO_SET_PATH:
            AUD_VIR_MSG ("Set Path: 0x%x\n", pIoctlArg->path);
            break;

        case AUDIO_SET_VOLUME:
            AUD_VIR_MSG ("Set volume: path = 0x%x, left = %d, right = %d\n",
                         pIoctlArg->volume.path, pIoctlArg->volume.left,
                         pIoctlArg->volume.right);
            break;

        case AUDIO_DEV_ENABLE:
            AUD_VIR_MSG ("Device enable\n");
            break;

        case AUDIO_DEV_DISABLE:
            AUD_VIR_MSG ("Device disable\n");
            break;

        default:
            break;
        }

    return OK;
    }
