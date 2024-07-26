# Makefile - Wind River Audio Layer Makefile
#
# Copyright (c) 2013, 2016, 2018, 2021, 2023 Wind River Systems, Inc.
#
# The right to copy, distribute, modify or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#
include $(WIND_KRNL_MK)/defs.layers.mk

ifdef _WRS_CONFIG_AUDIO_LIB
ifdef _WRS_CONFIG_AUDIO_DEMO
BUILD_DIRS += demo/playback
BUILD_DIRS += demo/record
ifdef _WRS_CONFIG_RTP
POSTBUILD_RTP_DIRS += demo/playback
POSTBUILD_RTP_DIRS += demo/record
endif
POST_NOBUILD_CDFDIRS += demo/cdf
endif

ifdef _WRS_CONFIG_AUDIO_DRV
POST_NOBUILD_CDFDIRS += drv/cdf
endif

ifdef _WRS_CONFIG_AUDIO_FSL_SGTL5000
BUILD_DIRS += drv/fslsgtl5000/src
POST_NOBUILD_CDFDIRS += drv/fslsgtl5000/cdf
endif

ifdef _WRS_CONFIG_AUDIO_TI_MCASP
BUILD_DIRS += drv/timcasp/src
POST_NOBUILD_CDFDIRS += drv/timcasp/cdf
endif

ifdef _WRS_CONFIG_AUDIO_TI_AIC3106
BUILD_DIRS += drv/tiaic3106/src
POST_NOBUILD_CDFDIRS += drv/tiaic3106/cdf
endif

ifdef _WRS_CONFIG_AUDIO_WM8962
BUILD_DIRS += drv/wm8962/src
POST_NOBUILD_CDFDIRS += drv/wm8962/cdf
endif

ifdef _WRS_CONFIG_AUDIO_VIRTUAL_AUDIO
BUILD_DIRS += drv/virtualaudio/src
POST_NOBUILD_CDFDIRS += drv/virtualaudio/cdf
endif

ifdef _WRS_CONFIG_AUDIO_FSL_SSI
BUILD_DIRS += drv/fslssi/src
POST_NOBUILD_CDFDIRS += drv/fslssi/cdf
endif

SHARED_PUBLIC_H_DIRS += lib/h
BUILD_DIRS += lib/src
ifdef _WRS_CONFIG_RTP
BUILD_USER_DIRS += lib/src
endif
POST_NOBUILD_CDFDIRS += lib/cdf


ifneq ("$(wildcard lib/vxTest/src/Makefile)","")
ifdef _WRS_CONFIG_AUDIO_LIB_VXTEST
BUILD_DIRS += lib/vxTest/src
POST_NOBUILD_CDFDIRS += lib/vxTest/cdf
endif
endif

else # _WRS_CONFIG_VX_SOUND_CORE

SHARED_PUBLIC_H_DIRS += lib/h
BUILD_DIRS += lib/src
ifdef _WRS_CONFIG_RTP
BUILD_USER_DIRS += lib/src
endif
POST_NOBUILD_CDFDIRS += lib/cdf

ifdef _WRS_CONFIG_AUDIO_DEMO
BUILD_DIRS += demo/playback
BUILD_DIRS += demo/record
BUILD_DIRS += demo/control
#ifdef _WRS_CONFIG_RTP
#POSTBUILD_RTP_DIRS += demo/playback
#POSTBUILD_RTP_DIRS += demo/record
#endif
POST_NOBUILD_CDFDIRS += demo/cdf
endif

ifdef _WRS_CONFIG_AUDIO_DRV
POST_NOBUILD_CDFDIRS += drv/cdf

BUILD_DIRS += drv/codecs/src
POST_NOBUILD_CDFDIRS += drv/codecs/cdf
endif

ifdef _WRS_CONFIG_SOUND_SAMSUNG_ABOX
BUILD_DIRS += drv/abox/src
POST_NOBUILD_CDFDIRS += drv/abox/cdf
endif

ifneq ("$(wildcard lib/vxTest/src/Makefile)","")
ifdef _WRS_CONFIG_AUDIO_LIB_VXTEST
BUILD_DIRS += lib/vxTest/src
POST_NOBUILD_CDFDIRS += lib/vxTest/cdf
endif
endif

endif

POST_NOBUILD_CDFDIRS += cdf

include $(WIND_KRNL_MK)/rules.layers.mk

