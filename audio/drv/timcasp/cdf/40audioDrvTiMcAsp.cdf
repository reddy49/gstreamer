/* 40audioDrvTiMcAsp.cdf - TI McASP Audio Driver */

/*
 * Copyright (c) 2014, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River License agreement.
 */

Component   DRV_AUDIO_TI_MCASP
    {
    NAME        TI McASP Audio Driver
    SYNOPSIS    TI McASP audio driver
    MODULES     audioDrvTiMcAsp.o
    LINK_SYMS   vxbFdtTiMcAspAudDrv
    _CHILDREN   FOLDER_AUDIO_DRV
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_DMA_SYS             \
                INCLUDE_AUDIO_LIB_CORE
    }