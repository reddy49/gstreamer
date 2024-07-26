# Makefile - Audio Codec Driver Makefile
#
# Copyright (c) 2023 Wind River Systems, Inc.
#
# The right to copy, distribute, modify or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#

C_SOURCE = vxbFdtTas6424.c \
           vxbFdtTca6416aTas6424.c \
           vxbFdtTlv320adcx140.c

LIB_BASE_NAME = audioDrvExynosAbox

OBJS += $(C_SOURCE:.c=.o)

include $(WIND_KRNL_MK)/rules.library.mk
