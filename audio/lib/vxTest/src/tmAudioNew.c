/* tmAdioNew.c - Audio Test for ALSA */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file provides testing case.

\cs
<module>
    <component>        INCLUDE_TM_NEW_AUDIO  </component>
    <minVxWorksVer>    7.0                  </minVxWorksVer>
    <maxVxWorksVer>    .*                   </maxVxWorksVer>
    <arch>             .*                         </arch>
    <cpu>              .*                          </cpu>
    <bsp>                                          </bsp>
</module>
\ce

*/


/* includes */

#include <vxTest.h>
#include <audioLibWav.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <dllLib.h>
#include <vxSoundCore.h>
#include <control.h>
#include <pcm.h>
#include <soc.h>
#include "tmAudioNew.h"
#include <dirent.h>


#ifdef _WRS_KERNEL
#include <unistd.h>
#endif /* _WRS_KERNEL */

#define AUDIO_TEST_PCM_C_NAME                "/dev/snd/pcmC0D16c"
#define AUDIO_TEST_PCM_P_NAME                "/dev/snd/pcmC0D0p"
#define AUTIO_TEST_CONTROL_NAME             "/dev/snd/controlC0"

SND_CONTROL_DEV * g_testCtrl = NULL;
LOCAL unsigned char g_buffer[16*1024];   /* some random data */
LOCAL VX_SND_CTRL_LIST g_testctrlList;
LOCAL char * g_testCtrlName[AUD_TEST_CTRL_MAX_NUM] = {NULL};
LOCAL TEST_CTRL_INFO_ALL g_testCtrlInfo;
LOCAL STATUS readFromCapture(TEST_AUDIO_INFO *audioInfo);

UINT32 g_testSndPcmDbgMask = (TEST_SND_AUD_DBG_ERR);

