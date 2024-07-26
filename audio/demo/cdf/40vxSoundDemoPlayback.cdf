/* 40vxSoundDemoPlayback.cdf - ABOX Driver Framework Playback Demo */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use of this software
 * may be licensed only pursuant to the terms of an applicable Wind River
 * license agreement.
 */

Component INCLUDE_VX_SOUND_DEMO_PLAYBACK
    {
    NAME        VxSound Framework Playback Demo
    SYNOPSIS    VxSound Framework Playback Demo
    _CHILDREN   FOLDER_AUDIO_DEMO
    REQUIRES    INCLUDE_VX_SOUND_CORE \
                INCLUDE_VX_SOUND_DEMO_CONTROL
    LINK_SYMS   vxSndPlayDemo
    }