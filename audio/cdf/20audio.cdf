/* 20audio.cdf - Audio Component Bundles */

/*
 * Copyright (c) 2013, 2018, 2021 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River License agreement.
 */

Folder FOLDER_MULTIMEDIA
    {
    NAME        Multimedia
    SYNOPSIS    This folder includes components and parameters used to \
                configure the multimedia.
    _CHILDREN   FOLDER_ROOT
    }

Folder FOLDER_AUDIO
    {
    NAME        Audio Components
    SYNOPSIS    Audio driver framework and drivers
    _CHILDREN   FOLDER_MULTIMEDIA
    }
