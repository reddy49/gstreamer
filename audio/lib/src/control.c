/* control.c - sound card control device driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides a sound card control driver.
*/

/* includes */

#include <cacheLib.h>
#include <iosLib.h>
#include <scMemVal.h>
#include <private/iosLibP.h>
#include <pmapLib.h>
#include <vxSoundCore.h>

#include <control.h>

#undef DEBUG_VX_SND_CTRL

#ifdef DEBUG_VX_SND_CTRL
# include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_CTRL_DBG_OFF          0x00000000U
#define VX_SND_CTRL_DBG_ERR          0x00000001U
#define VX_SND_CTRL_DBG_INFO         0x00000002U
#define VX_SND_CTRL_DBG_VERBOSE      0x00000004U
#define VX_SND_CTRL_DBG_ALL          0xffffffffU
UINT32 vxSndCtrlDbgMask = VX_SND_CTRL_DBG_ALL;

#define VX_SND_CTRL_MSG(mask, fmt, ...)                                 \
    do                                                                  \
        {                                                               \
        if ((vxSndCtrlDbgMask & (mask)))                                \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_CTRL : [%s,%d] "fmt, __func__,   \
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define SND_CTRL_MODE_INFO(...)                                         \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define SND_CTRL_ERR(fmt, ...)    \
                  VX_SND_CTRL_MSG(VX_SND_CTRL_DBG_ERR,     fmt, ##__VA_ARGS__)
#define SND_CTRL_INFO(fmt, ...)   \
                  VX_SND_CTRL_MSG(VX_SND_CTRL_DBG_INFO,    fmt, ##__VA_ARGS__)
#define SND_CTRL_DBG(fmt, ...)    \
                  VX_SND_CTRL_MSG(VX_SND_CTRL_DBG_VERBOSE, fmt, ##__VA_ARGS__)
#else /* DEBUG_VX_SND_CTRL */
#define SND_CTRL_ERR(x...)      do { } while (FALSE)
#define SND_CTRL_INFO(x...)     do { } while (FALSE)
#define SND_CTRL_DBG(x...)      do { } while (FALSE)
#define SND_CTRL_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_CTRL */

/* forward declarations */

#ifdef _WRS_CONFIG_RTP
LOCAL STATUS ctrlScInit (void);

LOCAL STATUS ctrlScIoctl (SND_CONTROL_DEV * pData, int function,
                          void * pIoctlArg);
#endif

LOCAL void * sndCtrlOpen (SND_CONTROL_DEV * pData, const char * name, int flag,
                          int mode);

LOCAL int sndCtrlClose (SND_CONTROL_DEV * pData);

LOCAL ssize_t sndCtrlRead (SND_CONTROL_DEV * pData, uint32_t * pBuffer,
                           size_t maxBytes);

LOCAL ssize_t sndCtrlWrite (SND_CONTROL_DEV * pData, uint32_t * pBuffer,
                            size_t nbytes);

LOCAL STATUS sndCtrlKernelIoctl (SND_CONTROL_DEV * pData, int function,
                                 void * pIoctlArg);

LOCAL STATUS sndCtrlIoctl (SND_CONTROL_DEV * pData, int function,
                           void * pIoctlArg, BOOL sysCall);

LOCAL STATUS vxSndCtrlTlvRead (SND_CARD * card, VX_SND_CTRL_TLV * tlv);
LOCAL STATUS vxSndCtrlPut (SND_CARD * card, VX_SND_CTRL_DATA_VAL * pValue);

LOCAL STATUS vxSndCtrlGet (SND_CARD * card, VX_SND_CTRL_DATA_VAL * pValue);
LOCAL STATUS vxSndCtrlInfo (SND_CARD * card, VX_SND_CTRL_INFO * info);
LOCAL STATUS vxSndCtrlListGet (SND_CARD * card, VX_SND_CTRL_LIST * ctrlList);
LOCAL VX_SND_CONTROL * vxSndCardCtrlFindNoLock (SND_CARD * card,
                                                VX_SND_CTRL_ID * id);
LOCAL STATUS vxSndCtlAdd (SND_CARD * card, VX_SND_CONTROL * newCtrl);

/* locals */

LOCAL int     ctrlDrvNum = -1;

#ifdef _WRS_CONFIG_RTP
LOCAL const SC_IOCTL_TBL_ENTRY  ioctlCtrlTbl[] =
    {
    {SC_IOCTL_ENABLE_READ,  sizeof (int)                  }, /*  0 */
    {SC_IOCTL_ENABLE_WRITE, sizeof (int)                  }, /*  1 */
    {SC_IOCTL_ENABLE_READ,  sizeof (VX_SND_CTRL_CARD_INFO)}, /*  2 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_CTRL_LIST)     }, /*  3 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_CTRL_INFO)     }, /*  4 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_CTRL_DATA_VAL) }, /*  5 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_CTRL_DATA_VAL) }, /*  6 */
    {SC_IOCTL_ENABLE_WRITE, sizeof (VX_SND_CTRL_ID)       }, /*  7 */
    {SC_IOCTL_ENABLE_WRITE, sizeof (VX_SND_CTRL_ID)       }, /*  8 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (int)                  }, /*  9 */
    {SC_IOCTL_ENABLE_RMW,   sizeof (VX_SND_CTRL_TLV)      }, /* 10 */
    };
#endif /* _WRS_CONFIG_RTP */

/*******************************************************************************
*
* sndCtrlInit - initialize the control device
*
* This routine installs the sound card control device driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void sndCtrlInit (void)
    {
#ifdef _WRS_CONFIG_RTP
    if (ctrlScInit () == ERROR)
        {
        SND_CTRL_ERR ("ctrlScInit() failed\n");
        return;
        }
#endif /* _WRS_CONFIG_RTP */

    /* install the driver */

    ctrlDrvNum = iosDrvInstall (
                                NULL,                             /* creat()  */
                                NULL,                             /* remove() */
                                (DRV_OPEN_PTR)  sndCtrlOpen,
                                (DRV_CLOSE_PTR) sndCtrlClose,
                                (DRV_READ_PTR)  sndCtrlRead,
                                (DRV_WRITE_PTR) sndCtrlWrite,
                                (DRV_IOCTL_PTR) sndCtrlKernelIoctl);
    if (ctrlDrvNum == -1)
        {
        SND_CTRL_ERR ("failed to install control IOS driver\n");
        return;
        }

#ifdef _WRS_CONFIG_RTP
    /*
     * Register a different ioctl function ctrlScIoctl() for system call
     * from RTP. It validates the address range and access permission.
     */

    if (iosDrvIoctlMemValSet (ctrlDrvNum, (DRV_IOCTL_PTR) ctrlScIoctl) == ERROR)
        {
        SND_CTRL_ERR ("failed to set memory validation\n");
        return;
        }
#endif /* _WRS_CONFIG_RTP */
    }

/*******************************************************************************
*
* vxSndCtrlCreate - create control device
*
* This routine creates a control device for specified card. It also creates a
* sound device for this control device.
*
* RETURNS: OK, or ERROR if failed to create
*
* ERRNO: N/A
*/
//snd_ctl_create
STATUS vxSndCtrlCreate
    (
    SND_CARD * card
    )
    {
    SND_CONTROL_DEV * ctrlDev;

    if (card == NULL)
        {
        SND_CTRL_ERR ("card is NULL\n");
        return ERROR;
        }

    ctrlDev = (SND_CONTROL_DEV *) vxbMemAlloc (sizeof (SND_CONTROL_DEV));
    if (ctrlDev == NULL)
        {
        SND_CTRL_ERR ("failed to alloc SND_CONTROL_DEV\n");
        return ERROR;
        }

    ctrlDev->card = card;

    /* get control device name */

    (void) snprintf_s (ctrlDev->name, SND_DEV_NAME_LEN, "/dev/snd/controlC%d",
                       card->cardNum);

    ctrlDev->ioSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE);
    if (ctrlDev->ioSem == SEM_ID_NULL)
        {
        SND_CTRL_ERR ("failed to create ctrlDev->ioSem\n");
        goto errOut;
        }

    /* create sound device for control */

    if (vxSndDevCreate (card, VX_SND_DEV_CONTROL, (void *) ctrlDev,
                        (SND_DEV_REG_PTR) sndCtrlDevRegister,
                        &ctrlDev->sndDevice) != OK)
        {
        SND_CTRL_ERR ("failed to create sound device for control %s\n",
                     ctrlDev->name);
        goto errOut;
        }

    return OK;
errOut:
    if (ctrlDev->ioSem != NULL)
        {
        (void) semDelete (ctrlDev->ioSem);
        }

    vxbMemFree (ctrlDev);

    return ERROR;
    }

/*******************************************************************************
*
* sndCtrlDevRegister - register a control IOS device
*
* This routine registers a control device to IOS. Each soud card has only one
* control device.
*
* RETURNS: OK, or ERROR if the device failed to register
*
* ERRNO: N/A
*
* \NOMANUAL
*/
// snd_ctl_dev_register() called by snd_device_register_all()
STATUS sndCtrlDevRegister
    (
    SND_DEVICE * sndDev
    )
    {
    SND_CARD *        card;
    SND_CONTROL_DEV * ctrlDev;

    SND_CTRL_DBG ("register sound device\n");

    if (sndDev == NULL || sndDev->card == NULL || sndDev->data == NULL)
        {
        return ERROR;
        }

    card = sndDev->card;
    ctrlDev = (SND_CONTROL_DEV *) sndDev->data;

    if (sndDevRegister (&ctrlDev->devHdr, ctrlDev->name, ctrlDrvNum) != OK)
        {
        return ERROR;
        }

    SND_CTRL_DBG ("controlC%d is registered to IOS, "
                  "device name is %s, ctrlDrvNum is %d\n",
                  card->cardNum, ctrlDev->name, ctrlDrvNum);

    return OK;
    }

#ifdef _WRS_CONFIG_RTP
/*******************************************************************************
*
* ctrlScInit - control ioctl() command validation initialization
*
* This routine implements the initialization of ioctl() common validation for
* control device driver.
*
* RETURNS: OK, or ERROR if the registration is already done or the supplied
* input data are invalid.
*
* ERRNO: N/A
*/

LOCAL STATUS ctrlScInit (void)
    {
    return scIoctlGroupRegister (VX_IOCG_AUDIO_CTL, ioctlCtrlTbl,
                                 NELEMENTS (ioctlCtrlTbl));
    }

/*******************************************************************************
*
* ctrlScIoctl - handle ioctl call from system RTP
*
* This is a wrapper routine of sndCtrlIoctl() to handle ioctl from RTP.
*
* RETURNS: OK, or ERROR if failed to call sndCtrlIoctl()
*
* ERRNO: N/A
*/

LOCAL STATUS ctrlScIoctl
    (
    SND_CONTROL_DEV * pData,
    int               function,           /* function to perform */
    void *            pIoctlArg           /* function argument */
    )
    {
    return sndCtrlIoctl (pData, function, pIoctlArg, TRUE);
    }
#endif

/*******************************************************************************
*
* sndCtrlDevUnregister - unregister a sound control device
*
* This routine unregisters a sound control device.
*
* RETURNS: OK, or ERROR if failed to unregister
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS sndCtrlDevUnregister
    (
    SND_DEVICE * sndDev
    )
    {
    return OK;
    }

/*******************************************************************************
*
* sndCtrlOpen - open the control device
*
* This routine opens the control device.
*
* RETURNS: control device structure when successful; otherwise ERROR
*
* ERRNO: N/A
*/
//snd_ctl_open
LOCAL void * sndCtrlOpen
    (
    SND_CONTROL_DEV * ctrlDev,
    const char *      name,
    int               flag,
    int               mode
    )
    {
    SND_CTRL_DBG ("open device: %s, control: %s \n", name, ctrlDev->name);

    if (semTake (ctrlDev->ioSem, NO_WAIT) != OK)
        {
        return (void *) ERROR;
        }

    if (ctrlDev->isOpened)
        {
        SND_CTRL_ERR ("control device %s is already opened\n", ctrlDev->name);
        (void) semGive (ctrlDev->ioSem);
        return (void *) ERROR;
        }

    if (ctrlDev->card == NULL || ctrlDev->sndDevice == NULL)
        {
        SND_CTRL_ERR ("control device %s has no card or sound device\n",
                      ctrlDev->name);
        (void) semGive (ctrlDev->ioSem);
        return (void *) ERROR;
        }

    ctrlDev->isOpened = TRUE;
    ctrlDev->rtpId = MY_CTX_ID ();
    (void) semGive (ctrlDev->ioSem);
    return (void *) ctrlDev;
    }

/*******************************************************************************
*
* sndCtrlClose - close control device
*
* This routine closes control device.
*
* RETURNS: OK, or ERROR if failed to close
*
* ERRNO: N/A
*/

LOCAL int sndCtrlClose
    (
    SND_CONTROL_DEV * ctrlDev
    )
    {
    SND_CTRL_DBG ("close control: %s \n", ctrlDev->name);

    if (semTake (ctrlDev->ioSem, WAIT_FOREVER) != OK)
        {
        return ERROR;
        }

    if (!ctrlDev->isOpened)
        {
        (void) semGive (ctrlDev->ioSem);
        return ERROR;
        }

    ctrlDev->isOpened = FALSE;
    (void) semGive (ctrlDev->ioSem);

    return OK;
    }

/*******************************************************************************
*
* sndCtrlRead - read control data from the control device
*
* This routine reads control data from the control device.
*
* RETURNS: number of bytes read when successful; otherwise -1
*
* ERRNO: N/A
*/

LOCAL ssize_t sndCtrlRead
    (
    SND_CONTROL_DEV *  pData,
    uint32_t *         pBuffer,    /* location to store read data */
    size_t             maxBytes    /* number of bytes to read */
    )
    {
    return -1;
    }

/*******************************************************************************
*
* sndCtrlWrite - write control data to the control device
*
* This routine writes control data to the control device.
*
* RETURNS: number of bytes written when successful; otherwise -1
*
* ERRNO: N/A
*/

LOCAL ssize_t sndCtrlWrite
    (
    SND_CONTROL_DEV *  pData,
    uint32_t *         pBuffer,    /* location to store read data */
    size_t             nbytes      /* number of bytes to output */
    )
    {
    return -1;
    }

/*******************************************************************************
*
* sndCtrlKernelIoctl - handle ioctl call not from system RTP
*
* This is a wrapper routine of sndCtrlIoctl() to handle ioctl not from RTP.
*
* RETURNS: OK, or ERROR if failed to call sndCtrlIoctl()
*
* ERRNO: N/A
*/

LOCAL STATUS sndCtrlKernelIoctl
    (
    SND_CONTROL_DEV * pData,
    int               function,           /* function to perform */
    void *            pIoctlArg           /* function argument */
    )
    {
    return sndCtrlIoctl (pData, function, pIoctlArg, FALSE);
    }

/*******************************************************************************
*
* vxSndCtrlTlvRead - read TLV
*
* This routine reads TLV from specified control.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_ctl_tlv_ioctl(ctl, argp, SNDRV_CTL_TLV_OP_READ)
LOCAL STATUS vxSndCtrlTlvRead
    (
    SND_CARD *             card,
    VX_SND_CTRL_TLV *      tlv
    )
    {
    VX_SND_CONTROL * ctrl;
    VX_SND_CTRL_ID * id;
    uint32_t *       buf = tlv->tlvBuf;
    DL_NODE *        pNode;
    BOOL             foundCtrl = FALSE;

    /* length check: length = type(uint32_t) + length(uint32_t) + value */

    if (tlv->length < sizeof (uint32_t) * 2 ||
        tlv->length % sizeof (uint32_t) != 0)
        {
        return ERROR;
        }

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    FOR_EACH_CARD_CTRLS (card, pNode)
        {
        ctrl = (VX_SND_CONTROL *) pNode;
        id = &ctrl->id;

        if (id->numId == tlv->numId)
            {
            foundCtrl = TRUE;
            break;
            }
        }

    if (foundCtrl == FALSE)
        {
        SND_CTRL_ERR ("cannot find specified control %s\n", id->name);
        goto errOut;
        }

    if ((id->access & VX_SND_CTRL_ACCESS_TLV_READ) == 0)
        {
        SND_CTRL_ERR ("Vx Sound control just supports READ ops of TLV now\n");
        goto errOut;
        }

    if (ctrl->tlv.p == NULL)
        {
        SND_CTRL_ERR ("ctrl->tlv.p should not be NULL if support TLV Read\n");
        goto errOut;
        }

    (void) memcpy_s ((void *) buf, tlv->length,
                     (void *) (ctrl->tlv.p),
                     ctrl->tlv.p[1] + sizeof (uint32_t) * 2);

    SND_CTRL_DBG ("read %d bytes from tlv.p, type p[0] is %d, "
                  "length p[1] is %d\n",
                  *ctrl->tlv.p + sizeof (uint32_t) * 2,
                  ctrl->tlv.p[0], ctrl->tlv.p[1]);

    (void) semGive (card->controlListSem);

    return OK;

errOut:
    (void) semGive (card->controlListSem);
    return ERROR;
    }

