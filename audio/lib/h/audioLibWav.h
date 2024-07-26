/* audioLibWav.h - wave format audio support header file */

/*
 * Copyright (c) 2014-2015, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */ 

#ifndef __INCaudioLibWaveh
#define __INCaudioLibWaveh

#if __cplusplus
extern "C" {
#endif

/* function declarations */

STATUS  audioWavHeaderRead (int fd, int * pChannels, UINT32 * pSampleRate, int *
                            pSampleBits, UINT32 * pSamples, UINT32 *
                            pDataStart);
STATUS  audioWavHeaderWrite (int fd, UINT8 nChannels, UINT32 nSampleRate, UINT8
                             nSampleBits, UINT32 nSamples);
STATUS  audioWavHeaderReadBuf (char * pBuf, size_t bufSize, int * pChannels, 
                               UINT32 * pSampleRate, int * pSampleBits, 
                               UINT32 * pSamples, UINT32 * pDataStart);
STATUS  audioWavHeaderWriteBuf (char * pBuf, size_t bufSize, UINT8 nChannels,
                                UINT32 nSampleRate, UINT8 nSampleBits, 
                                UINT32 nSamples);
#if __cplusplus
}
#endif /* __cplusplus  */

#endif  /* __INCaudioLibWaveh */
