/* 40audioLibCore.cdf - Core Library Component Bundles */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
#if 0
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
#endif
Component   INCLUDE_VX_SOUND_CORE
    {
    NAME        VxWorks Sound Core library
    SYNOPSIS    Provides an sound core framework that manages all audio devices.
    REQUIRES    INCLUDE_VXB_TIMESTAMP
    MODULES     vxSoundCore.o
    _CHILDREN   FOLDER_AUDIO_LIB
    PROTOTYPE   void vxSoundCoreInit (void);
    _INIT_ORDER usrIosExtraInit
    INIT_BEFORE INCLUDE_FORMATTED_IO
    INIT_RTN    vxSoundCoreInit ();
//    CFG_PARAMS  AUDIO_LIB_CORE_BUFFER_NUM   \
 //               AUDIO_LIB_CORE_BUFFER_TIME
    }