/*******************************************************************************
*
* vxSndCtrlPut - control put()
*
* This routine executes put() of specified control.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_ctl_elem_write_user
LOCAL STATUS vxSndCtrlPut
    (
    SND_CARD *             card,
    VX_SND_CTRL_DATA_VAL * pValue
    )
    {
    VX_SND_CONTROL * ctrl;
    VX_SND_CTRL_ID * id = &pValue->id;

    SND_CTRL_DBG ("enter vxSndCtrlPut: ctrl %s\n", pValue->id.name);

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    ctrl = vxSndCardCtrlFindNoLock (card, id);
    if (ctrl == NULL)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("cannot find specified control %s\n", id->name);
        return ERROR;
        }

    if ((ctrl->id.access & VX_SND_CTRL_ACCESS_WRITE) == 0 || ctrl->put == NULL)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("cannot write control %s\n", id->name);
        return ERROR;
        }

    if (ctrl->put (ctrl, pValue) != OK)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("ctrl(%s)->put() error\n", id->name);
        return ERROR;
        }

    (void) semGive (card->controlListSem);

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlGet - control get()
*
* This routine executes get() of specified control.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_ctl_elem_read_user
LOCAL STATUS vxSndCtrlGet
    (
    SND_CARD *             card,
    VX_SND_CTRL_DATA_VAL * pValue
    )
    {
    VX_SND_CONTROL * ctrl;
    VX_SND_CTRL_ID * id = &pValue->id;

    SND_CTRL_DBG ("enter ctrl %s get:\n", pValue->id.name);

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    ctrl = vxSndCardCtrlFindNoLock (card, id);
    if (ctrl == NULL)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("cannot find specified control %s\n", id->name);
        return ERROR;
        }

    if ((ctrl->id.access & VX_SND_CTRL_ACCESS_READ) == 0 || ctrl->get == NULL)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("cannot read control %s\n", id->name);
        return ERROR;
        }

    if (ctrl->get (ctrl, pValue) != OK)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("ctrl(%s)->get() error\n", id->name);
        return ERROR;
        }

    (void) semGive (card->controlListSem);

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlInfo - control info()
*
* This routine executes info() of specified control.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//snd_ctl_elem_info_user
LOCAL STATUS vxSndCtrlInfo
    (
    SND_CARD *         card,
    VX_SND_CTRL_INFO * info
    )
    {
    VX_SND_CONTROL * ctrl;
    VX_SND_CTRL_ID * id = &info->id;

    SND_CTRL_DBG ("enter ctrl %s info:\n", id->name);

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    ctrl = vxSndCardCtrlFindNoLock (card, id);
    if (ctrl == NULL)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("cannot find specified control %s\n", id->name);
        return ERROR;
        }

    if (ctrl->info == NULL || ctrl->info (ctrl, info) != OK)
        {
        (void) semGive (card->controlListSem);
        SND_CTRL_ERR ("ctrl(%s)->info() error\n", id->name);
        return ERROR;
        }

    info->access = ctrl->id.access;

    (void) semGive (card->controlListSem);

    return OK;
    }

/*******************************************************************************
*
* vxSndCtrlListGet - get specified controls' id
*
* This is routine gets a list of controls' id.
*
* RETURNS: OK, or ERROR if failed
*
* ERRNO: N/A
*/
//static int snd_ctl_elem_list_user(struct snd_card *card,  struct snd_ctl_elem_list __user *_list)
LOCAL STATUS vxSndCtrlListGet
    (
    SND_CARD *         card,
    VX_SND_CTRL_LIST * ctrlList
    )
    {
    uint32_t         startNumId = ctrlList->offset + 1;
    uint32_t         endNumId   = startNumId + ctrlList->count - 1;
    DL_NODE *        pNode;
    VX_SND_CONTROL * ctrl;

    if (ctrlList->count == 0)
        {
        SND_CTRL_ERR ("ctrlList->count should be greater than 0\n");
        return ERROR;
        }

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    if (endNumId > card->controlCnt + 1)
        {
        endNumId = card->controlCnt;
        }

    ctrlList->used = 0;
    FOR_EACH_CARD_CTRLS(card, pNode)
        {
        ctrl = (VX_SND_CONTROL *) pNode;
        if (ctrl->id.numId >= startNumId && ctrl->id.numId <= endNumId)
            {
            char * name = ctrlList->getBuf[ctrlList->used].name;
            (void) memcpy_s (name, SND_DEV_NAME_LEN, ctrl->id.name,
                             SND_DEV_NAME_LEN);
            (void) memcpy_s (ctrlList->getBuf + ctrlList->used,
                             sizeof (VX_SND_CTRL_ID),
                             &ctrl->id, sizeof (VX_SND_CTRL_ID));
            ctrlList->getBuf[ctrlList->used].name = name;

            ctrlList->used++;
            }
        else if (ctrl->id.numId > endNumId)
            {
            if ((endNumId - startNumId + 1) != ctrlList->used)
                {
                (void) semGive (card->controlListSem);
                SND_CTRL_ERR ("ctrlList numId is not sequential\n");
                return ERROR;
                }
            break;
            }
        }

    (void) semGive (card->controlListSem);

    return OK;
    }