LOCAL TEST_CTRL_CONFIG_INFO g_testCtrlConf[] =
    {
    /* PCM0p -> RDMA0 -> UAIF2 SPK */
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF2 SPK", "SIFS0_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM0p RDMA Route", "RDMA0"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM0p PP Route", "Direct"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA0 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA0 ASRC Route", "SIFS"),

    TEST_CTRL_CONFIG_INT("Speaker Driver CH1 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("Speaker Driver CH2 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("Speaker Driver CH3 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("Speaker Driver CH4 Playback Volume", 200),

    /* PCM1p -> RDMA1 -> UAIF3 SPK */
    TEST_CTRL_CONFIG_ENUM("ABOX PCM1p RDMA Route", "RDMA1"),
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF3 SPK", "SIFS1_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM1p PP Route", "Direct"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA1 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA1 ASRC Route", "SIFS"),

    TEST_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH1 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH2 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH3 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH4 Playback Volume", 200),

    /* PCM2p -> RDMA2 -> UAIF8 SPK */
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF8 SPK", "SIFS2_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM2p RDMA Route", "RDMA2"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM2p PP Route", "Direct"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA2 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA2 ASRC Route", "SIFS"),

    TEST_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH1 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH2 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH3 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH4 Playback Volume", 200),

    /* PCM3p -> RDMA3 -> UAIF9 SPK */
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF9 SPK", "SIFS3_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM3p RDMA Route", "RDMA3"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM3p PP Route", "Direct"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA3 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA3 ASRC Route", "SIFS"),

    TEST_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH1 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH2 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH3 Playback Volume", 200),
    TEST_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH4 Playback Volume", 200),

    /*PCM0c <- WDMA0 <- UAIF5 */
    TEST_CTRL_CONFIG_ENUM("ABOX NSRC0", "UAIF5 MAIN MIC"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM0c WDMA Route", "WDMA0"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM0c PP Route", "Direct"),
    TEST_CTRL_CONFIG_ENUM("ABOX SIFM0", "Reduce Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX WDMA0 Reduce Route", "WDMA"),

    /* other ? */
    TEST_CTRL_CONFIG_ENUM("ABOX PCM4p RDMA Route", "RDMA4"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM5p RDMA Route", "RDMA5"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM6p RDMA Route", "RDMA6"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM7p RDMA Route", "RDMA7"),

    TEST_CTRL_CONFIG_ENUM("ABOX PCM1c WDMA Route", "WDMA1"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM2c WDMA Route", "WDMA2"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM3c WDMA Route", "WDMA3"),
    TEST_CTRL_CONFIG_ENUM("ABOX PCM4c WDMA Route", "WDMA4"),

    TEST_CTRL_CONFIG_ENUM("ABOX UAIF0 SPK", "SIFS4_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF0 2TX SPK", "SIFS5_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF0 3TX SPK", "SIFS6_SET"),
    TEST_CTRL_CONFIG_ENUM("ABOX UAIF0 4TX SPK", "SIFS7_SET"),

    TEST_CTRL_CONFIG_ENUM("ABOX NSRC1", "UAIF1 MAIN MIC"),
    TEST_CTRL_CONFIG_ENUM("ABOX NSRC2", "UAIF1 SEC MIC"),
    TEST_CTRL_CONFIG_ENUM("ABOX NSRC3", "UAIF1 THIRD MIC"),
    TEST_CTRL_CONFIG_ENUM("ABOX NSRC4", "UAIF1 FOURTH MIC"),

    TEST_CTRL_CONFIG_ENUM("ABOX RDMA4 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA4 ASRC Route", "SIFS"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA5 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA5 ASRC Route", "SIFS"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA6 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA6 ASRC Route", "SIFS"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA7 Expand Route", "ASRC Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX RDMA7 ASRC Route", "SIFS"),

    TEST_CTRL_CONFIG_ENUM("ABOX SIFM1", "Reduce Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX WDMA1 Reduce Route", "WDMA"),
    TEST_CTRL_CONFIG_ENUM("ABOX SIFM2", "Reduce Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX WDMA2 Reduce Route", "WDMA"),
    TEST_CTRL_CONFIG_ENUM("ABOX SIFM3", "Reduce Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX WDMA3 Reduce Route", "WDMA"),
    TEST_CTRL_CONFIG_ENUM("ABOX SIFM4", "Reduce Route"),
    TEST_CTRL_CONFIG_ENUM("ABOX WDMA4 Reduce Route", "WDMA"),
    };

VX_SND_INTERVAL g_testInterval[VX_SND_HW_PARAM_IDX_MAX] =
    {
    [VX_SND_HW_PARAM_CHANNELS]=
        {
        .min = 0,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_PERIOD_BYTES]=
        {
        .min = 2560,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_PERIODS]=
        {
        .min = 1,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    [VX_SND_HW_PARAM_BUFFER_BYTES]=
        {
        .min = 1,
        .max = 0xffffffff,
        .openMin = 0,
        .openMax = 0,
        .integer = 1,
        .empty = 0,
        },
    };

LOCAL TEST_FILE_HEADER_PARA g_testFilePara[FILE_HEAD_BOTT] =
    {
        {
        .channels =     AUD_TEST_RECORD_CHANNEL,
        .recSec =       AUD_TEST_RECORD_SECOND,
        .sampleBits =   AUD_TEST_RECORD_SAMPLE_BIT,
        .sampleRate =   AUD_TEST_RECORD_SAMPLE_RATE,
        .format     =   AUD_TEST_FORMAT,
        },
    };

LOCAL void testSetIntervalPara
    (
    VX_SND_INTERVAL * intervals
    )
    {
    INT32 i;

    for (i = VX_SND_HW_PARAM_CHANNELS; i < VX_SND_HW_PARAM_IDX_MAX; i++)
        {
        intervals[i].min = g_testInterval[i].min;
        intervals[i].max = g_testInterval[i].max;
        intervals[i].openMin = g_testInterval[i].openMin;
        intervals[i].openMax = g_testInterval[i].openMax;
        intervals[i].integer = g_testInterval[i].integer;
        intervals[i].empty = g_testInterval[i].empty;
        }

    return;
    }

LOCAL STATUS recordCtrlInfo(VX_SND_CTRL_INFO * ctrlInfo)
    {
    if ((g_testCtrlInfo.cnt + 1)  == AUD_TEST_CTRL_INFO_NUM)
        {
        TEST_AUD_ERR("ctrl cnt is bigger than max\n");
        return ERROR;
        }

    memcpy(&g_testCtrlInfo.ctrlInfo[g_testCtrlInfo.cnt],
            ctrlInfo, sizeof (VX_SND_CTRL_INFO));

    g_testCtrlInfo.cnt++;

    return OK;
    }

LOCAL STATUS getCtrlInfoEnum
    (
    INT32 fd,
    VX_SND_CTRL_INFO *ctrlInfo
    )
    {
    UINT32 i;
    struct vxSndCtrlId *ctrlId;
    VX_SND_CTRL_INFO ctrlInfoEnum;

    ctrlId = &ctrlInfo->id;
    INT32 ret;

    memcpy(&ctrlInfoEnum.id, ctrlId, sizeof (struct vxSndCtrlId));

    for (i = 0; i < ctrlInfo->value.enumerated.itemSum; i++)
        {
        ctrlInfoEnum.value.enumerated.itemSelected = i;

        ret = ioctl (fd, VX_SND_CMD_CTRL_INFO_GET, &ctrlInfoEnum);
        if (ret < 0)
            {
            TEST_AUD_ERR("to get control info error\n");
            return ERROR;
            }
        TEST_AUD_DBG("\t\titemSum: %u\n",
        ctrlInfoEnum.value.enumerated.itemSum);
        TEST_AUD_DBG("\t\titemSelected: %u\n",
        ctrlInfoEnum.value.enumerated.itemSelected);
        TEST_AUD_DBG("\t\tname: %s\n",
        ctrlInfoEnum.value.enumerated.name);

        recordCtrlInfo(&ctrlInfoEnum);
        }

    return OK;
    }

LOCAL STATUS getCtrlInfo
    (
    TEST_AUDIO_INFO *audioInfo
    )
    {
    INT32 fd;
    INT32 ret;
    UINT32 i;
    struct vxSndCtrlId *ctrlId;
    VX_SND_CTRL_INFO ctrlInfo;

    fd = audioInfo->ctrlFd;

    g_testctrlList.offset = 0;
    g_testctrlList.space = AUD_TEST_CTRL_MAX_NUM;
    g_testctrlList.used = 0;
    g_testctrlList.count = AUD_TEST_CTRL_MAX_NUM;
    g_testctrlList.getBuf = (struct vxSndCtrlId *) malloc (sizeof(struct vxSndCtrlId)
                      * g_testctrlList.space);
    if (g_testctrlList.getBuf == NULL)
        {
        TEST_AUD_ERR("to malloc control list getBuf error\n");
        return ERROR;
        }

    for (i= 0; i < AUD_TEST_CTRL_MAX_NUM; i++)
        {
        g_testCtrlName[i] = (char *)malloc(SND_DEV_NAME_LEN);
        if (g_testCtrlName[i] == NULL)
            {
            TEST_AUD_ERR("malloc space for name failed\n");
            goto errOut;
            }
        ctrlId = &g_testctrlList.getBuf[i];
        ctrlId->name = g_testCtrlName[i];
        }

    /* VX_SND_CMD_CTRL_LIST_GET -> get ctrl id */
    ret = ioctl (fd, VX_SND_CMD_CTRL_LIST_GET, (_Vx_ioctl_arg_t)&g_testctrlList);
    if (ret != 0)
        {
        TEST_AUD_ERR("to get control list error\n");
        goto errOut;
        }

    for (i = 0; i < g_testctrlList.used; i++)
        {
        ctrlId = &g_testctrlList.getBuf[i];
        TEST_AUD_DBG ("******************************************\n");
        TEST_AUD_DBG ("name:  %s\n", ctrlId->name);
        TEST_AUD_DBG ("numId:   %u\n", ctrlId->numId);
        TEST_AUD_DBG ("access:  %u\n", ctrlId->access);
        TEST_AUD_DBG ("iface:  %u\n", ctrlId->iface);
        TEST_AUD_DBG ("deviceNum:  %u\n", ctrlId->deviceNum);
        TEST_AUD_DBG ("substreamNum:  %u\n", ctrlId->substreamNum);

        memset (&ctrlInfo, 0, sizeof (VX_SND_CTRL_INFO));

        ctrlInfo.id.numId = ctrlId->numId;
        ctrlInfo.id.iface = ctrlId->iface;
        ctrlInfo.id.deviceNum = ctrlId->deviceNum;
        ctrlInfo.id.substreamNum = ctrlId->substreamNum;
        ctrlInfo.id.name = ctrlId->name;

        /* VX_SND_CMD_CTRL_INFO_GET -> get ctrl info */

        ret = ioctl (fd, VX_SND_CMD_CTRL_INFO_GET, &ctrlInfo);
        if (ret < 0)
            {
            TEST_AUD_ERR("to get control info error\n");
            goto errOut;
            }

        TEST_AUD_DBG ("\n");
        TEST_AUD_DBG ("\tcontrol (%s) detail info:\n", ctrlId->name);
        TEST_AUD_DBG ("\tnumId:  %u\n", ctrlInfo.id.numId);
        TEST_AUD_DBG ("\ttype:  %u\n", ctrlInfo.type);
        TEST_AUD_DBG ("\taccess:  %u\n", ctrlInfo.access);
        TEST_AUD_DBG ("\trtpId:  %u\n", ctrlInfo.rtpId);
        TEST_AUD_DBG ("\tcount:  %u\n", ctrlInfo.count);
        TEST_AUD_DBG ("\n");

        switch (ctrlInfo.type)
            {
            case VX_SND_CTL_DATA_TYPE_BOOLEAN:
            case VX_SND_CTL_DATA_TYPE_INTEGER:
                TEST_AUD_DBG ("\t\tmin:  %u\n",
                                ctrlInfo.value.integer32.min);
                TEST_AUD_DBG ("\t\tmax:  %u\n",
                                ctrlInfo.value.integer32.max);
                TEST_AUD_DBG ("\t\tstep:  %u\n",
                                ctrlInfo.value.integer32.step);
                recordCtrlInfo(&ctrlInfo);
                break;
            case VX_SND_CTL_DATA_TYPE_ENUMERATED:
                if (ctrlInfo.value.enumerated.itemSum > 1)
                    {
                    /* 
                    set numId to INVALID_CTL_NUMID, so that the 
                    ctrl name will be used to get the info 
                    */
                    ctrlInfo.id.numId = INVALID_CTL_NUMID;
                    ret = getCtrlInfoEnum(fd, &ctrlInfo);
                    }
                else
                    {
                    TEST_AUD_DBG("\t\titemSum: %u\n",
                    ctrlInfo.value.enumerated.itemSum);
                    TEST_AUD_DBG("\t\titemSelected: %u\n",
                    ctrlInfo.value.enumerated.itemSelected);
                    TEST_AUD_DBG("\t\tname: %s\n",
                    ctrlInfo.value.enumerated.name);
                    ret = recordCtrlInfo(&ctrlInfo);
                    }

                if (ret != OK)
                    {
                    goto errOut;
                    }
                break;
            case VX_SND_CTL_DATA_TYPE_BYTES_ARRAY:
                TEST_AUD_DBG("value type is bytes array\n");
                break;
            case VX_SND_CTL_DATA_TYPE_INTEGER64:
                TEST_AUD_DBG ("\t\tmin:  %llu\n",
                                ctrlInfo.value.integer64.min);
                TEST_AUD_DBG ("\t\tmax:  %llu\n",
                                ctrlInfo.value.integer64.max);
                TEST_AUD_DBG ("\t\tstep:  %llu\n",
                                ctrlInfo.value.integer64.step);
                recordCtrlInfo(&ctrlInfo);
                break;
            default:
                break;
            }
        TEST_AUD_DBG ("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
        }

    return OK;

errOut:
    for (i= 0; i < AUD_TEST_CTRL_MAX_NUM; i++)
        {
        if (g_testCtrlName[i] == NULL)
            free (g_testCtrlName[i]);
        }

    free (g_testctrlList.getBuf);
    return ERROR;
    }


LOCAL STATUS setupPcmDev
    (
    TEST_AUDIO_INFO *audioInfo
    )
    {
    VX_SND_PCM_HW_PARAMS hwPara;
    INT32 fd;
    INT32 ret;

    if (audioInfo->playback)
        {
        fd = open(audioInfo->playbackName, O_RDWR);
        if (fd < 0)
            {
            TEST_AUD_ERR("open PCM %s error\n", audioInfo->playbackName);
            return ERROR;
            }
        audioInfo->devFd = fd;
        }
    else
        {
        fd = open(audioInfo->captureName, O_RDWR);
        if (fd < 0)
            {
            TEST_AUD_ERR("open PCM %s error\n", audioInfo->captureName);
            return ERROR;
            }
        audioInfo->devFdCapture = fd;
        }

    hwPara.accessTypeBits = SNDRV_PCM_ACCESS_INTERLEAVED;
    hwPara.formatBits = audioInfo->audioData.format;
    hwPara.info = AUD_TEST_PARA_UNKNOW;
    testSetIntervalPara(&hwPara.intervals[0]);

    hwPara.intervals[VX_SND_HW_PARAM_CHANNELS].min = audioInfo->audioData.channels;
    hwPara.intervals[VX_SND_HW_PARAM_CHANNELS].max = audioInfo->audioData.channels;
    hwPara.rates = audioInfo->audioData.sampleRate;

    /*
    * after ioctl(), hwPara returns the selected parameters by audio core.
    * The follow up operations should refer these parameter.
    */

    ret = ioctl (fd, VX_SND_CMD_HW_PARAMS, &hwPara);
    if (ret < 0)
        {
        TEST_AUD_ERR("set PCM hw_para error\n");
        goto errout;
        }

    audioInfo->rate = PARAMS_RATE (&hwPara);
    audioInfo->channels = PARAMS_CHANNELS(&hwPara);
    audioInfo->periodBytes = PARAMS_PERIOD_BYTES (&hwPara);
    audioInfo->periods = PARAMS_BUFFER_BYTES (&hwPara)/audioInfo->periodBytes;

    TEST_AUD_INFO("\trate: %16u\n", audioInfo->rate);
    TEST_AUD_INFO("\tchannels: %16u\n", audioInfo->channels);
    TEST_AUD_INFO("\tperiodBytes: %16u\n", audioInfo->periodBytes);
    TEST_AUD_INFO("\tperiods: %16u\n", audioInfo->periods);

    ret = ioctl (fd, VX_SND_CMD_PREPARE);
    if (ret < 0)
        {
        TEST_AUD_ERR("set PCM state to prepare failed\n");
        goto errout;
        }


    /*
    Now, the PCM device is ready,
    you could call write/read to transfer data.
    */

    return OK;

errout:
    close (fd);
    return ERROR;

    }


LOCAL STATUS configCtrlDev
    (
    TEST_AUDIO_INFO *audioInfo
    )
    {
    UINT32 i, j;
    //UINT32 num = sizeof (g_testCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    UINT32 num = audioInfo->numCtrlConf;
    TEST_CTRL_CONFIG_INFO *ctrlConf = NULL;
    VX_SND_CTRL_INFO *ctrlInfo = NULL;
    VX_SND_CTRL_DATA_VAL dataVal;
    int fd = audioInfo->ctrlFd;
    VX_SND_CTRL_TLV *tlvInfo = NULL;
    STATUS ret;

    for (i = 0; i < num; i++)
        {
        //ctrlConf = &g_testCtrlConf[i];
        ctrlConf = &(audioInfo->ctrlConfInfo)[i];
        for (j = 0; j < g_testCtrlInfo.cnt; j++)
            {
            ctrlInfo = &g_testCtrlInfo.ctrlInfo[j];
            if (strncmp(ctrlConf->ctrlName, ctrlInfo->id.name, SND_DEV_NAME_LEN) != 0)
                {
                continue;
                }

            memset ((void *)&dataVal, 0, sizeof(VX_SND_CTRL_DATA_VAL));
            memcpy (&dataVal.id, &ctrlInfo->id, sizeof(VX_SND_CTRL_ID));

            if (ctrlInfo->type == VX_SND_CTL_DATA_TYPE_INTEGER)
                {
                /* for volume value config */
                ret = ioctl(fd, VX_SND_CMD_CTRL_GET, (_Vx_ioctl_arg_t)&dataVal);
                if (ret != 0)
                    {
                    TEST_AUD_ERR("to get ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                /* do tlv read opt to just get some info */
#ifdef TEST_SND_AUD_DBG_INFO
                tlvInfo = (VX_SND_CTRL_TLV *)malloc (AUD_TEST_TLV_BUF_LEN + sizeof (VX_SND_CTRL_TLV));
                tlvInfo->numId = ctrlInfo->id.numId;
                tlvInfo->length = AUD_TEST_TLV_BUF_LEN;
                ret = ioctl (fd, VX_SND_CMD_TLV_READ, (_Vx_ioctl_arg_t)tlvInfo);
                if (ret != 0)
                    {
                    TEST_AUD_ERR("***to get ctrl (%s) TLV value failed\n", dataVal.id.name);
                    return ERROR;
                    }
                if (tlvInfo->tlvBuf[1] > AUD_TEST_TLV_BUF_LEN)
                    {
                    TEST_AUD_ERR("tlv buffer size is too small\n");
                    }
                TEST_AUD_DBG("tlv type %u, len %u\n", tlvInfo->tlvBuf[0],
                               tlvInfo->tlvBuf[1]);
                free (tlvInfo);
#endif
                /* value[1] for right, value[0] for left */
                dataVal.value.integer32.value[0] = ctrlConf->value.integer32.value;
                dataVal.value.integer32.value[1] = ctrlConf->value.integer32.value;
                ret = ioctl(fd, VX_SND_CMD_CTRL_PUT, (_Vx_ioctl_arg_t)&dataVal);
                if (ret != 0)
                    {
                    TEST_AUD_ERR("***to set ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }
                break;
                }
            else
                {
                /* for enumerate type */
                if (strncmp (ctrlConf->value.enumerated.value,
                             ctrlInfo->value.enumerated.name,
                             SND_DEV_NAME_LEN) !=  0)
                    {
                    continue;
                    }

                dataVal.value.enumerated.item[0] = ctrlInfo->value.enumerated.itemSelected;
                dataVal.value.enumerated.item[1] = ctrlInfo->value.enumerated.itemSelected;
                TEST_AUD_DBG("item name %s, item number %u\n",
                               ctrlInfo->value.enumerated.name,
                               ctrlInfo->value.enumerated.itemSelected);

                ret = ioctl(fd, VX_SND_CMD_CTRL_PUT, (_Vx_ioctl_arg_t)&dataVal);
                if (ret != 0)
                    {
                    TEST_AUD_ERR("***to set ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }
                break;
                }
            }

        if (j == g_testCtrlInfo.cnt)
            {
            TEST_AUD_ERR("ctrl device (%s) not found\n", ctrlConf->ctrlName);
            //return ERROR;
            }
        }

    return OK;
    }

LOCAL STATUS setupCtrlDev
    (
    TEST_AUDIO_INFO * audioInfo
    )
    {
    STATUS ret;
    INT32 fd;

    fd = open(g_testCtrl->name, O_RDWR);
    if (fd < 0)
        {
        TEST_AUD_ERR("open ctrl device %s error\n", g_testCtrl->name);
        return ERROR;
        }
    audioInfo->ctrlFd = fd;

    ret = getCtrlInfo(audioInfo);
    if (ret != OK)
        {
        TEST_AUD_ERR ("to get all ctrl info failed\n");
        return ERROR;
        }

    /* VX_SND_CMD_CTRL_PUT      -> set ctrl attribute */

    ret = configCtrlDev(audioInfo);
    if (ret != OK)
        {
        TEST_AUD_ERR ("to config ctrl device failed\n");
        return ERROR;
        }

    return OK;
    }

STATUS setupAudioDev
    (
    TEST_AUDIO_INFO *audioInfo
    )
    {
    STATUS ret;

    memset (&g_testctrlList, 0, sizeof(VX_SND_CTRL_LIST));
    memset (&g_testCtrlInfo.ctrlInfo[0], 0,
            sizeof (VX_SND_CTRL_INFO) * AUD_TEST_CTRL_INFO_NUM);
    g_testCtrlInfo.cnt = 0;

    /* set fd to invalid value */

    audioInfo->devFd = -1;
    audioInfo->devFdCapture = -1;
    audioInfo->ctrlFd = -1;

    ret = setupCtrlDev(audioInfo);
    if (ret != OK)
        {
        TEST_AUD_ERR("setup ctrl device failed\n");
        goto errorOut;
        }
    TEST_AUD_INFO("complete ctrl configuration\n");

    ret = setupPcmDev(audioInfo);
    if (ret != OK)
        {
        TEST_AUD_ERR("setup PCM device failed\n");
        goto errorOut;;
        }
    TEST_AUD_INFO("complete playback/capture device configuration\n");

    return OK;

errorOut:
    if (audioInfo->ctrlFd != -1)
        close (audioInfo->ctrlFd);
    if (audioInfo->devFd != -1)
        close (audioInfo->devFd);
    if (audioInfo->devFdCapture != -1)
        close (audioInfo->devFdCapture);
    return ERROR;
    }


LOCAL STATUS audioOpenReadFile
    (
    TEST_AUDIO_INFO *  pAudInfo
    )
    {
    int     sampleBits;
    UINT32  size = 0;
    int     channels;
    UINT32  sampleRate;
    UINT32  samples;
    UINT32  dataStart;
    UINT32  readBytes;
    VX_SND_PCM_HARDWARE hw;

    /* open the audio file */

    pAudInfo->wavFd = open ((char *)pAudInfo->wavfilename, O_RDONLY, 0666);
    if (pAudInfo->wavFd < 0)
        {
        TEST_AUD_ERR("Error opening file %s\n", pAudInfo->wavfilename);
        return ERROR;
        }

    /* assume a wav file and read the wav form header file */

    if (audioWavHeaderRead (pAudInfo->wavFd, &channels, &sampleRate, 
                            &sampleBits, &samples, &dataStart) != OK)
        {
        return ERROR;
        }

    if ((16 != sampleBits) && (24 != sampleBits) & (32 != sampleBits))
        {
        sampleBits = 8;
        }

    size = samples * channels * (sampleBits >> 3);

    /* print characteristics of the sound data stream  */

    TEST_AUD_INFO ("File name:   %s\n", pAudInfo->wavfilename);
    TEST_AUD_INFO ("Channels:    %d\n", channels);
    TEST_AUD_INFO ("Sample Rate: %d\n", sampleRate);
    TEST_AUD_INFO ("Sample Bits: %d\n", sampleBits);
    TEST_AUD_INFO ("Samples:     %d\n", samples);

    hw.rates = 0;
    hw.rateMin = sampleRate;
    hw.rateMax = sampleRate;
    vxSndPcmHwRateToBitmap(&hw);

    pAudInfo->audioData.channels     = (UINT8)channels;
    pAudInfo->audioData.sampleBits   = (UINT8)sampleBits;
    pAudInfo->audioData.sampleBytes  = (UINT8)(sampleBits >> 3);
    pAudInfo->audioData.sampleRate   = hw.rates;
    pAudInfo->audioData.useMsb       = 0;
    pAudInfo->dataStart              = dataStart;
    pAudInfo->dataSize               = size;

    switch (sampleBits)
        {
        case 8:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S8;
            break;
        case 16:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S16_LE;
            break;
        case 24:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S24_LE;
            break;
        case 32:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S32_LE;
            break;
        default:
            pAudInfo->audioData.format = VX_SND_FMTBIT_S16_LE;
            break;
        }
    TEST_AUD_INFO ("Formats:     %d\n", pAudInfo->audioData.format);

    pAudInfo->wavData = (char*) malloc(pAudInfo->dataSize);
    if (pAudInfo->wavData == NULL)
        {
        TEST_AUD_ERR("malloc wavData buffer failed\n");
        return ERROR;
        }

    /* Position to data start of file */

    if (ERROR == lseek (pAudInfo->wavFd, dataStart, SEEK_SET))
        {
        return ERROR;
        }

    /* Read enough data from the file */

    readBytes = (UINT32)read (pAudInfo->wavFd,
                                pAudInfo->wavData,
                                pAudInfo->dataSize);
    if (readBytes < pAudInfo->dataSize)
        {
        TEST_AUD_ERR("read %u bytes, expected %u bytes\n",
                     readBytes, pAudInfo->dataSize);
        return ERROR;
        }
    close (pAudInfo->wavFd);

    return OK;
    }

STATUS getWavData
    (
    TEST_AUDIO_INFO *audioInfo,
    BOOL fromfile,
    char* wavfile
    )
    {
    STATUS ret;
    unsigned int i;

    if (fromfile)
        {
        if (wavfile == NULL)
            {
            return ERROR;
            }
        TEST_AUD_INFO ("wavefile: %s\n", wavfile);
        audioInfo->wavfilename = wavfile;
        ret = audioOpenReadFile (audioInfo);
        if (ret == ERROR)
            {
            TEST_AUD_ERR("get audio data info fail\n");
            return ERROR;
            }
        }
    else
        {
        /* generate some random data */
        for (i = 0; i < sizeof(g_buffer); i++)
            {
            g_buffer[i] = rand() & 0xff;
            }

        audioInfo->wavData = (char *)&g_buffer[0];
        audioInfo->dataSize = sizeof (g_buffer);

        audioInfo->audioData.channels = g_testFilePara[FILE_HEAD_OPT0].channels;
        audioInfo->audioData.sampleBits = g_testFilePara[FILE_HEAD_OPT0].sampleBits;
        audioInfo->audioData.sampleBytes = (g_testFilePara[FILE_HEAD_OPT0].sampleBits >> 3);
        audioInfo->audioData.sampleRate = SNDRV_PCM_RATE_48000;
        audioInfo->audioData.format = g_testFilePara[FILE_HEAD_OPT0].format;

        audioInfo->dataSize = sizeof (g_buffer);
        audioInfo->dataStart = 0;

        TEST_AUD_INFO ("File name:   random data\n");
        TEST_AUD_INFO ("Channels:    %d\n", audioInfo->audioData.channels);
        TEST_AUD_INFO ("Sample Rate: %d\n", audioInfo->audioData.sampleRate);
        TEST_AUD_INFO ("Sample Bits: %d\n", audioInfo->audioData.sampleBits);
        TEST_AUD_INFO ("samples:     %d\n", audioInfo->dataSize / audioInfo->audioData.sampleBytes);
        TEST_AUD_INFO ("format:      %d\n", audioInfo->audioData.format);
        }

    return OK;
    }

LOCAL STATUS setAudioInfo(TEST_AUDIO_INFO* pAudInfo)
    {
    TEST_FILE_HEADER_PARA *fileHeader = &g_testFilePara[FILE_HEAD_OPT0];
    UINT32 channels = fileHeader->channels;

    channels = ((channels == 1)?1:2);
    pAudInfo->audioData.channels     = (UINT8)channels;
    pAudInfo->audioData.sampleBits   = (UINT8)fileHeader->sampleBits;
    pAudInfo->audioData.sampleRate   = fileHeader->sampleRate;
    pAudInfo->audioData.sampleBytes  = (UINT8)(fileHeader->sampleBits >> 3);
    pAudInfo->audioData.format       = fileHeader->format;

    pAudInfo->dataSize = (pAudInfo->audioData.sampleBytes *
                        vxSndGetRateFromBitmap(fileHeader->sampleRate) *
                        pAudInfo->audioData.channels *
                        fileHeader->recSec);

    TEST_AUD_INFO ("Channels:    %d\n", pAudInfo->audioData.channels);
    TEST_AUD_INFO ("Sample Rate: %d\n", pAudInfo->audioData.sampleRate);
    TEST_AUD_INFO ("Sample Bits: %d\n", pAudInfo->audioData.sampleBits);
    TEST_AUD_INFO ("Sample sampleBytes: %d\n", pAudInfo->audioData.sampleBytes);
    TEST_AUD_INFO ("Sample dataSize: %d\n", pAudInfo->dataSize);

    return OK;
    }

LOCAL STATUS audioCreateWriteFile(TEST_AUDIO_INFO * pAudInfo)
    {
    UINT32              samples;
    TEST_AUD_DATA_INFO *audioData = &pAudInfo->audioData;
    STATUS              retVal;
    UINT32              sampleRate;

    pAudInfo->recordFd = open (pAudInfo->recordfilename, O_RDWR | O_CREAT, 0666);
    if (pAudInfo->recordFd < 0)
        {
        TEST_AUD_ERR ("Error create file %s\n", pAudInfo->recordfilename);
        return ERROR;
        }

    /* create the WAV file headers */

    samples = (UINT32)(pAudInfo->dataSize / (audioData->sampleBits >> 3));
    sampleRate = vxSndGetRateFromBitmap(audioData->sampleRate);
    if (ERROR == audioWavHeaderWrite (pAudInfo->recordFd,
                                      (UINT8)audioData->channels,
                                      sampleRate,
                                      (UINT8)audioData->sampleBits,
                                      samples))
        {
        TEST_AUD_ERR ("Write wav file header failed!\n");
        goto out;
        }

    /*
    get data from capture and record the data,
    after audioWavHeaderWrite(), the file pointer should move to
    the position where the data should be wrote to
    */

    retVal = readFromCapture(pAudInfo);
    if(retVal == ERROR)
        {
        TEST_AUD_ERR("record capture data failed\n");
        goto out;
        }
    return OK;

out:
    /* close and remove the audio file when some errors occurs*/
    if (pAudInfo->recordFd > 0)
        {
        (void)close (pAudInfo->recordFd);
        (void)remove(pAudInfo->recordfilename);
        }
    return ERROR;
    }

STATUS recoredWavData(TEST_AUDIO_INFO *audioInfo, char* wavfile)
    {
    STATUS ret;

    if (wavfile == NULL)
        {
        return ERROR;
        }

    TEST_AUD_INFO ("wavefile: %s\n", wavfile);
    audioInfo->recordfilename = wavfile;
    audioInfo->dataStart = 0;
    ret = audioCreateWriteFile (audioInfo);
    if (ret == ERROR)
        {
        TEST_AUD_ERR("create audio file fail\n");
        return ERROR;
        }

    return OK;
    }

STATUS cleanupAudioInfo(TEST_AUDIO_INFO *audioInfo)
    {
    if (audioInfo->wavfilename != NULL)
        {
        free (audioInfo->wavData);
        }

    if (audioInfo->recordfilename != NULL)
        {
        close (audioInfo->recordFd);

        if (audioInfo->needRemove)
            {
            remove(audioInfo->recordfilename);
            }
        }

    return OK;
    }

STATUS cleanupAudioDev(TEST_AUDIO_INFO *audioInfo)
    {
    STATUS ret;
    BOOL   arg = TRUE;

    /* clean up PCM device */

    if (audioInfo->playback)
        {
        /* playback */

        TEST_AUD_INFO("ioctl drain playback\n");
        ret = ioctl (audioInfo->devFd, VX_SND_CMD_DRAIN);
        if (ret < 0)
            {
            TEST_AUD_ERR("set PCM state to drain failed\n");
            }

        /*
        PCM will wait for some time to complete the transfer when
        drain operation, when it return we could close the fd
        */

        TEST_AUD_INFO("close playback fd\n");
        close(audioInfo->devFd);
        }
    else
        {
        /* capture */

        TEST_AUD_INFO("ioctl pause capture\n");
        ret = ioctl (audioInfo->devFdCapture, VX_SND_CMD_PAUSE, &arg);
        if (ret < 0)
            {
            TEST_AUD_ERR("set PCM state to pause failed\n");
            }
        TEST_AUD_INFO("close capture fd\n");
        close(audioInfo->devFdCapture);
        }

    /* close the ctrl device */

    TEST_AUD_INFO("close control device fd\n");
    close(audioInfo->ctrlFd);
    return OK;
    }

STATUS sendDataToPlayback(TEST_AUDIO_INFO *audioInfo)
    {
    char *  buff = NULL;
    UINT32  buffSize = 0;
    long    ret;
    UINT32  i, n, leftBytes;

    buffSize = audioInfo->periodBytes;
    buff = (char *)malloc(buffSize);
    if (buff == NULL)
        {
        TEST_AUD_INFO("malloc buff for write failed\n");
        return ERROR;
        }
    n = audioInfo->dataSize / buffSize;
    leftBytes = audioInfo->dataSize % buffSize;

    /* write to playback */
    for (i = 0; i < n; i ++)
        {
        memcpy (buff,
                audioInfo->wavData + (i*buffSize),
                buffSize);
        ret= write (audioInfo->devFd, buff, buffSize);
        if (ret < 0)
            {
            TEST_AUD_ERR("to transfer data to PCM error\n");
            return ERROR;
            }
        }
    if (leftBytes != 0)
        {
        memset (buff, 0, buffSize);
        memcpy (buff, audioInfo->wavData + (n*buffSize), leftBytes);
        ret= write (audioInfo->devFd, buff, buffSize);
        if (ret < 0)
            {
            TEST_AUD_ERR("to transfer data to PCM error\n");
            free(buff);
            return ERROR;
            }
        }
    free(buff);

    return OK;
    }

LOCAL STATUS readFromCapture(TEST_AUDIO_INFO *audioInfo)
    {
    char *  buff = NULL;
    UINT32  buffSize = 0;
    long    ret;
    UINT32  i, n, leftBytes;

    buffSize = audioInfo->periodBytes * AUD_TEST_RECORD_FACTOR;

    buff = (char *)malloc(buffSize);
    if (buff == NULL)
        {
        TEST_AUD_INFO("malloc buff for write failed\n");
        return ERROR;
        }

    n = audioInfo->dataSize / buffSize;
    leftBytes = audioInfo->dataSize % buffSize;

    /* read from playback and record to file */
    for (i = 0; i < n; i ++)
        {
        memset (buff, 0, buffSize);
        ret= read (audioInfo->devFdCapture, buff, buffSize);
        if (ret != buffSize)
            {
            TEST_AUD_ERR("read data from PCM error(transfer "
                         "size:%d, expect: %d)\n", ret, buffSize);
            return ERROR;
            }

        /* make sure the file pointer should be after wav file header */
        ret = write (audioInfo->recordFd, buff, buffSize);
        if (ret != buffSize)
            {
            TEST_AUD_ERR("record data error(transfer size:%d), errno: %d\n", 
            ret, errnoGet());
            return ERROR;
            }
        }
    if (leftBytes != 0)
        {
        memset (buff, 0, buffSize);

        ret= read (audioInfo->devFdCapture, buff, leftBytes);
        if (ret != leftBytes)
            {
            TEST_AUD_ERR("read data from PCM error(transfer size:%d)\n", ret);
            return ERROR;
            }

        /* make sure the file pointer should be after wav file header */
        ret = write (audioInfo->recordFd, buff, leftBytes);
        if (ret != leftBytes)
            {
            TEST_AUD_ERR("record data error(transfer size:%d)\n", ret);
            return ERROR;
            }
        }

    free(buff);

    return OK;
    }

STATUS tmAudio_mini()
    {
    STATUS retVal;
    TEST_AUDIO_INFO audioInfo;
    SND_CONTROL_DEV sndCtrlDev;

    g_testCtrl = &sndCtrlDev;
    strncpy(g_testCtrl->name, AUTIO_TEST_CONTROL_NAME, SND_DEV_NAME_LEN);

    memset (&audioInfo, 0, sizeof(TEST_AUDIO_INFO));

    retVal = getWavData (&audioInfo, FALSE, NULL);
    if (retVal != OK)
        {
        TEST_AUD_ERR("get wav file audio info failed\n");
        return ERROR;
        }

    audioInfo.playback = TRUE;
    strncpy(audioInfo.playbackName, AUDIO_TEST_PCM_P_NAME, SND_DEV_NAME_LEN);

    audioInfo.numCtrlConf = sizeof (g_testCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio device failed\n");
        return ERROR;
        }

    retVal = sendDataToPlayback(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("sent data to playback failed\n");
        return ERROR;
        }

    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    return OK;
    }

STATUS tmAudio_playback()
    {
    STATUS retVal;
    TEST_AUDIO_INFO audioInfo;

    SND_CONTROL_DEV sndCtrlDev;
    g_testCtrl = &sndCtrlDev;
    strncpy(g_testCtrl->name, AUTIO_TEST_CONTROL_NAME, SND_DEV_NAME_LEN);

    memset (&audioInfo, 0, sizeof(TEST_AUDIO_INFO));

    retVal = getWavData (&audioInfo, TRUE, AUD_TEST_WAV_FILE);
    if (retVal != OK)
        {
        TEST_AUD_ERR("get wav file audio info failed\n");
        return ERROR;
        }

    audioInfo.playback = TRUE;
    strncpy(audioInfo.playbackName, AUDIO_TEST_PCM_P_NAME, SND_DEV_NAME_LEN);

    audioInfo.numCtrlConf = sizeof (g_testCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio device failed\n");
        goto errorOut;
        }

    TEST_AUD_INFO("audio device is ready, start to send data to PCM\n");
    retVal = sendDataToPlayback(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("sent data to playback failed\n");
        goto errorOut;
        }

    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    return OK;

errorOut:
    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    return ERROR;
    }


STATUS tmAudio_capture()
    {
    TEST_AUDIO_INFO audioInfo;
    STATUS retVal;

    SND_CONTROL_DEV sndCtrlDev;
    g_testCtrl = &sndCtrlDev;
    strncpy(g_testCtrl->name, AUTIO_TEST_CONTROL_NAME, SND_DEV_NAME_LEN);

    memset (&audioInfo, 0, sizeof(TEST_AUDIO_INFO));
    (void)setAudioInfo(&audioInfo);
    audioInfo.needRemove = FALSE;

    audioInfo.playback = FALSE;
    strncpy(audioInfo.captureName, AUDIO_TEST_PCM_C_NAME, SND_DEV_NAME_LEN);

    audioInfo.numCtrlConf = sizeof (g_testCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio device failed\n");
        goto errorOut;
        }

    retVal = recoredWavData(&audioInfo, AUD_TEST_RECORED_FILE);
    if (retVal != OK)
        {
        TEST_AUD_ERR("record capture data failed\n");
        goto errorOut;
        }

    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    retVal = getWavData (&audioInfo, TRUE, AUD_TEST_RECORED_FILE);
    if (retVal != OK)
        {
        TEST_AUD_ERR("get wav file audio info failed\n");
        return ERROR;
        }

    audioInfo.playback = TRUE;
    strncpy(audioInfo.playbackName, AUDIO_TEST_PCM_P_NAME, SND_DEV_NAME_LEN);

    audioInfo.numCtrlConf = sizeof (g_testCtrlConf) / sizeof (TEST_CTRL_CONFIG_INFO);
    audioInfo.ctrlConfInfo = &g_testCtrlConf[0];

    retVal = setupAudioDev(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("setup audio device failed\n");
        goto errorOut;
        }

    TEST_AUD_INFO("audio device is ready, start to send data to PCM\n");
    retVal = sendDataToPlayback(&audioInfo);
    if (retVal != OK)
        {
        TEST_AUD_ERR("sent data to playback failed\n");
        goto errorOut;
        }

    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    return OK;

errorOut:
    cleanupAudioDev(&audioInfo);

    cleanupAudioInfo(&audioInfo);

    return ERROR;
    }

STATUS tmAudio_WavParse()
    {
    STATUS ret;

    TEST_AUDIO_INFO audioInfo;
    char wavfile[256];
    DIR * dirInfo = NULL;
    struct dirent *dirent = NULL;

    dirInfo = opendir ("/romfs");
    if (dirInfo == NULL)
        {
        TEST_AUD_ERR("open directory /romfs failed\n");
        return ERROR;
        }

    while (1)
        {
        dirent = readdir(dirInfo);
        if (dirent == NULL)
            {
            break;
            }
        if (strncmp (dirent->d_name, ".", 256) == 0 ||
            strncmp (dirent->d_name, "..", 256) == 0)
            {
            continue;
            }

        TEST_AUD_INFO("read from /romfs: %s\n", dirent->d_name);

        snprintf(wavfile, sizeof(wavfile), "/romfs/%s", dirent->d_name);

        TEST_AUD_INFO ("wavefile: %s\n", wavfile);

        ret = getWavData (&audioInfo, TRUE, wavfile);
        if (ret == ERROR)
            {
            TEST_AUD_ERR("get audio data info fail\n");
            return ERROR;
            }

        if (audioInfo.wavData != NULL)
            {
            free (audioInfo.wavData);
            }
        close (audioInfo.wavFd);
        }

    return OK;
    }


void audiTestInit()
    {
    g_testSndPcmDbgMask = (TEST_SND_AUD_DBG_INFO | TEST_SND_AUD_DBG_ERR);
    return;
    }


LOCAL VXTEST_ENTRY vxTestTbl_tmAudioNew[] =
{
    /*pTestName,             FUNCPTR,                pArg,   flags,   cpuSet,   timeout,   exeMode,   osMode,   level*/
    {"tmAudio_mini",        (FUNCPTR)tmAudio_mini, 0, 0, 0, 5000, VXTEST_EXEMODE_ALL, VXTEST_OSMODE_ALL, 0,"test functionality of audio device driver."},
    {"tmAudio_playback",    (FUNCPTR)tmAudio_playback, 0, 0, 0, 500000, VXTEST_EXEMODE_ALL, VXTEST_OSMODE_ALL, 0,"test functionality of audio device driver."},
    {"tmAudio_capture",     (FUNCPTR)tmAudio_capture, 0, 0, 0, 5000, VXTEST_EXEMODE_ALL, VXTEST_OSMODE_ALL, 0,"test functionality of audio device driver."},
    {"tmAudio_WavParse",    (FUNCPTR)tmAudio_WavParse, 0, 0, 0, 5000, VXTEST_EXEMODE_ALL, VXTEST_OSMODE_ALL, 0,"test functionality of wav file parse"},
    {NULL, (FUNCPTR)"tmAudioNew", 0, 0, 0, 1000000, 0, 0, 0}
};

/**************************************************************************
*
* tmAudioNewExec - Exec test module
*
* This routine should be called to initialize the test module.
*
* RETURNS: N/A
*
* \NOMANUAL
*/
#ifdef _WRS_KERNEL

STATUS tmAudioNewExec
    (
    char * testCaseName,
    VXTEST_RESULT * pTestResult
    )
    {
    return vxTestRun((VXTEST_ENTRY**)&vxTestTbl_tmAudioNew, testCaseName, pTestResult);
    }

#else

STATUS tmAudioNewExec
    (
    char * testCaseName,
    VXTEST_RESULT * pTestResult,
    int    argc,
    char * argv[]
    )
    {
    return vxTestRun((VXTEST_ENTRY**)&vxTestTbl_tmAudioNew, testCaseName, pTestResult, argc, argv);
    }

/**************************************************************************
* main - User application entry function
*
* This routine is the entry point for user application. A real time process
* is created with first task starting at this entry point.
*
* \NOMANUAL
*/
int main
    (
    int    argc,    /* number of arguments */
    char * argv[]   /* array of arguments */
    )
    {
    return tmAudioNewExec(NULL, NULL, argc, argv);
    }

#endif
