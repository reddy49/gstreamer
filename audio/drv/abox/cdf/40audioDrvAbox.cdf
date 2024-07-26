/* 40audioDrvAbox.cdf - Samsung ExynosAuto V920 Abox */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

Component DRV_ABOX_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX Driver
    MODULES     vxbFdtAboxCtr.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   aboxCtrDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                INCLUDE_VX_SOUND_CORE       \
                DRV_ABOX_GIC_FDT_EXYNOS     \
                DRV_ABOX_MAILBOX_FDT_EXYNOS
    }

Component DRV_ABOX_SADK_MACHINE_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 Audio Machine Driver
    SYNOPSIS    Samsung ExynosAuto V920 Audio Mahcine Driver
    MODULES     vxbFdtAboxMachine.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   aboxMachineDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS         \
                DRV_ABOX_PCM_PLAYBACK_FDT_EXYNOS \
                DRV_ABOX_PCM_CAPTURE_FDT_EXYNOS \
                DRV_ABOX_RDMA_FDT_EXYNOS \
                DRV_ABOX_WDMA_FDT_EXYNOS \
                DRV_ABOX_UAIF_FDT_EXYNOS \
                DRV_TAS6424_FDT_EXYNOS \
                DRV_TCA6416_TAS6424_FDT_EXYNOS \
                DRV_TLV320ADCX140_FDT_EXYNOS
    }

Component DRV_ABOX_GIC_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX GIC Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX GIC Driver
    MODULES     vxbFdtAboxGic.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   aboxGicDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT
    }

Component DRV_ABOX_MAILBOX_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX Mailbox Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX Mailbox Driver
    MODULES     vxbFdtAboxMailbox.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   aboxMailboxDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT
    }

Component DRV_ABOX_PCM_PLAYBACK_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX PCM Playback Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX PCM Playback Driver
    MODULES     vxbFdtAboxPcmPlayback.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxbFdtAboxPcmPlaybackDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS
    }

Component DRV_ABOX_PCM_CAPTURE_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX PCM Capture Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX PCM Capture Driver
    MODULES     vxbFdtAboxPcmCapture.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxbFdtAboxPcmCaptureDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS
    }

Component DRV_ABOX_RDMA_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX RDMA Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX RDMA Driver
    MODULES     vxbFdtAboxRdma.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxbFdtAboxRdmaDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS
    }

Component DRV_ABOX_WDMA_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX WDMA Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX WDMA Driver
    MODULES     vxbFdtAboxWdma.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxbFdtAboxWdmaDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS
    }

Component DRV_ABOX_UAIF_FDT_EXYNOS
    {
    NAME        Samsung ExynosAuto V920 ABOX UAIF Driver
    SYNOPSIS    Samsung ExynosAuto V920 ABOX UAIF Driver
    MODULES     vxbFdtAboxUaif.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   aboxUaifDrv
    REQUIRES    INCLUDE_VXBUS               \
                DRV_BUS_FDT_ROOT            \
                DRV_ABOX_FDT_EXYNOS
    }

Component INCLUDE_ABOX_SHOW
    {
    NAME        Samsung ExynosAuto V920 ABOX Show
    SYNOPSIS    Samsung ExynosAuto V920 ABOX Debug Show Driver
    MODULES     aboxDebug.o
    _CHILDREN   FOLDER_AUDIO_DRV
    LINK_SYMS   vxAboxUaifShow
    REQUIRES    INCLUDE_VXBUS               \
                DRV_ABOX_FDT_EXYNOS
    }