/*******************************************************************************
*
* sndCtrlIoctl - handle ioctls for control device
*
* This routine handles ioctls to the control device.
*
* RETURNS: OK when operation was successful; otherwise ERROR
*
* ERRNO: N/A
*/
//snd_ctl_ioctl

LOCAL STATUS sndCtrlIoctl
    (
    SND_CONTROL_DEV * ctrlDev,
    int               cmd,           /* function to perform */
    void *            pArg,          /* function argument */
    BOOL              sysCall        /* system call flag */
    )
    {
    SND_CARD * card;
    STATUS     status;

    if (vxSndDevScMemValidate ((const void *) pArg, cmd, sysCall) != OK)
        {
        return ERROR;
        }

    if (cmd != FIOSHUTDOWN && cmd != FIOFSTATGET)
        {
        if (IOCPARM_LEN (cmd) != sizeof (0) && pArg == NULL)
            {
            SND_CTRL_ERR ("cmd idx 0x%x should have a argument\n", cmd & 0xff);
            return ERROR;
            }
        }

    card = ctrlDev->card;

    switch (cmd)
        {
        case VX_SND_CMD_CTRL_CORE_VER:
            *((uint32_t *) pArg) = VX_SND_CTRL_CORE_VERSION;
            return OK;
        case VX_SND_CMD_CTRL_APP_VER:
            ctrlDev->appVersion = *((uint32_t *) pArg);
            return OK;
        case VX_SND_CMD_CARD_INFO:
            break;
        case VX_SND_CMD_CTRL_LIST_GET:
            return vxSndCtrlListGet (card, (VX_SND_CTRL_LIST *) pArg);
        case VX_SND_CMD_CTRL_INFO_GET:
            return vxSndCtrlInfo (card, (VX_SND_CTRL_INFO *) pArg);
        case VX_SND_CMD_CTRL_GET:
            return vxSndCtrlGet (card, (VX_SND_CTRL_DATA_VAL *) pArg);
        case VX_SND_CMD_CTRL_PUT:
            return vxSndCtrlPut (card, (VX_SND_CTRL_DATA_VAL *) pArg);
        case VX_SND_CMD_CTRL_LOCK:
        case VX_SND_CMD_CTRL_UNLOCK:
        case VX_SND_CMD_SUBSCRIBE_EVENTS:
            break;
        case VX_SND_CMD_TLV_READ:
            status = vxSndCtrlTlvRead (card, (VX_SND_CTRL_TLV *)  pArg);
            return status;
        case FIOSHUTDOWN:
        case FIOFSTATGET:
            SND_CTRL_DBG ("FIOSHUTDOWN or FIOFSTATGET\n");
            break;
        default:
            SND_CTRL_ERR ("unknown CONTROL cmd 0x%x\n", cmd);
            return ERROR;
        }

    SND_CTRL_DBG ("CONTROL cmd 0x%x is not supported now\n", cmd);
    return OK;
    }

