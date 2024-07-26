/* vxbFdtAboxMailbox.c - Samsung Abox mailbox driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#include <vxWorks.h>
#include <hwif/buslib/vxbFdtLib.h>
#include <vxbFdtAboxGic.h>
#include <vxbFdtAboxMailbox.h>

#undef ABOX_MAILBOX_DEBUG
#ifdef ABOX_MAILBOX_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0xffffffff

LOCAL uint32_t aboxMailboxDbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((aboxMailboxDbgMask & (mask)) || ((mask) == DBG_ALL))       \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("[%s(%d)] ", __func__, __LINE__);     \
                (* _func_kprintf)(__VA_ARGS__);                         \
                }                                                       \
            }                                                           \
        }                                                               \
    while ((FALSE))
#else
#undef DBG_MSG
#define DBG_MSG(...)
#endif  /* TI_MAILBOX_DEBUG */

/* defines */

/* controllor read and write interface */

#define REG_READ_4(pData, reg)             \
        vxbRead32((pData)->regHandle, (uint32_t *)((pData)->regBase + (reg)))

#define REG_WRITE_4(pData, reg, data)      \
        vxbWrite32((pData)->regHandle,        \
                   (uint32_t *)((pData)->regBase + (reg)), (data))

/* forward declarations */

IMPORT void vxbUsDelay (int delayTime);

LOCAL STATUS aboxMailboxProbe (VXB_DEV_ID pDev);
LOCAL STATUS aboxMailboxAttach (VXB_DEV_ID pDev);

/* locals */

LOCAL VXB_DRV_METHOD aboxMailboxMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxMailboxProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxMailboxAttach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxMailboxMatch[] =
    {
        {
        "samsung,abox_mailbox",
        NULL
        },
        {
        "samsung,abox_mailbox_rx",
        NULL
        },
        {
        "samsung,abox_mailbox_tx",
        NULL
        },
        {}                              /* empty terminated list */
    };

/* globals */

VXB_DRV aboxMailboxDrv =
    {
    {NULL},
    ABOX_MAILBOX_DRV_NAME,              /* Name */
    "Abox Mailbox FDT driver",          /* Description */
    VXB_BUSID_FDT,                      /* Class */
    0,                                  /* Flags */
    0,                                  /* Reference count */
    aboxMailboxMethodList               /* Method table */
    };

VXB_DRV_DEF(aboxMailboxDrv)


//abox_mailbox_reset
LOCAL void aboxMailboxReset
    (
    ABOX_MAILBOX_DATA * pMailboxData
    )
    {
    REG_WRITE_4 (pMailboxData, ABOX_MAILBOX_MCUCTRL, 1);
    }

//abox_mailbox_spi_to_gen_id
LOCAL uint32_t aboxMailboxSpiToGenId
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            spi
    )
    {
    uint32_t            i;

    for (i = 0; i < pMailboxData->spiNum; i++)
        {
        if ((pMailboxData->mboxSpi[i] + ABOX_GIC_SPI_OFS) == spi)
            {
            return i;
            }
        }

    return 0;
    }

//abox_mailbox_get_ipc
LOCAL uint32_t aboxMailboxGetIpc
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            genId
    )
    {
    uint32_t            value;
    uint32_t            shift;
    uint32_t            mask;
    uint32_t            reg;

    switch (genId)
        {
        case ABOX_MAILBOX_GENERATE0:
            shift = 16;
            mask = 0XFFFF0000;
            reg = ABOX_MAILBOX_INTSR0;
            break;

        case ABOX_MAILBOX_GENERATE1:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR1;
            break;

        case ABOX_MAILBOX_GENERATE2:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR2;
            break;

        case ABOX_MAILBOX_GENERATE3:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR3;
            break;

        default:
            return NOT_USED15;
        }

    value = (REG_READ_4 (pMailboxData, reg)) & mask;
    return (value >> shift);
    }

//abox_mailbox_generate_interrupt
LOCAL void aboxMailboxGenerateInterrupt
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            ipcId,
    uint32_t            genId
    )
    {
    uint32_t            value;
    uint32_t            reg;

    switch (genId)
        {
        case ABOX_MAILBOX_GENERATE0:
            value = ipcId << 16;
            reg = ABOX_MAILBOX_INTGR0;
            break;

        case ABOX_MAILBOX_GENERATE1:
            value = ipcId;
            reg = ABOX_MAILBOX_INTGR1;
            break;

        case ABOX_MAILBOX_GENERATE2:
            value = ipcId;
            reg = ABOX_MAILBOX_INTGR2;
            break;

        case ABOX_MAILBOX_GENERATE3:
            value = ipcId;
            reg = ABOX_MAILBOX_INTGR3;
            break;

        default:
            return;
        }

    REG_WRITE_4 (pMailboxData, reg, value);
    }

