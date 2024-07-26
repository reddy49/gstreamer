/* control.h - Sound Control Header File */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file defines the Control functions and structures
*/

#ifndef __INCsoundControlh
#define __INCsoundControlh

/* includes */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <ioLib.h>
#include <errnoLib.h>
#include <semLib.h>
#include <stdio.h>
#include <sysLib.h>
#ifdef _WRS_KERNEL
#include <hwif/vxBus.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#endif
#include <msgQLib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <string.h>
#include <sys/ioctl.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus  */

/* defines */

/* ioctl commands */

#ifndef VX_IOCG_AUDIO_CTL
#define VX_IOCG_AUDIO_CTL 'T'
#endif

#define VX_SND_CMD_CTRL_CORE_VER    _IOR(VX_IOCG_AUDIO_CTL,  0x00, int)
#define VX_SND_CMD_CTRL_APP_VER     _IOW(VX_IOCG_AUDIO_CTL,  0x01, int)
#define VX_SND_CMD_CARD_INFO        _IOR(VX_IOCG_AUDIO_CTL,  0x02, \
                                         VX_SND_CTRL_CARD_INFO)
#define VX_SND_CMD_CTRL_LIST_GET    _IOWR(VX_IOCG_AUDIO_CTL, 0x03, \
                                          VX_SND_CTRL_LIST)
#define VX_SND_CMD_CTRL_INFO_GET    _IOWR(VX_IOCG_AUDIO_CTL, 0x04, \
                                          VX_SND_CTRL_INFO)
#define VX_SND_CMD_CTRL_GET         _IOWR(VX_IOCG_AUDIO_CTL, 0x05, \
                                          VX_SND_CTRL_DATA_VAL)
#define VX_SND_CMD_CTRL_PUT         _IOWR(VX_IOCG_AUDIO_CTL, 0x06, \
                                          VX_SND_CTRL_DATA_VAL)
#define VX_SND_CMD_CTRL_LOCK        _IOW(VX_IOCG_AUDIO_CTL,  0x07, \
                                         VX_SND_CTRL_ID)
#define VX_SND_CMD_CTRL_UNLOCK      _IOW(VX_IOCG_AUDIO_CTL,  0x08, \
                                         VX_SND_CTRL_ID)
#define VX_SND_CMD_SUBSCRIBE_EVENTS _IOWR(VX_IOCG_AUDIO_CTL, 0x09, int)
#define VX_SND_CMD_TLV_READ         _IOWR(VX_IOCG_AUDIO_CTL, 0x0a, \
                                          VX_SND_CTRL_TLV)

#define VX_SND_CTRL_CORE_VERSION  VX_SND_MODULE_VERSION(0, 0, 1)

struct vxSndMixerControl;
struct vxSndEnum;
struct vxSndControl;

/* typedef */

#define INVALID_CTL_NUMID               0

#define VX_SND_CTRL_ACCESS_READ         (1u << 0)
#define VX_SND_CTRL_ACCESS_WRITE        (1u << 1)
#define VX_SND_CTRL_ACCESS_READWRITE    \
        (VX_SND_CTRL_ACCESS_READ | VX_SND_CTRL_ACCESS_WRITE)
#define SNDRV_CTL_ELEM_ACCESS_VOLATILE  (1u << 2)
#define VX_SND_CTRL_ACCESS_TLV_READ     (1u << 3)
#define VX_SND_CTRL_ACCESS_LOCK         (1u << 4)
#define VX_SND_CTRL_ACCESS_OWNER        (1u << 5)

#define FOR_EACH_CARD_CTRLS(card, pNode) \
        for (pNode = DLL_FIRST(&card->controlList); pNode != NULL; \
             pNode = DLL_NEXT(pNode))

typedef enum vxSndCtrlType
    {
    SNDRV_CTL_TYPE_CARD = 0,
    SNDRV_CTL_TYPE_HWDEP,
    SNDRV_CTL_TYPE_MIXER,
    SNDRV_CTL_TYPE_PCM,
    SNDRV_CTL_TYPE_TYPE_MAX,
    } VX_SND_CTRL_TYPE;

typedef enum vxSndCtrlDataType
    {
    VX_SND_CTL_DATA_TYPE_BOOLEAN = 0,
    VX_SND_CTL_DATA_TYPE_INTEGER,
    VX_SND_CTL_DATA_TYPE_ENUMERATED,
    VX_SND_CTL_DATA_TYPE_BYTES_ARRAY,
    VX_SND_CTL_DATA_TYPE_INTEGER64,
    SNDRV_CTL_DATA_TYPE_MAX,
    } VX_SND_CTRL_DATA_TYPE;

#define SNDRV_CTL_TLV_CONTAINER      0  /* one level down - group of TLVs */
#define SNDRV_CTL_TLV_DB_SCALE       1  /* dB scale */
#define SNDRV_CTL_TLV_DB_LINEAR      2  /* linear volume */
#define SNDRV_CTL_TLV_DB_RANGE       3  /* dB range container */
#define SNDRV_CTL_TLV_DB_MINMAX      4  /* dB scale with min/max */
#define SNDRV_CTL_TLV_DB_MINMAX_MUTE 5  /* dB scale with min/max with mute */

/*
 *     when reg != rreg, reg or rreg as below
 *     +--+---------------+-----------------+
 * MSB |  | L or R value  |                 | LSB
 *     +--+---------------+-----------------+
 *                        |                 |
 *                        |                 |
 *                        |<---- shift ---->|
 *
 *     when reg == rreg, reg as below
 *     +----------------------------------------+
 * MSB | |right value|       |left value|       | LSB
 *     +----------------------------------------+
 *                   |                  |-shift-|
 *                   |                  |       |
 *                   |<---- shiftRight -------->|
 */

