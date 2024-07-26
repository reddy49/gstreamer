/* vxSoundDemoControl.c - VxSound Framework Control Demo */

/*
 * Copyright (c) 2023 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
DESCRIPTION
This file provides a control demo for Samsung ExynosAuto V920.

The configuration procedure is as below:
    configure audio path and volume.
     -> amixer(0)

This demo configures the audio paths as:
    playback path
        1. pcmC0D0p -> RDMA0 -> UAIF2
        2. pcmC0D1p -> RDMA1 -> UAIF3
        3. pcmC0D2p -> RDMA2 -> UAIF8
        4. pcmC0D3p -> RDMA3 -> UAIF9
    capture path
        1. pcmC0D16c -> WDMA0 -> UAIF5
*/

/* includes */

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <dllLib.h>
#include <vxSoundCore.h>
#include <control.h>
#include <pcm.h>
#include <soc.h>
#include <audioLibWav.h>

#ifdef _WRS_KERNEL
#include <unistd.h>
#endif /* _WRS_KERNEL */


#undef PRINT_CTRL_LIST_INFO
#undef PRINT_CTRL_INFO_FUNC_INFO




#define DEBUG_VX_SND_CTRL_DEMO
#ifdef _WRS_KERNEL

#ifdef DEBUG_VX_SND_CTRL_DEMO

# include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_CTRL_MSG(...)                                                \
        do                                                                  \
            {                                                               \
            if (_func_kprintf != NULL)                                      \
                {                                                           \
                (* _func_kprintf)(__VA_ARGS__);                           \
                }                                                           \
            }                                                               \
        while (0)
#else /* DEBUG_VX_SND_CTRL_DEMO */
#define VX_SND_CTRL_MSG(...)  do { } while (FALSE)
#endif /* DEBUG_VX_SND_CTRL_DEMO */

#else /* _WRS_KERNEL */

#ifdef DEBUG_VX_SND_CTRL_DEMO
#define VX_SND_CTRL_MSG(...)  printf(##__VA_ARGS__)
#else /* DEBUG_VX_SND_CTRL_DEMO */
#define VX_SND_CTRL_MSG(...)  do { } while (FALSE)
#endif /* DEBUG_VX_SND_CTRL_DEMO */

#endif /* _WRS_KERNEL */

#define AUDIO_TEST_DEV_NUM                  0
#define AUDIO_TEST_PARA_UNKNOW              0

#define CONTROL_LIST_ID_MAX              512
#define AUDIO_TEST_CTRL_INFO_NUM         CONTROL_LIST_ID_MAX//    (2*4096)
#define AUDIO_TEST_TLV_BUF_LEN               64

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define VX_SND_CTRL_DEV_DEFAULT "/dev/snd/controlC0"

#define DEMO_CTRL_CONFIG_ENUM(name, a)            \
    {                                                \
    .ctrlName = name,                                \
    .type = VX_SND_CTL_DATA_TYPE_ENUMERATED,         \
    .value = {.enumerated = {.value = a}}       }

#define DEMO_CTRL_CONFIG_INT(name, b)            \
    {                                                \
    .ctrlName = name,                                \
    .type = VX_SND_CTL_DATA_TYPE_INTEGER,            \
    .value = {.integer32 = {.value = b}}        }

typedef struct ctrlInfoAll
    {
    uint32_t cnt;
    VX_SND_CTRL_INFO ctrlInfo[AUDIO_TEST_CTRL_INFO_NUM];
    char * enumNames[AUDIO_TEST_CTRL_INFO_NUM];
    } DEMO_CTRL_INFO_ALL;

typedef struct ctrlConfigInfo
    {
    char * ctrlName;
    VX_SND_CTRL_DATA_TYPE type;
    union
        {
        struct
            {
            uint32_t value;
            } integer32;
        struct
            {
            UINT64 value;
            } integer64;
        struct
            {
            char *value;
            } enumerated;
//        unsigned char reserved[128];
        } value;
    } CTRL_CONFIG_INFO_CONST;

LOCAL BOOL          listInfoGeted = FALSE;
LOCAL BOOL          defaultConfigured = FALSE;

LOCAL VX_SND_CTRL_LIST gControlList;