/*******************************************************************************
*
* vxSndCardCtrlFindNoLock - find control from card
*
* This routine finds specified control from card control list.
*
* RETURNS: contrl pointer, or NULL if not find
*
* ERRNO: N/A
*/

LOCAL VX_SND_CONTROL * vxSndCardCtrlFindNoLock
    (
    SND_CARD *       card,
    VX_SND_CTRL_ID * id
    )
    {
    DL_NODE *        pNode;
    VX_SND_CTRL_ID * idTemp;
    VX_SND_CONTROL * ctrl;

    FOR_EACH_CARD_CTRLS (card, pNode)
        {
        ctrl = (VX_SND_CONTROL *) pNode;
        idTemp = &ctrl->id;

        if (id->numId != INVALID_CTL_NUMID)
            {
            if (id->numId == idTemp->numId)
                {
                SND_CTRL_DBG ("control-%s is found by numId-%d\n",
                              idTemp->name, idTemp->numId);
                return ctrl;
                }
            }
        else{
            if (id->iface == idTemp->iface &&
                id->deviceNum == idTemp->deviceNum &&
                id->substreamNum == idTemp->substreamNum &&
                strncmp (id->name, idTemp->name, SND_DEV_NAME_LEN) == 0)
                {
                SND_CTRL_DBG ("control is found by name-%s\n",
                              idTemp->name);
                return ctrl;
                }
            }
        }

    return NULL;
    }

