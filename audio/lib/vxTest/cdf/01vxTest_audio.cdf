/* 01vxTest_audio.cdf - VxTest AUDIO components groups */

/*
 * Copyright (c) 2015-2016, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION

This file contains the definition of the components for OS vxTest components
groups.Do not add test module components or parameters to this file.

*/

Component       INCLUDE_TM_AUDIO_LIB_TEST {
        NAME            UI AUDIO Test components group
        SYNOPSIS        This component adds all AUDIO components
        REQUIRES        INCLUDE_TM_NEW_AUDIO
}

Component       INCLUDE_TM_AUDIO {
        NAME            Audio Test Module
        SYNOPSIS        This component adds the audio stand IO Test Module
        REQUIRES        INCLUDE_VXTEST_DRIVER \
                        INCLUDE_RAM_DISK \
                        INCLUDE_AUDIO_LIB_CORE \
                        INCLUDE_AUDIO_LIB_WAV
        MODULES         tmAudio.o
        INCLUDE_WHEN    INCLUDE_TM_AUDIO_LIB_TEST
        PROTOTYPE       void atInit();
        INIT_RTN        atInit();
        LINK_SYMS       atInit
}

Component       INCLUDE_TM_NEW_AUDIO {
        NAME            Audio Test for ABOX Module
        SYNOPSIS        This component adds the audio abox Test Module
        REQUIRES        INCLUDE_VXTEST_DRIVER \
                        INCLUDE_RAM_DISK \
                        INCLUDE_VX_SOUND_CORE \
                        INCLUDE_AUDIO_LIB_WAV
        MODULES         tmAudioNew.o
        INCLUDE_WHEN    INCLUDE_TM_AUDIO_LIB_TEST
        PROTOTYPE       void audiTestInit();
        INIT_RTN        audiTestInit();
        LINK_SYMS       audiTestInit
}

Component       INCLUDE_TM_NEW_AUDIO_LIB {
        NAME            Audio Test for VxWorks Sound Core Module
        SYNOPSIS        This component adds the vxSoundCore Test Module
        REQUIRES        INCLUDE_VXTEST_DRIVER \
                        INCLUDE_RAM_DISK \
                        INCLUDE_VX_SOUND_CORE \
                        INCLUDE_AUDIO_LIB_WAV
        MODULES         tmAudioNewLib.o
        INCLUDE_WHEN    INCLUDE_TM_AUDIO_LIB_TEST
        PROTOTYPE       void audioLibTestInit();
        INIT_RTN        audioLibTestInit();
        LINK_SYMS       audioLibTestInit
}

InitGroup        usrVxTestAudioInit {
    INIT_RTN        usrVxTestAudioInit ();
    SYNOPSIS        VxTest AUDIO tests initialization sequence
    INIT_ORDER      INCLUDE_TM_NEW_AUDIO
    _INIT_ORDER     usrVxTestInit
}

/*
 * Tests Folder
 */
Folder        FOLDER_VXTEST_UI_AUDIO_LIB {
    NAME            VxTest audio lib test components
    SYNOPSIS        Used to group audio lib test components
    CHILDREN        INCLUDE_TM_NEW_AUDIO
    DEFAULTS        INCLUDE_TM_NEW_AUDIO
    _CHILDREN       FOLDER_VXTEST_UI_TESTS
}