LOCAL DEMO_CTRL_INFO_ALL gControlInfo;

LOCAL CTRL_CONFIG_INFO_CONST gAboxAudioConf[] =
    {

    /* PCM0p -> RDMA0 -> UAIF2 SPK */

    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF2 SPK", "SIFS0_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM0p RDMA Route", "RDMA0"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM0p PP Route", "Direct"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA0 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA0 ASRC Route", "SIFS"),

    DEMO_CTRL_CONFIG_INT("Speaker Driver CH1 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("Speaker Driver CH2 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("Speaker Driver CH3 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("Speaker Driver CH4 Playback Volume", 200),

    /* PCM1p -> RDMA1 -> UAIF3 SPK */

    DEMO_CTRL_CONFIG_ENUM("ABOX PCM1p RDMA Route", "RDMA1"),
    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF3 SPK", "SIFS1_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM1p PP Route", "Direct"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA1 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA1 ASRC Route", "SIFS"),

    DEMO_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH1 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH2 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH3 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_1 Speaker Driver CH4 Playback Volume", 200),

    /* PCM2p -> RDMA2 -> UAIF8 SPK */

    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF8 SPK", "SIFS2_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM2p RDMA Route", "RDMA2"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM2p PP Route", "Direct"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA2 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA2 ASRC Route", "SIFS"),

    DEMO_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH1 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH2 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH3 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_2 Speaker Driver CH4 Playback Volume", 200),

    /* PCM3p -> RDMA3 -> UAIF9 SPK */

    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF9 SPK", "SIFS3_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM3p RDMA Route", "RDMA3"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM3p PP Route", "Direct"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA3 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA3 ASRC Route", "SIFS"),

    DEMO_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH1 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH2 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH3 Playback Volume", 200),
    DEMO_CTRL_CONFIG_INT("tas6424_3 Speaker Driver CH4 Playback Volume", 200),

    /*PCM0c <- WDMA0 <- UAIF5 */

    DEMO_CTRL_CONFIG_ENUM("ABOX NSRC0", "UAIF5 MAIN MIC"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM0c WDMA Route", "WDMA0"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM0c PP Route", "Direct"),
    DEMO_CTRL_CONFIG_ENUM("ABOX SIFM0", "Reduce Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX WDMA0 Reduce Route", "WDMA"),

    DEMO_CTRL_CONFIG_ENUM("ABOX PCM4p RDMA Route", "RDMA4"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM5p RDMA Route", "RDMA5"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM6p RDMA Route", "RDMA6"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM7p RDMA Route", "RDMA7"),

    DEMO_CTRL_CONFIG_ENUM("ABOX PCM1c WDMA Route", "WDMA1"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM2c WDMA Route", "WDMA2"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM3c WDMA Route", "WDMA3"),
    DEMO_CTRL_CONFIG_ENUM("ABOX PCM4c WDMA Route", "WDMA4"),

    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF0 SPK", "SIFS4_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF0 2TX SPK", "SIFS5_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF0 3TX SPK", "SIFS6_SET"),
    DEMO_CTRL_CONFIG_ENUM("ABOX UAIF0 4TX SPK", "SIFS7_SET"),

    DEMO_CTRL_CONFIG_ENUM("ABOX NSRC1", "UAIF1 MAIN MIC"),
    DEMO_CTRL_CONFIG_ENUM("ABOX NSRC2", "UAIF1 SEC MIC"),
    DEMO_CTRL_CONFIG_ENUM("ABOX NSRC3", "UAIF1 THIRD MIC"),
    DEMO_CTRL_CONFIG_ENUM("ABOX NSRC4", "UAIF1 FOURTH MIC"),

    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA4 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA4 ASRC Route", "SIFS"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA5 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA5 ASRC Route", "SIFS"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA6 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA6 ASRC Route", "SIFS"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA7 Expand Route", "ASRC Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX RDMA7 ASRC Route", "SIFS"),

    DEMO_CTRL_CONFIG_ENUM("ABOX SIFM1", "Reduce Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX WDMA1 Reduce Route", "WDMA"),
    DEMO_CTRL_CONFIG_ENUM("ABOX SIFM2", "Reduce Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX WDMA2 Reduce Route", "WDMA"),
    DEMO_CTRL_CONFIG_ENUM("ABOX SIFM3", "Reduce Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX WDMA3 Reduce Route", "WDMA"),
    DEMO_CTRL_CONFIG_ENUM("ABOX SIFM4", "Reduce Route"),
    DEMO_CTRL_CONFIG_ENUM("ABOX WDMA4 Reduce Route", "WDMA"),
    };

/* function declarations */

LOCAL STATUS vxSndCtrl (uint32_t v);


LOCAL STATUS recordCtrlInfo
    (
    VX_SND_CTRL_INFO * ctrlInfo
    )
    {
    VX_SND_CTRL_INFO * pCtrlInfo;
    int                idx;
    int                count = gControlInfo.cnt;
    BOOL               foundCtrlInfo = FALSE;
    uint32_t           selectedEnum;

    if (ctrlInfo->type != VX_SND_CTL_DATA_TYPE_ENUMERATED)
        {
        if ((count + 1)  == AUDIO_TEST_CTRL_INFO_NUM)
            {
            VX_SND_CTRL_MSG("gControlInfo cnt is bigger than max\n");
            return ERROR;
            }

        pCtrlInfo = &gControlInfo.ctrlInfo[count];

        memcpy(pCtrlInfo, ctrlInfo, sizeof (VX_SND_CTRL_INFO));
        gControlInfo.cnt++;
        }
    else
        {
        for (idx = 0; idx < count; idx++)
            {
            if (strncmp (gControlInfo.ctrlInfo[idx].id.name,
                         ctrlInfo->id.name, SND_DEV_NAME_LEN) == 0)
                {

                /* found an exist enum control */

                foundCtrlInfo = TRUE;
                break;
                }
            }

        if (!foundCtrlInfo)
            {
            if ((count + 1)  == AUDIO_TEST_CTRL_INFO_NUM)
                {
                VX_SND_CTRL_MSG("gControlInfo cnt is bigger than max\n");
                return ERROR;
                }

            pCtrlInfo = &gControlInfo.ctrlInfo[count];
            memcpy (pCtrlInfo, ctrlInfo, sizeof (VX_SND_CTRL_INFO));

            selectedEnum = pCtrlInfo->value.enumerated.itemSelected;

            /* allocate memory to store all enum names */

            gControlInfo.enumNames[count] = calloc (SND_DEV_NAME_LEN *
                                           pCtrlInfo->value.enumerated.itemSum, 1);
            if (gControlInfo.enumNames[count] == NULL)
                {
                VX_SND_CTRL_MSG ("failed to allocate memory for "
                                 "gControlInfo.enumNames\n");
                return ERROR;
                }

            strncpy (gControlInfo.enumNames[count] + selectedEnum * SND_DEV_NAME_LEN,
                     pCtrlInfo->value.enumerated.name, SND_DEV_NAME_LEN);

            gControlInfo.cnt++;
            }
        else
            {
            //pCtrlInfo = &gControlInfo.ctrlInfo[idx];

            //memcpy (pCtrlInfo, ctrlInfo, sizeof (VX_SND_CTRL_INFO));

            selectedEnum = ctrlInfo->value.enumerated.itemSelected;

            if (gControlInfo.enumNames[idx] == NULL)
                {
                VX_SND_CTRL_MSG("an exist control %s should have enumNames "
                                "pointer\n", ctrlInfo->id.name);
                return ERROR;
                }

            memset (gControlInfo.enumNames[idx] +
                    selectedEnum * SND_DEV_NAME_LEN, 0, SND_DEV_NAME_LEN);

            strncpy (gControlInfo.enumNames[idx] + selectedEnum * SND_DEV_NAME_LEN,
                     ctrlInfo->value.enumerated.name, SND_DEV_NAME_LEN);
            }
        }

    return OK;
    }

LOCAL STATUS getCtrlInfoEnum(int32_t fd, VX_SND_CTRL_INFO *ctrlInfo)
    {
    uint32_t i;
    struct vxSndCtrlId * ctrlId;
    VX_SND_CTRL_INFO     ctrlInfoEnum;

    ctrlId = &ctrlInfo->id;
    int32_t ret;

    memcpy (&ctrlInfoEnum.id, ctrlId, sizeof (struct vxSndCtrlId));

    for (i = 0; i < ctrlInfo->value.enumerated.itemSum; i++)
        {
        ctrlInfoEnum.value.enumerated.itemSelected = i;

        ret = ioctl (fd, VX_SND_CMD_CTRL_INFO_GET, &ctrlInfoEnum);
        if (ret < 0)
            {
            VX_SND_CTRL_MSG("to get control info error\n");
            return ERROR;
            }
#ifdef PRINT_CTRL_INFO_FUNC_INFO
        VX_SND_CTRL_MSG("itemSum: %u\n",
        ctrlInfoEnum.value.enumerated.itemSum);
        VX_SND_CTRL_MSG("itemSelected: %u\n",
        ctrlInfoEnum.value.enumerated.itemSelected);
        VX_SND_CTRL_MSG("name: %s\n",
        ctrlInfoEnum.value.enumerated.name);
#endif

        recordCtrlInfo (&ctrlInfoEnum);
        }

    return OK;
    }

LOCAL STATUS getCtrlListInfo (int fd)
    {
    uint32_t             ret;
    int32_t              size;
    uint32_t             i;
#ifdef PRINT_CTRL_LIST_INFO
    struct vxSndCtrlId * ctrlId;
#endif
    VX_SND_CTRL_INFO     ctrlInfo;
    LOCAL char *         ctrllListNames = NULL;

   // if (listInfoGeted)
    //    return OK;

    /* initial control list structure */

    gControlList.offset = 0;
    gControlList.space = CONTROL_LIST_ID_MAX;
    gControlList.used = 0;
    gControlList.count = CONTROL_LIST_ID_MAX;

    /* allocate vxSndCtrlId memory */

    size = sizeof(struct vxSndCtrlId) * CONTROL_LIST_ID_MAX;
    gControlList.getBuf = (struct vxSndCtrlId *) calloc (size, 1);
    if (gControlList.getBuf == NULL)
        {
        VX_SND_CTRL_MSG ("malloc control list getBuf error\n");
        return ERROR;
        }

    /* allocate name memory for control list */

    size = SND_DEV_NAME_LEN * CONTROL_LIST_ID_MAX;
    ctrllListNames = (char *) calloc (size, 1);
    if (ctrllListNames == NULL)
        {
        VX_SND_CTRL_MSG ("malloc space for control list names failed\n");
        goto errOut;
        }

    for (i= 0; i < CONTROL_LIST_ID_MAX; i++)
        {
        gControlList.getBuf[i].name = ctrllListNames + (i * SND_DEV_NAME_LEN);
        }

    /* ioctl(): get control list information */

    ret = ioctl (fd, VX_SND_CMD_CTRL_LIST_GET, (_Vx_ioctl_arg_t) &gControlList);
    if (ret != 0)
        {
        VX_SND_CTRL_MSG ("ioctl(VX_SND_CMD_CTRL_LIST_GET): "
                         "get control list error\n");
        goto errOut;
        }

#ifdef PRINT_CTRL_LIST_INFO

    /* print control list information */

    VX_SND_CTRL_MSG ("\n%s: has %d controls overall\n=======================\n",
                     VX_SND_CTRL_DEV_DEFAULT, gControlList.used);
    for (i = 0; i < gControlList.used; i++)
        {
        ctrlId = &gControlList.getBuf[i];
        VX_SND_CTRL_MSG ("%s:\n",                 ctrlId->name);
        VX_SND_CTRL_MSG ("\tnumId:         %u\n", ctrlId->numId);
        VX_SND_CTRL_MSG ("\taccess:        %u\n", ctrlId->access);
        VX_SND_CTRL_MSG ("\tiface:         %u\n", ctrlId->iface);
        VX_SND_CTRL_MSG ("\tdeviceNum:     %u\n", ctrlId->deviceNum);
        VX_SND_CTRL_MSG ("\tsubstreamNum:  %u\n", ctrlId->substreamNum);
        }
    VX_SND_CTRL_MSG ("\n");

#endif

    for (i = 0; i < gControlList.used; i++)
        {
        memset (&ctrlInfo, 0, sizeof (VX_SND_CTRL_INFO));
        memcpy (&ctrlInfo.id, &gControlList.getBuf[i], sizeof (VX_SND_CTRL_ID));

        /* first info() call of current control */

        ret = ioctl (fd, VX_SND_CMD_CTRL_INFO_GET, &ctrlInfo);
        if (ret < 0)
            {
            VX_SND_CTRL_MSG ("%s ioctl(VX_SND_CMD_CTRL_INFO_GET) error\n",
                             ctrlInfo.id.name);
            goto errOut;
            }

#ifdef PRINT_CTRL_INFO_FUNC_INFO
        VX_SND_CTRL_MSG ("\n\n");

        VX_SND_CTRL_MSG ("numId:  %u\n", ctrlInfo.id.numId);
        VX_SND_CTRL_MSG ("type:  %u\n", ctrlInfo.type);
        VX_SND_CTRL_MSG ("access:  %u\n", ctrlInfo.access);
        VX_SND_CTRL_MSG ("rtpId:  %u\n", ctrlInfo.rtpId);
        VX_SND_CTRL_MSG ("count:  %u\n", ctrlInfo.count);
        VX_SND_CTRL_MSG ("\n");
#endif

        /*follow info() call of current control */

        switch (ctrlInfo.type)
            {
            case VX_SND_CTL_DATA_TYPE_BOOLEAN:
            case VX_SND_CTL_DATA_TYPE_INTEGER:
#ifdef PRINT_CTRL_INFO_FUNC_INFO
                VX_SND_CTRL_MSG ("min:  %u\n",
                                ctrlInfo.value.integer32.min);
                VX_SND_CTRL_MSG ("max:  %u\n",
                                ctrlInfo.value.integer32.max);
                VX_SND_CTRL_MSG ("step:  %u\n",
                                ctrlInfo.value.integer32.step);
#endif
                recordCtrlInfo(&ctrlInfo);
                break;
            case VX_SND_CTL_DATA_TYPE_ENUMERATED:
                if (ctrlInfo.value.enumerated.itemSum > 1)
                    {

                    /* set numId to INVALID_CTL_NUMID, so that the ctrl name will be used to get the info */

                    //ctrlInfo.id.numId = INVALID_CTL_NUMID;
                    ret = getCtrlInfoEnum (fd, &ctrlInfo);
                    }
                else
                    {
#ifdef PRINT_CTRL_INFO_FUNC_INFO
                    VX_SND_CTRL_MSG("itemSum: %u\n",
                    ctrlInfo.value.enumerated.itemSum);
                    VX_SND_CTRL_MSG("itemSelected: %u\n",
                    ctrlInfo.value.enumerated.itemSelected);
                    VX_SND_CTRL_MSG("name: %s\n",
                    ctrlInfo.value.enumerated.name);
#endif
                    ret = recordCtrlInfo (&ctrlInfo);
                    }

                if (ret != OK)
                    {
                    goto errOut;
                    }
                break;
            case VX_SND_CTL_DATA_TYPE_BYTES_ARRAY:
#ifdef PRINT_CTRL_INFO_FUNC_INFO
                VX_SND_CTRL_MSG("value type is bytes array\n");
#endif
                break;
            case VX_SND_CTL_DATA_TYPE_INTEGER64:
#ifdef PRINT_CTRL_INFO_FUNC_INFO
                VX_SND_CTRL_MSG ("min:  %llu\n",
                                ctrlInfo.value.integer64.min);
                VX_SND_CTRL_MSG ("max:  %llu\n",
                                ctrlInfo.value.integer64.max);
                VX_SND_CTRL_MSG ("step:  %llu\n",
                                ctrlInfo.value.integer64.step);
#endif
                recordCtrlInfo(&ctrlInfo);
                break;
            default:
                break;
            }
        }

    listInfoGeted = TRUE;

    return OK;

errOut:
    if (ctrllListNames == NULL)
        {
        free (ctrllListNames);
        }

    free (gControlList.getBuf);

    return ERROR;
    }

LOCAL STATUS configCtrlDev(int fd)
    {
    uint32_t i, j, k;
    uint32_t num = sizeof (gAboxAudioConf) / sizeof (CTRL_CONFIG_INFO_CONST);
    CTRL_CONFIG_INFO_CONST *ctrlConf = NULL;
    VX_SND_CTRL_INFO *ctrlInfo = NULL;

    VX_SND_CTRL_TLV *tlvInfo = NULL;
    STATUS ret;
    BOOL foundItemName = FALSE;
    uint32_t selectedNum;

    //if (defaultConfigured)
   //     return OK;

    for (i = 0; i < num; i++)
        {
        ctrlConf = &gAboxAudioConf[i];

        /* find the specified control info */

        for (j = 0; j < gControlInfo.cnt; j++)
            {
            VX_SND_CTRL_DATA_VAL dataVal;

            ctrlInfo = &gControlInfo.ctrlInfo[j];
            if (strncmp (ctrlConf->ctrlName, ctrlInfo->id.name, SND_DEV_NAME_LEN) != 0)
                {
                /* not match */
                //VX_SND_CTRL_MSG("control not match: %s vs %s\n", ctrlConf->ctrlName, ctrlInfo->id.name);

                continue;
                }

            memset ((void *)&dataVal, 0, sizeof (VX_SND_CTRL_DATA_VAL));
            memcpy (&dataVal.id, &ctrlInfo->id, sizeof (VX_SND_CTRL_ID));

            if (ctrlInfo->type == VX_SND_CTL_DATA_TYPE_INTEGER)
                {
                /* for INTEGER value config */

                ret = ioctl (fd, VX_SND_CMD_CTRL_GET, (_Vx_ioctl_arg_t) &dataVal);
                if (ret != 0)
                    {
                    VX_SND_CTRL_MSG("to get ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                /* for Volume control */

                if (strstr (dataVal.id.name, "Volume") != NULL)
                    {
                    /* do tlv read opt to just get some info */

                    tlvInfo = (VX_SND_CTRL_TLV *)calloc (AUDIO_TEST_TLV_BUF_LEN + sizeof (VX_SND_CTRL_TLV), 1);
                    tlvInfo->numId = ctrlInfo->id.numId;
                    tlvInfo->length = AUDIO_TEST_TLV_BUF_LEN;
                    ret = ioctl (fd, VX_SND_CMD_TLV_READ, (_Vx_ioctl_arg_t)tlvInfo);
                    if (ret != 0)
                        {
                        VX_SND_CTRL_MSG("to get ctrl (%s) TLV value failed\n", dataVal.id.name);
                        return ERROR;
                        }
                    if (tlvInfo->tlvBuf[1] > AUDIO_TEST_TLV_BUF_LEN)
                        {
                        VX_SND_CTRL_MSG("tlv buffer size is too small\n");
                        }
#ifdef PRINT_CTRL_LIST_INFO
                    VX_SND_CTRL_MSG("tlv type 0x%08x, len 0x%08x, value 0x%08x\n", tlvInfo->tlvBuf[0],
                                   tlvInfo->tlvBuf[1], tlvInfo->tlvBuf[2]);
#endif
                    free (tlvInfo);
                    }

                /* value[1] for right, value[0] for left */
                dataVal.value.integer32.value[0] = ctrlConf->value.integer32.value;
                dataVal.value.integer32.value[1] = ctrlConf->value.integer32.value;
                ret = ioctl (fd, VX_SND_CMD_CTRL_PUT, (_Vx_ioctl_arg_t) &dataVal);
                if (ret != 0)
                    {
                    VX_SND_CTRL_MSG("to set ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                /* verify if set successfully */

                dataVal.value.integer32.value[0] = ~0;
                dataVal.value.integer32.value[1] = ~0;

                ret = ioctl (fd, VX_SND_CMD_CTRL_GET, (_Vx_ioctl_arg_t) &dataVal);
                if (ret != 0)
                    {
                    VX_SND_CTRL_MSG("to get ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                if (dataVal.value.integer32.value[0] != ctrlConf->value.integer32.value)
                    {
                    VX_SND_CTRL_MSG ("%s set failed, set: %d, get %d\n",
                                     dataVal.id.name,
                                     ctrlConf->value.integer32.value,
                                     dataVal.value.integer32.value[0]);
                    return ERROR;
                    }
                break;
                }
            else if (ctrlInfo->type == VX_SND_CTL_DATA_TYPE_ENUMERATED) /* for enumerate type */
                {

                /* find specified enum */

                foundItemName = FALSE;

                for (k = 0; k < ctrlInfo->value.enumerated.itemSum; k++)
                    {
                    if (strncmp (gControlInfo.enumNames[j] + k * SND_DEV_NAME_LEN,
                                ctrlConf->value.enumerated.value,
                                SND_DEV_NAME_LEN) == 0)
                        {
                        foundItemName = TRUE;
                        selectedNum   = k;
                        }
                    }

                if (!foundItemName)
                    {
                    VX_SND_CTRL_MSG ("control %s has no item %s\n",
                                     ctrlInfo->id.name,
                                     ctrlConf->value.enumerated.value);
                    return ERROR;
                    }

                dataVal.value.enumerated.item[0] = selectedNum;
                dataVal.value.enumerated.item[1] = selectedNum;
                //VX_SND_CTRL_MSG("item name %s, item number %u\n",
                 //               ctrlConf->value.enumerated.value,
                 //               selectedNum);

                ret = ioctl(fd, VX_SND_CMD_CTRL_PUT, (_Vx_ioctl_arg_t)&dataVal);
                if (ret != 0)
                    {
                    VX_SND_CTRL_MSG("to set ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                /* verify if set successfully */

                dataVal.value.enumerated.item[0] = ~0;
                dataVal.value.enumerated.item[1] = ~0;

                ret = ioctl (fd, VX_SND_CMD_CTRL_GET, (_Vx_ioctl_arg_t) &dataVal);
                if (ret != 0)
                    {
                    VX_SND_CTRL_MSG("to get ctrl (%s) value failed\n", dataVal.id.name);
                    return ERROR;
                    }

                if (dataVal.value.enumerated.item[0] != selectedNum)
                    {
                    VX_SND_CTRL_MSG ("%s set failed, set: %s, get %s\n",
                                     dataVal.id.name,
                                     gControlInfo.enumNames[j] + selectedNum * SND_DEV_NAME_LEN,
                                     gControlInfo.enumNames[j] + dataVal.value.enumerated.item[0] * SND_DEV_NAME_LEN
                                     );
                    }
                break;
                }
            else
                {
                VX_SND_CTRL_MSG ("ctrl (%s)'s type %d not support now\n",
                                 ctrlInfo->id.name, ctrlInfo->type);
                return ERROR;
                }
            }

        if (j == gControlInfo.cnt)
            {
            VX_SND_CTRL_MSG("ctrl device (%s) not found\n", ctrlConf->ctrlName);
            return ERROR;
            }

        }

    defaultConfigured = TRUE;

    return OK;
    }

STATUS showAllControlInfo
    (
    uint32_t content
    )
    {
    uint32_t             idx;
    uint32_t             itemIdx;
    struct vxSndCtrlId * pId;
    VX_SND_CTRL_INFO   * pInfo;
    VX_SND_CTRL_DATA_VAL dataVal;
    int                  ret;
    int                  fd;

    if (content == 0)
        {
        return OK;
        }

    fd = open (VX_SND_CTRL_DEV_DEFAULT, O_RDWR);
    if (fd < 0)
        {
        VX_SND_CTRL_MSG ("open %s error\n", VX_SND_CTRL_DEV_DEFAULT);
        return ERROR;
        }

    VX_SND_CTRL_MSG ("\n");

    if (content == 1) // show list info
        {
        for (idx = 0; idx < gControlList.used; idx++)
            {
            pId = &gControlList.getBuf[idx];
            VX_SND_CTRL_MSG ("numId=%d,name=%s\n", pId->numId, pId->name);
            }
        }
    else
        {
        VX_SND_CTRL_MSG ("have %d contorls overall:\n=======================\n",
                gControlInfo.cnt);
        for (idx = 0; idx < gControlInfo.cnt; idx++)
            {
            pInfo = &gControlInfo.ctrlInfo[idx];
            pId   = &pInfo->id;

            memset ((void *)&dataVal, 0, sizeof (VX_SND_CTRL_DATA_VAL));
            memcpy (&dataVal.id, &pInfo->id, sizeof (VX_SND_CTRL_ID));

            ret = ioctl (fd, VX_SND_CMD_CTRL_GET, (_Vx_ioctl_arg_t) &dataVal);
            if (ret < 0)
                {
                //close (fd);
                VX_SND_CTRL_MSG ("failed to get control %s info\n",
                                 pId->name);
                continue;
                //return ERROR;
                }

            if (pInfo->type == VX_SND_CTL_DATA_TYPE_INTEGER)
                {
                VX_SND_CTRL_MSG ("numId=%d,name=%s,value=%d\n",
                        pId->numId, pId->name, dataVal.value.integer32.value[0]);
                }
            else if (pInfo->type == VX_SND_CTL_DATA_TYPE_ENUMERATED)
                {
                VX_SND_CTRL_MSG ("numId=%d,name=%s,value=%d,items=%d\n",
                        pId->numId, pId->name, dataVal.value.enumerated.item[0],
                        pInfo->value.enumerated.itemSum);
                for (itemIdx = 0; itemIdx < pInfo->value.enumerated.itemSum;
                     itemIdx++)
                    {
                    VX_SND_CTRL_MSG ("\tItem #%d '%s'\n", itemIdx,
                            gControlInfo.enumNames[idx] + itemIdx * SND_DEV_NAME_LEN);
                    }
                }
            }
        }

    close (fd);

    return OK;
    }

STATUS vxSndCtrl (uint32_t v)
    {
    int fd;

  //  if (!listInfoGeted)
        {
        memset (&gControlList, 0, sizeof(VX_SND_CTRL_LIST));
        memset (&gControlInfo, 0, sizeof (gControlInfo));
        }

    fd = open (VX_SND_CTRL_DEV_DEFAULT, O_RDWR);
    if (fd < 0)
        {
        VX_SND_CTRL_MSG ("open %s error\n", VX_SND_CTRL_DEV_DEFAULT);
        return ERROR;
        }

    if (getCtrlListInfo (fd) != OK)
        {
        close (fd);
        VX_SND_CTRL_MSG ("to get all ctrl info failed\n");
        return ERROR;
        }

    /* VX_SND_CMD_CTRL_PUT      -> set ctrl attribute */

    if (configCtrlDev (fd) != OK)
        {
        close (fd);
        VX_SND_CTRL_MSG ("to config ctrl device failed\n");
        return ERROR;
        }

    close (fd);

    if (showAllControlInfo (v) != OK)
        return ERROR;

 //   if (showAllControlInfo (1) != OK)
  //      return ERROR;

    return OK;
    }


#ifdef _WRS_KERNEL

STATUS amixer (uint32_t v)
    {
    TASK_ID     tid;
#ifdef _WRS_CONFIG_SMP
    cpuset_t    affinity = 0;
#endif /* _WRS_CONFIG_SMP */

    tid = taskSpawn ("tVxSoundControlDemo", 110, 0, 8192, (FUNCPTR) vxSndCtrl,
                     v, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L);
    if (tid == TASK_ID_ERROR)
        {
        return ERROR;
        }

#ifdef _WRS_CONFIG_SMP
    CPUSET_ZERO (affinity);
    CPUSET_SET (affinity, (vxCpuConfiguredGet () - 1));
    if (taskCpuAffinitySet (tid, affinity) == ERROR)
       {
       (void) taskDelete (tid);
       return ERROR;
       }

    (void) taskActivate (tid);
#endif /* _WRS_CONFIG_SMP */

    return OK;
    }

#else

/*******************************************************************************
*
* main - start of the demo program in RTP mode
*
* This is the RTP mode entry point.
*
* RETURNS: EXIT_SUCCESS, or non-zero if the routine had to abort
*
* ERRNO: N/A
*
* \NOMANUAL
*/
#if 0
int main
    (
    int     argc,
    char *  argv[]
    )
    {
//    if (argc <= 3)
//        {
 //       (void) printf_s ("Error: 3 parameters are required"
 //                          " in cameraCap function\n");
  //      return ERROR;
  //      }

    return vxSndCtrl ();
    }
#endif
#endif /* _WRS_KERNEL */


