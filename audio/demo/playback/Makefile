# Makefile - Audio Driver Framework Playback Demo Makefile
#
# Copyright (c) 2014, 2021 Wind River Systems, Inc.
#
# The right to copy, distribute, modify or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#

include $(WIND_KRNL_MK)/defs.layers.mk

ifdef _WRS_CONFIG_VX_SOUND_CORE
ifeq ($(SPACE), user)
EXE = vxSoundDemoPlayback.vxe
ADDED_LIBS += -lvxSoundLib
else
LIB_BASE_NAME = vxSoundDemoPlayback
endif

OBJS = vxSoundDemoPlayback.o
else
ifeq ($(SPACE), user)
EXE = audioDemoPlayback.vxe
ADDED_LIBS += -laudioLib
else
LIB_BASE_NAME = audioDemoPlayback
endif

OBJS = audioDemoPlayback.o
endif

ifeq ($(SPACE), user)
include $(WIND_USR_MK)/rules.rtp.mk
else
include $(WIND_KRNL_MK)/rules.library.mk
endif