//abox_mailbox_clear_interrupt
LOCAL void aboxMailboxClearInterrupt
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            ipcId,
    uint32_t            genId
    )
    {
    uint32_t            reg;
    uint32_t            value;

    switch (genId)
        {
        case ABOX_MAILBOX_GENERATE0:
            value = ipcId << 16;
            reg = ABOX_MAILBOX_INTCR0;
            break;

        case ABOX_MAILBOX_GENERATE1:
            value = ipcId;
            reg = ABOX_MAILBOX_INTCR1;
            break;

        case ABOX_MAILBOX_GENERATE2:
            value = ipcId;
            reg = ABOX_MAILBOX_INTCR2;
            break;

        case ABOX_MAILBOX_GENERATE3:
            value = ipcId;
            reg = ABOX_MAILBOX_INTCR3;
            break;

        default:
            return;
        }

    REG_WRITE_4 (pMailboxData, reg, value);
    }

//abox_mailbox_check_ipc_status
LOCAL BOOL aboxMailboxCheckIpcStatus
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            ipcId,
    uint32_t            genId
    )
    {
    uint32_t            value;
    uint32_t            shift;
    uint32_t            mask;
    uint32_t            reg;
    BOOL                ret;

    switch (genId)
        {
        case ABOX_MAILBOX_GENERATE0:
            shift = 16;
            mask = 0XFFFF0000;
            reg = ABOX_MAILBOX_INTSR0;
            break;

        case ABOX_MAILBOX_GENERATE1:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR1;
            break;

        case ABOX_MAILBOX_GENERATE2:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR2;
            break;

        case ABOX_MAILBOX_GENERATE3:
            shift = 0;
            mask = 0XFFFF;
            reg = ABOX_MAILBOX_INTSR3;
            break;

        default:
            return FALSE;
        }

    value = (REG_READ_4 (pMailboxData, reg)) & mask;
    ret = ((value >> shift) & ipcId) ? TRUE : FALSE;

    return ret;
    }

//abox_mailbox_msg_read
LOCAL void aboxMailboxMsgRead
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    void *              msg,
    uint32_t            msgSize
    )
    {
    uint32_t *          pDst;
    uint32_t            srcReg;
    uint32_t            i;

    srcReg = pMailboxData->ipcRxOfs;
    pDst = (uint32_t *)(msg);

    SPIN_LOCK_ISR_TAKE (&pMailboxData->rxMsgSpinlockIsr);

    for (i = 0; i < msgSize; i += sizeof (uint32_t))
        {
        *pDst = REG_READ_4 (pMailboxData, srcReg + i);
        pDst++;
        }

    SPIN_LOCK_ISR_GIVE (&pMailboxData->rxMsgSpinlockIsr);
    }


//__abox_mailbox_ipc_queue_full
LOCAL BOOL aboxMailboxIpcQueIsFull
    (
    ABOX_MAILBOX_DATA * pMailboxData
    )
    {
    return (((pMailboxData->ipcQueEnd + 1) % ABOX_MAILBOX_SPI_QUE_SIZE)
             == pMailboxData->ipcQueStart);
    }

//__abox_mailbox_ipc_queue_empty
LOCAL BOOL aboxMailboxIpcQueIsEmpty
    (
    ABOX_MAILBOX_DATA * pMailboxData
    )
    {
    return (pMailboxData->ipcQueEnd == pMailboxData->ipcQueStart);
    }

//abox_mailbox_ipc_queue_put
LOCAL STATUS aboxMailboxIpcQuePut
    (
    ABOX_MAILBOX_DATA * pMailboxData,
    uint32_t            ipcId,
    ABOX_IPC_MSG *      pMsg
    )
    {
    STATUS              ret = OK;

    SPIN_LOCK_ISR_TAKE (&pMailboxData->ipcQueSpinlockIsr);

    if (!aboxMailboxIpcQueIsFull (pMailboxData))
        {
        ABOX_MAILBOX_IPC_QUE * curIpcQue;

        curIpcQue = &pMailboxData->ipcQue[pMailboxData->ipcQueEnd];
        curIpcQue->ipcId = ipcId;
        memcpy(&curIpcQue->msg, pMsg, sizeof(ABOX_IPC_MSG));
        pMailboxData->ipcQueEnd =
            (pMailboxData->ipcQueEnd + 1) % ABOX_MAILBOX_SPI_QUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pMailboxData->ipcQueSpinlockIsr);

    return ret;
    }