typedef struct vxSndMixerControl
    {
    int min;
    int max;
    int platformMax; /* register max value */
    int reg;
    int rreg;
    uint32_t shift; /* shift for regs has only 1 channel, or left channel when 1 reg has 2 channels */
    uint32_t shiftRight; /* shift for right channel when 1 reg has 2 channels */
    uint32_t invert:1;
    uint32_t autodisable:1;
    } VX_SND_MIXER_CTRL;

typedef struct vxSndEnum
    {
    int reg;
    uint8_t shiftLeft;
    uint8_t shiftRight;
    uint32_t itemNum;
    /* workaround: before using mask,
       it should be calculated as 'roundupPowerOfTwo (pEnum->itemNum) - 1' */
    uint32_t mask;
    const char * const * textList;
    const unsigned int * valueList;
    uint32_t autodisable:1;
    } VX_SND_ENUM;

typedef struct vxSndCtrlCardInfo
    {
    int cardNum;
    uint8_t id[16];
    uint8_t driver[16];         /* Driver name */
    uint8_t name[32];           /* Short name of soundcard */
    uint8_t longname[80];       /* name + info text about soundcard */
    } VX_SND_CTRL_CARD_INFO;

typedef struct vxSndCtrlId
    {
    uint32_t numId;         /* numeric identifier, zero = invalid */
    uint32_t access;
    VX_SND_CTRL_TYPE iface; /* interface identifier */
    uint32_t deviceNum;     /* device/client number */
    uint32_t substreamNum;  /* subdevice (substream) number */
    char *  name;
    } VX_SND_CTRL_ID;

typedef struct vxSndCtrlInfo
    {
    struct vxSndCtrlId id;          /* W: control ID */
    VX_SND_CTRL_DATA_TYPE  type;    /* R: value type - VX_SND_CTL_DATA_TYPE_* */
    uint32_t access;                /* R: value access (bitmask) - SNDRV_CTL_ELEM_ACCESS_* */
    uint32_t count;                 /* count of values */
    RTP_ID rtpId;                   /* owner's task of this control */
    union
        {
        struct
            {
            uint32_t min;
            uint32_t max;
            uint32_t step;
            } integer32;
        struct
            {
            uint64_t min;
            uint64_t max;
            uint64_t step;
            } integer64;
        struct
            {
            uint32_t itemSum;               /* R: number of items */
            uint32_t itemSelected;          /* W: item number */
            char name[SND_DEV_NAME_LEN];    /* R: value name */

            } enumerated;
        unsigned char reserved[128];
        } value;
    unsigned char reserved[64];
    } VX_SND_CTRL_INFO;

typedef struct vxSndCtrlList
    {
    uint32_t offset;    /* W: first control ID to get */
    uint32_t space;     /* W: count of control IDs to get */
    uint32_t used;      /* R: count of control IDs set to pids*/
    uint32_t count;     /* R: count of all controls on the card */
    struct vxSndCtrlId  * getBuf; /* R: IDs */
    unsigned char reserved[50];
    } VX_SND_CTRL_LIST;

typedef struct vxSndCtrlDataValue
    {
    struct vxSndCtrlId id;  /* W: control ID */
    union
        {
        union
            {
            uint32_t value[128];
            } integer32;
        union
            {
            uint64_t value[64];
            } integer64;
        union
            {
            uint32_t item[128];
            } enumerated;
        union
            {
            unsigned char data[512];
            } bytes;

        } value;        /* RO */

    } VX_SND_CTRL_DATA_VAL;

typedef struct vxSndCtlTlv
    {
    uint32_t numId;        /* control element numeric identification */
    uint32_t length;       /* in bytes aligned to 4 */
    uint32_t tlvBuf[0];    /* first TLV */
    } VX_SND_CTRL_TLV;

typedef STATUS (*VX_SND_CTL_INFO_PTR) (struct vxSndControl * kcontrol, struct vxSndCtrlInfo * uinfo);
typedef STATUS (*VX_SND_CTL_GET_PTR) (struct vxSndControl * kcontrol, struct vxSndCtrlDataValue * ucontrol);
typedef STATUS (*VX_SND_CTL_PUT_PTR) (struct vxSndControl * kcontrol, struct vxSndCtrlDataValue * ucontrol);
typedef STATUS (*VX_SND_CTL_REG_RW_PTR)(struct vxSndControl *kcontrol, int op_flag, uint32_t size, uint32_t *tlv);

typedef struct vxSndControl
    {
    DL_NODE                node;   /* node should be the first member */
    struct vxSndCtrlId     id;
    VX_SND_CTL_INFO_PTR    info;
    VX_SND_CTL_GET_PTR     get;
    VX_SND_CTL_PUT_PTR     put;
    union
        {
        VX_SND_CTL_REG_RW_PTR *c;
        const uint32_t *       p;
        } tlv;
    uint64_t privateValue; /* private data struct */
    void * privateData;
    void (*privateFree)(struct vxSndControl *kcontrol);
    } VX_SND_CONTROL;

/* function declarations */

#ifdef _WRS_KERNEL

void sndCtrlInit (void);

STATUS sndCtrlDevRegister (SND_DEVICE * sndDev);
STATUS sndCtrlDevUnregister (SND_DEVICE * sndDev);
STATUS vxSndCtrlCreate (SND_CARD * card);
STATUS vxSndControlsAdd (SND_CARD * card, VX_SND_CONTROL * ctrlList,
                         int num, const char * namePrefix, void * privateData);

#endif /* _WRS_KERNEL */

#if __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCsoundControlh */

