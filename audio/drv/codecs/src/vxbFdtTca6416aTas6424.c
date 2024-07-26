/* vxbFdtTca6416aTas6424.c - Tca6416a driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#include <vxWorks.h>
#include <string.h>
#include <semLib.h>
#include <hwif/vxbus/vxbLib.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <hwif/buslib/vxbI2cLib.h>
#include "vxbFdtTca6416aTas6424.h"

#undef TCA6416A_DEBUG
#ifdef TCA6416A_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t tca6416aDbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((tca6416aDbgMask & (mask)) || ((mask) == DBG_ALL))           \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* TCA6416A_DEBUG */

/* defines */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define TCA6416A_CONFIG_0_INIT_VAL  (0xDB)
#define TCA6416A_CONFIG_1_INIT_VAL  (0x06)

#define V920_SADK_AMP3_STANDBY      (4)
#define V920_SADK_AMP3_MUTE         (5)

typedef enum ampId
    {
    AMP0,
    AMP1,
    AMP2,
    AMP3,
    } AMP_ID;
struct reg_default {
    unsigned int reg;
    unsigned int def;
};

/* forward declarations */

LOCAL STATUS tca6416aProbe (VXB_DEV_ID pDev);
LOCAL STATUS tca6416aAttach (VXB_DEV_ID pDev);

/* locals */
static const struct reg_default tas6424_reg_defaults[] = {
    { TCA6416A_OUT_PORT_0,      0xff },
    { TCA6416A_OUT_PORT_1,      0xff },
    { TCA6416A_POLAR_INVERSION_0,   0x00 },
    { TCA6416A_POLAR_INVERSION_1,   0x00 },
    { TCA6416A_CONFIG_0,        0xDB },
    { TCA6416A_CONFIG_1,        0x06 },
};

LOCAL VXB_DRV_METHOD tca6416aMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), tca6416aProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), tca6416aAttach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY tca6416aMatch[] =
    {
    {
    "ti,tca6416a_audioexp",
    NULL
    },
    {}                              /* empty terminated list */
    };

LOCAL TCA6416A_DATA *   pTca6416aData = NULL; //only one dev in the system

/* globals */

VXB_DRV tca6416aDrv =
    {
    {NULL},
    TCA6416A_DRV_NAME,                  /* Name */
    "Tca6416a FDT driver",              /* Description */
    VXB_BUSID_FDT,                      /* Class */
    0,                                  /* Flags */
    0,                                  /* Reference count */
    tca6416aMethodList                  /* Method table */
    };

VXB_DRV_DEF(tca6416aDrv)

LOCAL STATUS tca6416aRegRead
    (
    VXB_DEV_ID      pDev,
    UINT8           regAddr,
    UINT8 *         pRegVal
    )
    {
    I2C_MSG         msgs[2];
    int32_t         num;
    TCA6416A_DATA * pTas6424AudIoData;
    STATUS          ret;

    pTas6424AudIoData = (TCA6416A_DATA *) vxbDevSoftcGet (pDev);
    if (pTas6424AudIoData == NULL)
        {
        return ERROR;
        }

    (void) semTake (pTas6424AudIoData->i2cMutex, WAIT_FOREVER);

    memset (msgs, 0, sizeof (I2C_MSG) * 2);
    num = 0;

    /* write reg address */

    msgs[num].addr  = pTas6424AudIoData->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_WR;
    msgs[num].buf   = &regAddr;
    msgs[num].len   = 1;
    num++;

    /* read reg data */

    msgs[num].addr  = pTas6424AudIoData->i2cDevAddr;
    msgs[num].scl   = 0;           /* use controller's default scl */
    msgs[num].flags = I2C_M_RD;
    msgs[num].buf   = pRegVal;
    msgs[num].len   = 1;
    num++;

    ret = vxbI2cDevXfer (pDev, &msgs[0], num);

    (void) semGive (pTas6424AudIoData->i2cMutex);

    return ret;
    }

LOCAL STATUS tca6416aRegWrite
    (
    VXB_DEV_ID      pDev,
    UINT8           regAddr,
    UINT8           pRegVal
    )
    {
    I2C_MSG         msg;
    UINT8           msgBuf[2];
    TCA6416A_DATA * pTas6424AudIoData;
    STATUS          ret;

    pTas6424AudIoData = (TCA6416A_DATA *) vxbDevSoftcGet (pDev);
    if (pTas6424AudIoData == NULL)
        {
        return ERROR;
        }

    (void) semTake (pTas6424AudIoData->i2cMutex, WAIT_FOREVER);

    memset (&msg, 0, sizeof (I2C_MSG));

    msgBuf[0] = regAddr;
    msgBuf[1] = pRegVal;

    /* write reg address and data */

    msg.addr    = pTas6424AudIoData->i2cDevAddr;
    msg.scl     = 0;        /* use controller's default scl */
    msg.flags   = I2C_M_WR;
    msg.buf     = msgBuf;
    msg.len     = 2;

    ret = vxbI2cDevXfer (pDev, &msg, 1);

    (void) semGive (pTas6424AudIoData->i2cMutex);

    return ret;
    }

