/* vxSoundCore.c - VxWorks Sound Core Library */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This is the VxSound core.
*/

/* includes */

#include <cacheLib.h>
#include <iosLib.h>
#include <sysLib.h>
#include <scMemVal.h>
#include <private/iosLibP.h>
#include <pmapLib.h>
#include <vxSoundCore.h>
#include <pcm.h>
#include <control.h>

#undef DEBUG_VX_SND_CORE

#ifdef DEBUG_VX_SND_CORE
# include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_CORE_DBG_OFF          0x00000000U
#define VX_SND_CORE_DBG_ERR          0x00000001U
#define VX_SND_CORE_DBG_INFO         0x00000002U
#define VX_SND_CORE_DBG_VERBOSE      0x00000004U
#define VX_SND_CORE_DBG_ALL          0xffffffffU
UINT32 vxSndCoreDbgMask = VX_SND_CORE_DBG_ALL;

#define VX_SND_CORE_MSG(mask, fmt, ...)                                  \
    do                                                                  \
        {                                                               \
        if ((vxSndCoreDbgMask & (mask)))                                \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_CORE LIB: [%s,%d] "fmt, __func__,  \
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define SND_CORE_MODE_INFO(...)                                           \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define SND_CORE_ERR(fmt, ...)    \
                    VX_SND_CORE_MSG(VX_SND_CORE_DBG_ERR,     fmt, ##__VA_ARGS__)
#define SND_CORE_INFO(fmt, ...)   \
                    VX_SND_CORE_MSG(VX_SND_CORE_DBG_INFO,    fmt, ##__VA_ARGS__)
#define SND_CORE_DBG(fmt, ...)    \
                    VX_SND_CORE_MSG(VX_SND_CORE_DBG_VERBOSE, fmt, ##__VA_ARGS__)
#else /* DEBUG_VX_SND_CORE */
#define SND_CORE_ERR(x...)      do { } while (FALSE)
#define SND_CORE_INFO(x...)     do { } while (FALSE)
#define SND_CORE_DBG(x...)      do { } while (FALSE)
#define SND_CORE_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_CORE */

/* defines */

LOCAL int gCardNumber = 0;

/* typedefs */

/* forward declarations */

#ifdef _WRS_CONFIG_RTP

#endif

/* locals */

/*******************************************************************************
*
* vxSoundCoreInit - install the sound device framework
*
* This routine installs the sound device framework.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void vxSoundCoreInit (void)
    {
    SND_CORE_DBG ("vxSoundCoreInit()\n");
    if (sysTimestampEnable() == ERROR)
        {
        SND_CORE_ERR ("failed to enable timestamp\n");
        return;
        }

    sndPcmInit ();
    sndCtrlInit ();
    }

/*******************************************************************************
*
* vxSndDevScMemValidate - validate address range passed to a system call routine
*
* This routine validates address range passed to system call. Because some
* address passed on to kernel by ioctl argument can accept address pointer or
* NULL in the driver implementation, the ioctl can only be registered as _IO
* type since a NULL pointer passed on to scMemValidate() will return ERROR.
* Therefore, some address validation like the argument of VX_IOCG_AUDIO_PCM,
* implements its address validation in the driver.
*
* RETURNS: OK, or ERROR if failed to validate address range
*
* ERRNO: N/A
*/

STATUS vxSndDevScMemValidate
    (
    const void * addr,
    int          cmd,
    BOOL         syscall
    )
    {
    #ifdef _WRS_CONFIG_RTP
    size_t size = IOCPARM_LEN (cmd);

    if (syscall)
        {
        if (size == 0)
            {
            return OK;
            }

        if (_func_scMemValidate != NULL)
            {
            if (_func_scMemValidate (addr, size, cmd & 0xff) == OK)
                {
                return OK;
                }
            }

        SND_CORE_ERR ("cmd idx 0x%x _func_scMemValidate() failed\n", cmd & 0xff);

        return ERROR;
        }
    #endif
    return OK;
    }

/*******************************************************************************
*
* sndDevRegister - register a sound device to I/O system
*
* This routine registers an sound device to I/O system.
* For registering successfully, please make sure the required sensor device has
* been registered to sensor list.
*
* RETURNS: OK when device successfully registered; otherwise ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sndDevRegister
    (
    DEV_HDR *     pDevHdr,
    char *        name,
    int           drvNum
    )
    {
    if (iosDevAddX (pDevHdr, name, drvNum, IODEV_DIRLIKE) == ERROR)
        {
        SND_CORE_ERR ("failed to register sound device %s\n", name);
        return ERROR;
        }

    return OK;
    }

STATUS snd_device_register_all
    (
    SND_CARD * card
    )
    {
    DL_NODE * pNode;

    pNode = DLL_FIRST (&card->deviceList);
    while (pNode != NULL)
        {
        SND_DEVICE * sndDev = (SND_DEVICE *) pNode;
        if (sndDev->regFunc == NULL)
            {
            SND_CORE_ERR ("sound device should have registration function\n");
            return ERROR;
            }

        if (sndDev->type != VX_SND_DEV_PCM &&
            sndDev->type != VX_SND_DEV_CONTROL)
            {
            SND_CORE_ERR ("sound device type %d not support now\n",
                          sndDev->type);
            return ERROR;
            }

        if (sndDev->regFunc (sndDev) != OK)
            {
            SND_CORE_ERR ("failed to register sound device, type is %d\n",
                          sndDev->type);
            return ERROR;
            }

        pNode = DLL_NEXT (pNode);
        }

    return OK;
    }

STATUS snd_card_register
    (
    SND_CARD * card
    )
    {
    if (snd_device_register_all (card) != OK)
        {
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* vxSndDevCreate - create a sound device
*
* This routine creates a sound device and initializes it.
*
* RETURNS: OK, or ERROR if failed to create
*
* ERRNO: N/A
*
* \NOMANUAL
*/
//snd_device_new
STATUS vxSndDevCreate
    (
    SND_CARD *      card,
    VX_SND_DEV_TYPE type,
    void *          data,
    SND_DEV_REG_PTR func,
    SND_DEVICE **   ppSndDev
    )
    {
    SND_DEVICE * sndDev;
    DL_NODE *    pNode;

    sndDev = (SND_DEVICE *) vxbMemAlloc (sizeof (SND_DEVICE));
    if (sndDev == NULL)
        {
        SND_CORE_ERR ("failed to alloc sndDev\n");
        return ERROR;
        }

    sndDev->card    = card;
    sndDev->data    = data;
    sndDev->type    = type;
    sndDev->regFunc = func;

    /* add to card->deviceList*/

    pNode = DLL_FIRST (&card->deviceList);
    while (pNode != NULL)
        {
        SND_DEVICE * temp = (SND_DEVICE *) pNode;
        if (sndDev->card == temp->card &&
            sndDev->data == temp->data &&
            sndDev->type == temp->type)
            {
            SND_CORE_ERR ("snd device already exists\n");
            goto errOut;
            }

        pNode = DLL_NEXT (pNode);
        }

    dllAdd (&card->deviceList, &sndDev->node);

    if (ppSndDev != NULL)
        {
        *ppSndDev = sndDev;
        }

    SND_CORE_DBG ("sound device (Type-%d) is added to card->deviceList\n",
                  type);

    return OK;

errOut:
    vxbMemFree (sndDev);
    return ERROR;
    }

STATUS snd_card_new
    (
    VXB_DEV_ID  pDev,
    SND_CARD ** ppCard
    )
    {
    SND_CARD * pSndCard;

    /* just support one card, no card related mutex now */

    pSndCard = (SND_CARD *) vxbMemAlloc (sizeof (SND_CARD));
    if (pSndCard == NULL)
        {
        SND_CORE_ERR ("failed to allocate memory for sound card\n");
        return ERROR;
        }

    DLL_INIT(&pSndCard->deviceList);
    DLL_INIT(&pSndCard->controlList);

    pSndCard->instantiated = FALSE;
    pSndCard->cardMutex = semMCreate (SEM_Q_PRIORITY
                                            | SEM_DELETE_SAFE
                                            | SEM_INVERSION_SAFE);
    pSndCard->controlListSem = semBCreate (SEM_Q_FIFO, SEM_FULL);

    pSndCard->pDev    = pDev;
    pSndCard->cardNum = gCardNumber++;

    (void) snprintf_s (pSndCard->name, SND_DEV_NAME_LEN, "card%d",
                       pSndCard->cardNum);

    if ( vxSndCtrlCreate (pSndCard) != OK)
        {
        SND_CORE_ERR ("failed to create control sound device\n");
        return ERROR;
        }

    *ppCard = pSndCard;

    SND_CORE_DBG ("create sound card %s successfully\n", pSndCard->name);

    return OK;
    }

void snd_card_free
    (
    SND_CARD * card
    )
    {

    //to do: other resource release

    vxbMemFree (card);
    }

/*******************************************************************************
*
* sndDummyDevRegister - register a dummy sound device
*
* This is a dummy sound device registration routine.
*
* RETURNS: OK, or ERROR if the device failed to register
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sndDummyDevRegister
    (
    SND_DEVICE * sndDev
    )
    {
    SND_CORE_DBG ("register a dummy sound device\n");

    if (sndDev == NULL)
        {
        return ERROR;
        }

    return OK;
    }

/*******************************************************************************
*
* roundupPowerOfTwoPri - round up a number to the power of two
*
* This routine rounds up a number to the power of two.
*
* RETURNS: the number
*
* ERRNO: N/A
*/

uint32_t roundupPowerOfTwoPri
    (
    uint32_t n
    )
    {
    int i;

    if (n == 0)
        return n;

    for (i = 0; i < 32; i ++)
        {
        if (n == (1 << i))
            return n;
        if ((n > (1 << i)) && (n <= (1 << (i + 1))))
            return (1 << (i + 1));
        }
    return 0;
    }

