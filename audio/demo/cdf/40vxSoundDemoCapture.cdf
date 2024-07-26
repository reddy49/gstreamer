/* 40vxSoundDemoCapture.cdf - ABOX Driver Framework Capture Demo */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use of this software
 * may be licensed only pursuant to the terms of an applicable Wind River
 * license agreement.
 */

Component INCLUDE_VX_SOUND_DEMO_CAPTURE
    {
    NAME        VxSound Framework Capture Demo
    SYNOPSIS    VxSound Framework Capture Demo
    _CHILDREN   FOLDER_AUDIO_DEMO
    REQUIRES    INCLUDE_VX_SOUND_CORE \
                INCLUDE_VX_SOUND_DEMO_CONTROL \
                INCLUDE_VX_SOUND_DEMO_PLAYBACK
    LINK_SYMS   vxSndCapDemo
    }