/******************************************************************************
*
* tca6416aProbe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS tca6416aProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, tca6416aMatch, NULL);
    }

/******************************************************************************
*
* tca6416aAttach - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

LOCAL STATUS tca6416aAttach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;
    TCA6416A_DATA * pTas6424AudIoData;
    VXB_RESOURCE *  pResMem = NULL;
    uint32_t        idx;

    if (pDev == NULL)
        {
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        return ERROR;
        }

    pTas6424AudIoData = (TCA6416A_DATA *) vxbMemAlloc (sizeof (TCA6416A_DATA));
    if (pTas6424AudIoData == NULL)
        {
        return ERROR;
        }

    pTca6416aData = pTas6424AudIoData;
    vxbDevSoftcSet (pDev, pTas6424AudIoData);
    pTas6424AudIoData->pDev = pDev;

    pTas6424AudIoData->i2cMutex = semMCreate (SEM_Q_PRIORITY
                                              | SEM_DELETE_SAFE
                                              | SEM_INVERSION_SAFE);
    if (pTas6424AudIoData->i2cMutex == SEM_ID_NULL)
        {
        goto errOut;
        }

    pResMem = vxbResourceAlloc (pDev, VXB_RES_MEMORY, 0);
    if ((pResMem == NULL) || (pResMem->pRes == NULL))
        {
        goto errOut;
        }
    pTas6424AudIoData->i2cDevAddr =
        (UINT16)(((VXB_RESOURCE_ADR *)(pResMem->pRes))->virtAddr);

    /* write initial values to registers */

    for (idx = 0; idx < NELEMENTS (tas6424_reg_defaults); idx++)
        {
        if (tca6416aRegWrite (pDev,
                                   (uint8_t) tas6424_reg_defaults[idx].reg,
                                   (uint8_t) tas6424_reg_defaults[idx].def)
                                   != OK)
            {
            DBG_MSG (DBG_ERR, "failed to write reg[%02xh] = 0x%02x\n",
                     (uint8_t) tas6424_reg_defaults[idx].reg,
                     (uint8_t) tas6424_reg_defaults[idx].def);
            goto errOut;
            }
        }


    if (tca6416aRegWrite (pDev,
                          TCA6416A_CONFIG_0, TCA6416A_CONFIG_0_INIT_VAL) != OK)
        {
        goto errOut;
        }

    if (tca6416aRegWrite (pDev,
                          TCA6416A_CONFIG_1, TCA6416A_CONFIG_1_INIT_VAL) != OK)
        {
        goto errOut;
        }

    return OK;

errOut:
    if (pResMem != NULL)
        {
        (void) vxbResourceFree (pDev, pResMem);
        }

    if (pTas6424AudIoData->i2cMutex != SEM_ID_NULL)
        {
        (void) semDelete (pTas6424AudIoData->i2cMutex);
        }

    if (pTas6424AudIoData != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pTas6424AudIoData);
        }

    return ERROR;
    }

/* APIs */

STATUS tca6416aConfigAmpStandby
    (
    uint32_t    ampId,
    uint32_t    status
    )
    {
    UINT8       addr;
    UINT8       mask;
    UINT8       value;
    VXB_DEV_ID  pDev;
    UINT8       regVal;

    if (pTca6416aData == NULL)
        {
        DBG_MSG (DBG_ERR, "Tac6416a is not initialized\n");
        return ERROR;
        }

    pDev = pTca6416aData->pDev;

    DBG_MSG (DBG_INFO, "ampId=%d status=%d\n", ampId, status);

    switch (ampId)
        {
        case AMP3:
            addr = TCA6416A_OUT_PORT_0;
            mask = (0x1 << V920_SADK_AMP3_STANDBY);
            value = (status ?
                     (0 << V920_SADK_AMP3_STANDBY) :
                     (1 << V920_SADK_AMP3_STANDBY));
            break;

        default:
            DBG_MSG (DBG_ERR, "Wrong AMP ID: %d\n", ampId);
            return ERROR;
        }

    if (tca6416aRegRead (pDev, addr, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "Unable to read amp%d reg:0x%x\n", ampId, addr);
        return ERROR;
        }

    regVal &= ~mask;
    regVal |= value;

    if (tca6416aRegWrite (pDev, addr, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "Unable to write amp%d reg:0x%x\n", ampId, addr);
        return ERROR;
        }

    return OK;
    }

STATUS tca6416aConfigAmpMute
    (
    uint32_t    ampId,
    uint32_t    mute
    )
    {
    UINT8       addr;
    UINT8       mask;
    UINT8       value;
    VXB_DEV_ID  pDev;
    UINT8       regVal;

    if (pTca6416aData == NULL)
        {
        DBG_MSG (DBG_ERR, "Tac6416a is not initialized\n");
        return ERROR;
        }

    pDev = pTca6416aData->pDev;

    DBG_MSG (DBG_INFO, "ampId=%d mute=%d\n", ampId, mute);

    switch (ampId)
        {
        case AMP3:
            addr = TCA6416A_OUT_PORT_0;
            mask = (0x1 << V920_SADK_AMP3_MUTE);
            value = (mute ?
                     (0 << V920_SADK_AMP3_MUTE) :
                     (1 << V920_SADK_AMP3_MUTE));
            break;

        default:
            DBG_MSG (DBG_ERR, "Wrong AMP ID : %d\n", ampId);
            return ERROR;
        }

    if (tca6416aRegRead (pDev, addr, &regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "Unable to read amp%d reg:0x%x\n", ampId, addr);
        return ERROR;
        }

    regVal &= ~mask;
    regVal |= value;

    if (tca6416aRegWrite (pDev, addr, regVal) != OK)
        {
        DBG_MSG (DBG_ERR, "Unable to write amp%d reg:0x%x\n", ampId, addr);
        return ERROR;
        }

    return OK;
    }