//abox_mailbox_ipc_queue_get
LOCAL STATUS aboxMailboxIpcQueGet
    (
    ABOX_MAILBOX_DATA *     pMailboxData,
    ABOX_MAILBOX_IPC_QUE *  ipcQue
    )
    {
    STATUS                  ret = OK;

    SPIN_LOCK_ISR_TAKE (&pMailboxData->ipcQueSpinlockIsr);

    if (!aboxMailboxIpcQueIsEmpty (pMailboxData))
        {
        ABOX_MAILBOX_IPC_QUE * curIpcQue;

        curIpcQue = &pMailboxData->ipcQue[pMailboxData->ipcQueStart];
        memcpy(ipcQue, curIpcQue, sizeof(ABOX_MAILBOX_IPC_QUE));
        pMailboxData->ipcQueStart =
            (pMailboxData->ipcQueStart + 1) % ABOX_MAILBOX_SPI_QUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pMailboxData->ipcQueSpinlockIsr);

    return ret;
    }

//__abox_mailbox_irq_handler
LOCAL STATUS aboxMailboxIpcHandle
    (
    ABOX_MAILBOX_DATA *     pMailboxData,
    uint32_t                ipcId,
    ABOX_IPC_MSG *          pMsg
    )
    {
    ABOX_IPC_HANDLE_FUNC    ipcFunc;

    ipcFunc = pMailboxData->ipcHandler.ipcFunc;
    if (ipcFunc == NULL)
        {
        return OK;
        }

    return ipcFunc (ipcId, pMsg, pMailboxData->ipcHandler.devId);
    }

//abox_mailbox_irq_handler
LOCAL STATUS aboxMailboxIsr
    (
    uint32_t            irq,
    VXB_DEV_ID          devId
    )
    {
    ABOX_MAILBOX_DATA * pMailboxData = vxbDevSoftcGet (devId);
    uint32_t            genId = 0;
    ABOX_IPC_MSG        msg = {0};
    uint32_t            ipcId;
    int32_t             ret = OK;

    if (pMailboxData == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return ERROR;
        }

    genId = aboxMailboxSpiToGenId (pMailboxData, irq);

    while (aboxMailboxGetIpc (pMailboxData, genId))
        {
        ipcId = aboxMailboxGetIpc (pMailboxData, genId);

        aboxMailboxMsgRead (pMailboxData, &msg, sizeof(msg));

        switch (ipcId)
            {
            case IPC_RECEIVED:
            case IPC_SYSTEM:
            case IPC_PCMPLAYBACK:
            case IPC_PCMCAPTURE:
            case IPC_OFFLOAD:
            case IPC_ABOX_CONFIG:
            case IPC_ABOX_STOP_LOOPBACK:
                if (aboxMailboxIpcQuePut (pMailboxData, ipcId, &msg) != OK)
                    {
                    DBG_MSG (DBG_ERR, "mbox ipc queue full\n");
                    }
                else
                    {
                    if (!pMailboxData->irqWaitFlag)
                        {
                        pMailboxData->irqWaitFlag = TRUE;
                        semGive (pMailboxData->irqHandleSem);
                        }
                    }
                break;

            case IPC_DMA_INTR:
                ret = aboxMailboxIpcHandle (pMailboxData, ipcId, &msg);
                break;

            default:
                break;
            }
        aboxMailboxClearInterrupt (pMailboxData, ipcId, genId);
        }

    return ret;
    }

//abox_mailbox_irq_thread
LOCAL void aboxMailboxirqTask
    (
    ABOX_MAILBOX_DATA *     pMailboxData
    )
    {
    ABOX_MAILBOX_IPC_QUE    ipcQue;

    while (1)
        {
        semTake (pMailboxData->irqHandleSem, WAIT_FOREVER);
        pMailboxData->irqWaitFlag = FALSE;

        while (aboxMailboxIpcQueGet (pMailboxData, &ipcQue) == OK)
            {
            DBG_MSG (DBG_INFO, "get mbox ipc id=%d to handle\n", ipcQue.ipcId);
            if (aboxMailboxIpcHandle (pMailboxData, ipcQue.ipcId, &ipcQue.msg)
                != OK)
                {
                DBG_MSG (DBG_ERR, "handle mbox ipc failed\n");
                }
            }
        }
    }

/******************************************************************************
*
* aboxMailboxProbe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS aboxMailboxProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxMailboxMatch, NULL);
    }

/******************************************************************************
*
* aboxMailboxAttach - attach a device
*
* This is the device attach routine.
*
* RETURNS: OK, or ERROR if attach failed
*
* ERRNO: N/A
*/

