/* 40audioDrvFslSgtl5000.cdf - Freescale SGTL5000 Audio Codec */

/*
 * Copyright (c) 2014, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River License agreement.
 */

Component   DRV_AUDIO_FSL_SGTL5000
    {
    NAME        Freescale SGTL5000 Audio Codec Driver
    SYNOPSIS    Freescale SGTL5000 audio codec driver
    MODULES     audioDrvFslSgtl5000.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxbFdtSgtl5000AudDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_AUDIO_LIB_CORE
    }