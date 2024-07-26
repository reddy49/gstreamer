# Makefile - Audio ABOX Driver Makefile
#

C_SOURCE = vxbFdtAboxCtr.c \
           vxbFdtAboxGic.c \
           vxbFdtAboxMachine.c \
           vxbFdtAboxMailbox.c \
           vxbFdtAboxPcmCapture.c \
           vxbFdtAboxPcmPlayback.c \
           vxbFdtAboxRdma.c \
           vxbFdtAboxWdma.c \
           vxbFdtAboxUaif.c \
           aboxUtil.c \
           aboxDebug.c


LIB_BASE_NAME = audioDrvExynosAbox

OBJS += $(C_SOURCE:.c=.o)

include $(WIND_KRNL_MK)/rules.library.mk