LOCAL STATUS aboxMailboxAttach
    (
    VXB_DEV_ID          pDev
    )
    {
    VXB_FDT_DEV *       pFdtDev;
    ABOX_MAILBOX_DATA * pMailboxData;
    VXB_RESOURCE *      pResMem = NULL;
    ABOX_CTR_DATA *     pCtrData;
    uint32_t *          pValue;
    int32_t             len;
    char                propStr[ABOX_MAILBOX_STR_MAX] = {NULL};
    UINT16              idx;
    uint32_t            spiId;

    if (pDev == NULL)
        {
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        return ERROR;
        }

    /* abox GIC needs to be attached first */

    pCtrData = ABOX_CTR_GLB_DATA;
    if (vxbDevSoftcGet (pCtrData->gicDev) == NULL)
        {
        return ERROR;
        }

    pMailboxData = (ABOX_MAILBOX_DATA *)
                        vxbMemAlloc (sizeof (ABOX_MAILBOX_DATA));
    if (pMailboxData == NULL)
        {
        return ERROR;
        }

    pMailboxData->gicDev = pCtrData->gicDev;

    /* DTS configuration */

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,type", &len);
    if (pValue != NULL)
        {
        pMailboxData->type = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    if (pMailboxData->type == ABOX_MAILBOX_RX)
        {
        pMailboxData->id = ABOX_MAILBOX_RX_ID;
        }
    else
        {
        pMailboxData->id = ABOX_MAILBOX_TX_ID;
        }

    DBG_MSG (DBG_INFO, "mailbox type=%d id=%d\n",
             pMailboxData->type, pMailboxData->id);

    (void) snprintf (propStr, ABOX_MAILBOX_STR_MAX, "mailbox%d",
                     pMailboxData->id);

    idx = (UINT16) vxFdtPropStrIndexGet (pFdtDev->offset, "reg-names", propStr);

    pResMem = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResMem == NULL) || (pResMem->pRes == NULL))
        {
        goto errOut;
        }

    pMailboxData->regBase = ((VXB_RESOURCE_ADR *)(pResMem->pRes))->virtAddr;
    pMailboxData->regHandle = ((VXB_RESOURCE_ADR *)(pResMem->pRes))->pHandle;

    DBG_MSG (DBG_INFO, "pDev=0x%llx vaddr=0x%llx paddr=0x%llx\n", pDev,
             pMailboxData->regBase,
             ((VXB_RESOURCE_ADR *)(pResMem->pRes))->start);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "ipc_tx_offset", &len);
    if (pValue != NULL)
        {
        pMailboxData->ipcTxOfs = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "ipc_rx_offset", &len);
    if (pValue != NULL)
        {
        pMailboxData->ipcRxOfs = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "num-of-spi", &len);
    if (pValue != NULL)
        {
        pMailboxData->spiNum = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    pMailboxData->mboxSpi = (uint32_t *)
                        vxbMemAlloc (sizeof (uint32_t) * pMailboxData->spiNum);

    (void) snprintf (propStr, ABOX_MAILBOX_STR_MAX, "spi-of-mailbox%d",
                     pMailboxData->id);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, propStr, &len);
    if (pValue == NULL || len != (sizeof (uint32_t) * pMailboxData->spiNum))
        {
        goto errOut;
        }

    for (idx = 0; idx < pMailboxData->spiNum; idx++, pValue++)
        {
        pMailboxData->mboxSpi[idx] = vxFdt32ToCpu (*pValue);
        }

    SPIN_LOCK_ISR_INIT (&pMailboxData->txMsgSpinlockIsr, 0);
    SPIN_LOCK_ISR_INIT (&pMailboxData->rxMsgSpinlockIsr, 0);
    SPIN_LOCK_ISR_INIT (&pMailboxData->ipcQueSpinlockIsr, 0);

    pMailboxData->irqHandleSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pMailboxData->irqHandleSem == SEM_ID_NULL)
        {
        goto errOut;
        }

    if (pMailboxData->type == ABOX_MAILBOX_TX)
        {
        aboxMailboxReset (pMailboxData);
        }

    for (idx = 0; idx < pMailboxData->spiNum; idx++)
        {
        spiId = pMailboxData->mboxSpi[idx];

        if (spiId != 0 && pMailboxData->type != ABOX_MAILBOX_TX)
            {
            spiId += ABOX_GIC_SPI_OFS;
            DBG_MSG (DBG_INFO, "register mbox rx isr to spiId=%d dev=0x%llx\n",
                     spiId, pDev);
            aboxGicRegisterSpiHandler (pMailboxData->gicDev, spiId,
                                       aboxMailboxIsr, pDev);
            if (pMailboxData->spi)      // bug?? always 0
                {
                pMailboxData->spi = spiId;
                }
            }
        }

    /* save pMailboxData in VXB_DEVICE structure */

    pMailboxData->pDev = pDev;
    vxbDevSoftcSet (pDev, pMailboxData);

    if (pMailboxData->type == ABOX_MAILBOX_RX)
        {
        pCtrData->mboxRxDev = pDev;

        pMailboxData->irqTask = taskSpawn (ABOX_MAILBOX_IRQ_TASK_NAME,
                                       ABOX_MAILBOX_IRQ_TASK_PRI,
                                       0,
                                       ABOX_MAILBOX_IRQ_TASK_STACK,
                                       (FUNCPTR) aboxMailboxirqTask,
                                       (_Vx_usr_arg_t) pMailboxData,
                                       0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (pMailboxData->irqTask == TASK_ID_ERROR)
            {
            goto errOut;
            }
        }
    else
        {
        pCtrData->mboxTxDev = pDev;
        }

    return OK;

errOut:
    if (pMailboxData->irqTask != NULL)
        {
        (void) taskDelete (pMailboxData->irqTask);
        }

    if (pMailboxData->irqHandleSem != SEM_ID_NULL)
        {
        (void) semDelete (pMailboxData->irqHandleSem);
        }

    if (pMailboxData->mboxSpi != NULL)
        {
        (void) vxbMemFree (pMailboxData->mboxSpi);
        }

    if (pResMem != NULL)
        {
        (void) vxbResourceFree (pDev, pResMem);
        }

    if (pMailboxData != NULL)
        {
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pMailboxData);
        }

    return ERROR;
    }

