/* 40audioDrvCodec.cdf - Audio Codec Driver */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

Component DRV_TAS6424_FDT_EXYNOS
    {
    NAME        TI Tas6424 Codec Driver
    SYNOPSIS    TI Digital Input 4-Channel Automotive Class-D Audio Amplifier
    MODULES     vxbFdtTas6424.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   tas6424Drv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_VX_SOUND_CORE
    }

Component DRV_TCA6416_TAS6424_FDT_EXYNOS
    {
    NAME        TCA6416 and TAS6424 Hybrid Control Driver
    SYNOPSIS    TCA6416 and TAS6424 Hybrid Control Driver
    MODULES     vxbFdtTca6416aTas6424.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   tca6416aDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_VX_SOUND_CORE
    }

Component DRV_TLV320ADCX140_FDT_EXYNOS
    {
    NAME        TI TLV320ADCX140 Codec Driver
    SYNOPSIS    TI Analog to Digital Converter
    MODULES     vxbFdtTlv320adcx140.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   tlv320adcx140Drv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_VX_SOUND_CORE
    }