/* 40audioDemoPlayback.cdf - Audio Driver Framework Playback Demo */

/*
 * Copyright (c) 2014, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use of this software
 * may be licensed only pursuant to the terms of an applicable Wind River
 * license agreement.
 */

Component INCLUDE_VX_SOUND_DEMO_CONTROL
    {
    NAME        VxSound Framework Control Demo
    SYNOPSIS    VxSound Framework Control Demo
    _CHILDREN   FOLDER_AUDIO_DEMO
    REQUIRES    INCLUDE_VX_SOUND_CORE
    LINK_SYMS   amixer
    }