/*******************************************************************************
*
* vxSndCtlAdd - add a new control to card
*
* This routine adds a new control to specified card. If card already has a same
* control, it will fail to add.
*
* RETURNS: OK, or ERROR if failed to add
*
* ERRNO: N/A
*/
//snd_ctl_add
LOCAL STATUS vxSndCtlAdd
    (
    SND_CARD *       card,
    VX_SND_CONTROL * newCtrl
    )
    {
    if (card == NULL || newCtrl ==  NULL)
        {
        return ERROR;
        }
    if (newCtrl->id.numId != INVALID_CTL_NUMID)
        {
        SND_CTRL_ERR ("newCtrl->id.numId (%d) should be INVALID_CTL_NUMID\n",
                      newCtrl->id.numId);
        return ERROR;
        }

    if (semTake (card->controlListSem, WAIT_FOREVER) != OK)
         {
         return ERROR;
         }

    /* check if ctrl exists on card->controlList */

    if (vxSndCardCtrlFindNoLock (card, &newCtrl->id) != NULL)
        {
        SND_CTRL_ERR ("control %s already exists\n", newCtrl->id.name);
        (void) semGive (card->controlListSem);

        return ERROR;
        }

    /* add new control to the tail */

    dllAdd (&card->controlList, &newCtrl->node);

    card->controlCnt++; /* valid numId from 1 */
    newCtrl->id.numId = card->controlCnt;

    (void) semGive (card->controlListSem);

    return OK;
    }

