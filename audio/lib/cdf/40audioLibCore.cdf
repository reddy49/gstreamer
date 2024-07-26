/* 40audioLibCore.cdf - Core Library Component Bundles */

/*
 * Copyright (c) 2013, 2021-2022 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

Parameter AUDIO_LIB_CORE_BUFFER_NUM
    {
    NAME        Buffer number
	SYNOPSIS	Specifies the number of buffers for buffering the PCM data.
    TYPE        uint
    DEFAULT     2
    }

Parameter AUDIO_LIB_CORE_BUFFER_TIME
    {
    NAME        Buffer time
	SYNOPSIS	Specifies the time (in millisecond) for buffering the PCM data.
    TYPE        uint
    DEFAULT     2000
    }

Component   INCLUDE_AUDIO_LIB_CORE
    {
    NAME        Audio driver framework core library
    SYNOPSIS    Provides an audio driver framework that manages all audio devices.
    MODULES     audioLibCore.o
    _CHILDREN   FOLDER_AUDIO_LIB
    PROTOTYPE   void audioCoreInit (UINT32 bufNum, UINT32 bufTime);
    _INIT_ORDER usrIosExtraInit
    INIT_BEFORE INCLUDE_FORMATTED_IO
    INIT_RTN    audioCoreInit (AUDIO_LIB_CORE_BUFFER_NUM, AUDIO_LIB_CORE_BUFFER_TIME);
    CFG_PARAMS  AUDIO_LIB_CORE_BUFFER_NUM   \
                AUDIO_LIB_CORE_BUFFER_TIME
    }