/* APIs */

//abox_mailbox_register_irq_handler
STATUS aboxMailboxRegisterIpcHandler
    (
    VXB_DEV_ID              pMboxDev,
    ABOX_IPC_HANDLE_FUNC    handler,
    VXB_DEV_ID              devId
    )
    {
    ABOX_MAILBOX_DATA *     pMailboxData = vxbDevSoftcGet (pMboxDev);

    pMailboxData->ipcHandler.ipcFunc = handler;
    pMailboxData->ipcHandler.devId = devId;

    return OK;
    }

//abox_mailbox_unregister_irq_handler
STATUS aboxMailboxUnregisterIpcHandler
    (
    VXB_DEV_ID          pDev
    )
    {
    ABOX_MAILBOX_DATA * pMailboxData = vxbDevSoftcGet (pDev);

    pMailboxData->ipcHandler.ipcFunc = NULL;
    pMailboxData->ipcHandler.devId = NULL;

    return OK;
    }

//abox_mailbox_send_msg
STATUS aboxMailboxSendMsg
    (
    VXB_DEV_ID              pDev,
    uint32_t                ipcId,
    const ABOX_IPC_MSG *    pMsg,
    uint32_t                msgSize,
    uint32_t                timeoutUs,
    uint32_t                adsp
    )
    {
    ABOX_MAILBOX_DATA *     pMailboxData = vxbDevSoftcGet (pDev);
    STATUS                  ret = OK;
    uint32_t *              pSrc;
    uint32_t                dstReg;
    uint32_t                i;

    pSrc = (uint32_t *)(pMsg);
    dstReg = pMailboxData->ipcTxOfs;

    DBG_MSG (DBG_INFO, "mbox msg ipc trigger ipcId=%d adsp=%d regbase=0x%llx\n",
             ipcId, adsp, pMailboxData->regBase);

    SPIN_LOCK_ISR_TAKE (&pMailboxData->txMsgSpinlockIsr);

    for (i = 0; i < msgSize; i += sizeof (uint32_t))
        {
        REG_WRITE_4 (pMailboxData, dstReg + i, *pSrc);
        pSrc++;
        }

    aboxMailboxGenerateInterrupt (pMailboxData, ipcId, adsp);
    for (i = timeoutUs;
         i && aboxMailboxCheckIpcStatus (pMailboxData, ipcId, adsp);
         i--)
        {
        vxbUsDelay (1);
        }

    if (aboxMailboxCheckIpcStatus (pMailboxData, ipcId, adsp))
        {
        aboxMailboxClearInterrupt (pMailboxData, ipcId, adsp);
        DBG_MSG (DBG_ERR, "DSP don't clear mbox ipcId=%d adsp=%d\n", ipcId,
                 adsp);
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pMailboxData->txMsgSpinlockIsr);

    return ret;
    }

