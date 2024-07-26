# Makefile - VxSound Framework Control Demo Makefile
#
# Copyright (c) 2023 Wind River Systems, Inc.
#
# The right to copy, distribute, modify or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#


ifeq ($(SPACE), user)
EXE = vxSoundDemoControl.vxe
ADDED_LIBS += -lvxSoundLib
else
LIB_BASE_NAME = vxSoundDemoControl
endif

OBJS = vxSoundDemoControl.o

ifeq ($(SPACE), user)
include $(WIND_USR_MK)/rules.rtp.mk
else
include $(WIND_KRNL_MK)/rules.library.mk
endif