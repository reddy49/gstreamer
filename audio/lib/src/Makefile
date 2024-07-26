# Makefile - Audio Libraries src Makefile
#

ifeq ($(SPACE), user)
C_SOURCE = audioLibWav.c
else
ifdef _WRS_CONFIG_AUDIO_LIB
C_SOURCE = audioLibCore.c audioLibWav.c
LIB_BASE_NAME = audioLib
else
C_SOURCE = vxSoundCore.c pcm.c soc.c control.c audioLibWav.c card.c vxSoundUtils.c
LIB_BASE_NAME = vxSoundLib
endif
endif

OBJS += $(C_SOURCE:.c=.o)
LIB_FORMAT = both

ifeq ($(SPACE), user)
include $(WIND_USR_MK)/rules.library.mk
else
include $(WIND_KRNL_MK)/rules.library.mk
endif