/*******************************************************************************
*
* vxSndControlsAdd - add controls to card
*
* This routine adds a list new controls to card.
*
* RETURNS: OK, or ERROR if failed to add
*
* ERRNO: N/A
*/
//static int snd_soc_add_controls(struct snd_card *card, struct device *dev,const struct snd_kcontrol_new *controls, int num_controls,const char *prefix, void *data)
STATUS vxSndControlsAdd
    (
    SND_CARD *       card,
    VX_SND_CONTROL * ctrlList,
    int              num,
    const char *     namePrefix,
    void *           privateData
    )
    {
    uint32_t         idx;
    VX_SND_CONTROL * pCtrl;
    VX_SND_CONTROL * newCtrl;
    DL_NODE *        pNode;

    for (idx = 0; idx < num; idx++)
        {
        pCtrl = &ctrlList[idx];
        if (pCtrl->info == NULL)
            {
            SND_CTRL_ERR ("SND_CONTROL %d must have info()\n", idx);
            continue;
            }

        if (pCtrl->id.name == NULL)
            {
            SND_CTRL_ERR ("SND_CONTROL %d must have name()\n", idx);
            continue;
            }

        newCtrl = (VX_SND_CONTROL *) vxbMemAlloc (sizeof (VX_SND_CONTROL));
        if (newCtrl == NULL)
            {
            SND_CTRL_ERR ("failed to alloc SND_CONTROL\n");
            goto errOut;
            }

        (void) memcpy_s (newCtrl, sizeof (VX_SND_CONTROL), pCtrl,
                         sizeof (VX_SND_CONTROL));

        newCtrl->id.name = vxbMemAlloc (SND_DEV_NAME_LEN);
        if (newCtrl->id.name == NULL)
            {
            SND_CTRL_ERR ("Fail to create name string\n");
            goto errOut;
            }
        else
            {
            /* add name prefix */

            if (namePrefix != NULL)
                {
                (void) snprintf_s (newCtrl->id.name, SND_DEV_NAME_LEN, "%s %s",
                                   namePrefix, pCtrl->id.name);
                }
            else
                {
                (void) snprintf_s (newCtrl->id.name, SND_DEV_NAME_LEN, "%s",
                                   pCtrl->id.name);
                }
            }

        /* fix up access */

        if (newCtrl->id.access == 0)
            {
            newCtrl->id.access = VX_SND_CTRL_ACCESS_READWRITE;
            }

        /* privateData usually stores component or DAI pointer */

        newCtrl->privateData = privateData;

        /*
         * numId should be INVALID_CTL_NUMID before add to card, card will
         * assign it a index
         */

        newCtrl->id.numId = INVALID_CTL_NUMID;

        /* attach to card  */

        if (vxSndCtlAdd (card, newCtrl) != OK)
            {
            SND_CTRL_ERR ("failed to add control %s\n", newCtrl->id.name);
            vxbMemFree (newCtrl);

            return ERROR;
            }
        }

    return OK;

errOut:

    pNode = DLL_FIRST (&card->controlList);
    while (pNode != NULL)
        {
        DLL_REMOVE (&card->controlList, pNode);
        pCtrl = (VX_SND_CONTROL *) pNode;
        if (pCtrl->id.name != NULL)
            {
            vxbMemFree (pCtrl->id.name);
            }
        vxbMemFree (pCtrl);
        pNode = DLL_FIRST (&card->controlList);
        }

    return ERROR;
    }
