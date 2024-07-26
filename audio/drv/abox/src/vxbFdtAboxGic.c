/* vxbFdtAboxGic.c - Samsung Abox GIC driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION

*/

#include <vxWorks.h>
#include <subsys/int/vxbIntLib.h>
#include <vxbAboxPcm.h>
#include <vxbFdtAboxMailbox.h>
#include <vxbFdtAboxGic.h>

#undef ABOX_GIC_DEBUG
#ifdef ABOX_GIC_DEBUG

#include <private/kwriteLibP.h>         /* _func_kprintf */
#define DBG_OFF             0x00000000
#define DBG_WARN            (0x1 << 1)
#define DBG_ERR             (0x1 << 2)
#define DBG_INFO            (0x1 << 3)
#define DBG_VERB            (0x1 << 4)
#define DBG_IRQ             (0x1 << 5)
#define DBG_ALL             0XFFFFFFFF

LOCAL uint32_t aboxGicDbgMask = DBG_ALL;
#undef DBG_MSG
#define DBG_MSG(mask, ...)                                              \
    do                                                                  \
        {                                                               \
        if ((aboxGicDbgMask & (mask)) || ((mask) == DBG_ALL))           \
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
#endif  /* ABOX_GIC_DEBUG */

/* defines */

/* controllor read and write interface */

#define REG_READ_4(pData, regBase, regOfs)             \
        vxbRead32((pData)->regHandle, (uint32_t *)((regBase) + (regOfs)))

#define REG_WRITE_4(pData, regBase, regOfs, data)      \
        vxbWrite32((pData)->regHandle,              \
                   (uint32_t *)((regBase) + (regOfs)), (data))


/* forward declarations */

LOCAL STATUS aboxGicProbe (VXB_DEV_ID pDev);
LOCAL STATUS aboxGicAttach (VXB_DEV_ID pDev);

/* locals */

LOCAL VXB_DRV_METHOD aboxGicMethodList[] =
    {
    /* DEVICE API */

    {VXB_DEVMETHOD_CALL(vxbDevProbe), aboxGicProbe},
    {VXB_DEVMETHOD_CALL(vxbDevAttach), aboxGicAttach},
    {0, NULL}
    };

LOCAL VXB_FDT_DEV_MATCH_ENTRY aboxGicMatch[] =
    {
        {
        "samsung,abox_gic",
        NULL
        },
        {}                              /* empty terminated list */
    };

LOCAL ABOX_GIC_DATA * pAboxGicData;

/* globals */

VXB_DRV aboxGicDrv =
    {
    {NULL},
    ABOX_GIC_DRV_NAME,                  /* Name */
    "Abox GIC FDT driver",              /* Description */
    VXB_BUSID_FDT,                      /* Class */
    0,                                  /* Flags */
    0,                                  /* Reference count */
    aboxGicMethodList                   /* Method table */
    };

VXB_DRV_DEF(aboxGicDrv)

//abox_gic_backup_iar
LOCAL void aboxGicBackupIar
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        iar
    )
    {
    if (pGicData == NULL || pGicData->iarBakRegBase == NULL)
        {
        DBG_MSG (DBG_ERR, "\n");
        return;
        }

    REG_WRITE_4 (pGicData, pGicData->iarBakRegBase, 0, iar);
    }

//__abox_gic_irq_handler
LOCAL STATUS aboxGicIrqHandle
    (
    ABOX_GIC_DATA *             pGicData,
    uint32_t                    spiId
    )
    {
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc;

    if (spiId >= ABOX_GIC_IRQ_MAXNUM)
        {
        return OK;
        }

    irqFunc = pGicData->gicIrqHandler[spiId].irqFunc;
    if (irqFunc == NULL)
        {
        return OK;
        }

    return irqFunc (spiId, pGicData->gicIrqHandler[spiId].devId);
    }

//__abox_gic_iar_queue_full
LOCAL BOOL aboxGicIrqQueIsFull
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    return (((pGicData->irqQueEnd + 1) % ABOX_GIC_IRQ_QUEUE_SIZE)
            == pGicData->irqQueStart);
    }

//__abox_gic_iar_queue_empty
LOCAL BOOL aboxGicIrqQueIsEmpty
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    return (pGicData->irqQueEnd == pGicData->irqQueStart);
    }

//abox_gic_iar_queue_put
LOCAL STATUS aboxGicIrqQuePut
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irqId
    )
    {
    STATUS          ret = OK;

    SPIN_LOCK_ISR_TAKE (&pGicData->irqQueSpinlockIsr);

    if (!aboxGicIrqQueIsFull (pGicData))
        {
        pGicData->irqQue[pGicData->irqQueEnd] = irqId;
        pGicData->irqQueEnd =
            (pGicData->irqQueEnd + 1) % ABOX_GIC_IRQ_QUEUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pGicData->irqQueSpinlockIsr);

    return ret;
    }

//abox_gic_iar_queue_get
LOCAL STATUS aboxGicIrqQueGet
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t *      pIrqId
    )
    {
    STATUS          ret = OK;

    SPIN_LOCK_ISR_TAKE (&pGicData->irqQueSpinlockIsr);

    if (!aboxGicIrqQueIsEmpty (pGicData))
        {
        *pIrqId = pGicData->irqQue[pGicData->irqQueStart];
        pGicData->irqQueStart =
            (pGicData->irqQueStart + 1) % ABOX_GIC_IRQ_QUEUE_SIZE;
        }
    else
        {
        ret = ERROR;
        }

    SPIN_LOCK_ISR_GIVE (&pGicData->irqQueSpinlockIsr);

    return ret;
    }

//abox_gic_irq_handler
LOCAL STATUS aboxGicIsr
    (
    VXB_DEV_ID      devId
    )
    {
    ABOX_GIC_DATA * pGicData;
    STATUS          ret = OK;
    uint32_t        giccIar;
    uint32_t        irqId;
    uint32_t        retryCnt = 10;

    pGicData = (ABOX_GIC_DATA *) vxbDevSoftcGet (devId);

    do {
        giccIar = REG_READ_4 (pGicData, pGicData->giccRegBase, GIC_CPU_INTACK);
        irqId = giccIar & GICC_IAR_INT_ID_MASK;

        aboxGicBackupIar (pGicData, giccIar);

        if (irqId < NUM_OF_SGI)
            {
            REG_WRITE_4 (pGicData, pGicData->giccRegBase, GIC_CPU_EOI, giccIar);
            REG_WRITE_4 (pGicData, pGicData->giccRegBase,
                         GIC_CPU_DEACTIVATE, giccIar);
            break;
            }
        else if (irqId < ABOX_GIC_IRQ_MAXNUM)
            {
            if (IS_SPI_OF_ABOX_DMA (irqId)
                || IS_SPI_OF_ABOX_RX_MAILBOX (irqId))
                {
                ret = aboxGicIrqHandle (pGicData, irqId);
                }
            else
                {
                if (aboxGicIrqQuePut (pGicData, irqId) != OK)
                    {
                    DBG_MSG (DBG_ERR,
                             "irqId=0x%d lost due to irq queue is full\n",
                             irqId);
                    }
                else
                    {
                    if (!pGicData->irqWaitFlag)
                        {
                        pGicData->irqWaitFlag = TRUE;
                        (void) semGive (pGicData->irqHandleSem);
                        DBG_MSG (DBG_INFO, "start abox gic irq task\n");
                        }
                    }
                }

            REG_WRITE_4 (pGicData, pGicData->giccRegBase, GIC_CPU_EOI, giccIar);
            break;
            }
    } while (retryCnt--);

    aboxGicBackupIar (pGicData, GICC_IAR_INT_ID_MASK);

    return ret;
    }

//abox_gic_irq_thread
LOCAL void aboxGicIrqTask
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    uint32_t        irqId;

    while (1)
        {
        semTake (pGicData->irqHandleSem, WAIT_FOREVER);
        pGicData->irqWaitFlag = FALSE;

        DBG_MSG (DBG_INFO, "abox gic irq task run\n");

        while (aboxGicIrqQueGet (pGicData, &irqId) == OK)
            {
            DBG_MSG (DBG_INFO, "get irqId=0%d from irq queue\n");
            if (irqId < ABOX_GIC_IRQ_MAXNUM)
                {
                if (aboxGicIrqHandle (pGicData, irqId) != OK)
                    {
                    DBG_MSG (DBG_ERR, "handle abox gic irq failed\n");
                    }
                }
            }
        }
    }

//abox_gic_getTarget
LOCAL uint32_t aboxGicGetTarget
    (
    ABOX_GIC_DATA *     pGicData,
    uint32_t            irqId
    )
    {
    uint32_t            reg;

    reg = REG_READ_4 (pGicData,
                      pGicData->gicdRegBase,
                      GIC_DIST_TARGET + (0x4 * (irqId / 4)));
    return reg;
    }

//abox_gic_setTarget
LOCAL void aboxGicSetTarget
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq,
    uint32_t        cpuTarget
    )
    {
    uint32_t        regVal;

    regVal = REG_READ_4 (pGicData,
                         pGicData->gicdRegBase,
                         GIC_DIST_TARGET + (0x4 * (irq / 4)))
             & ~(0xFF << ((irq % 4) * 8));

    REG_WRITE_4 (pGicData,
                 pGicData->gicdRegBase,
                 GIC_DIST_TARGET + (0x4 * (irq / 4)),
                 regVal | cpuTarget << ((irq % 4) * 8));
    }

//abox_gic_mapping_backup_iar
LOCAL int32_t aboxGicMappingBackupIar
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    char            backupIarName[ABOX_GIC_STR_MAX] = {0};
    uint32_t        cpuId;
    VXB_FDT_DEV *   pFdtDev;
    VXB_RESOURCE *  pResIarBak;
    UINT16          idx;

    if (pGicData->iarBakRegBase != NULL)
        {
        return 0;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pGicData->pDev));
    if (pFdtDev == NULL)
        {
        return -EINVAL;
        }

    for (cpuId = 0; cpuId < ABOX_GIC_MAX; cpuId++)
        {
        if ((0x1 << cpuId) & pGicData->cpuId)
            {
            break;
            }
        }

    if (cpuId == ABOX_GIC_MAX)
        {
        return -EINVAL;
        }

    (void) snprintf (backupIarName, ABOX_GIC_STR_MAX, "backup_cpu%d", cpuId);
    idx = (UINT16) vxFdtPropStrIndexGet (pFdtDev->offset, "reg-names",
                                         backupIarName);
    pResIarBak = vxbResourceAlloc (pGicData->pDev, VXB_RES_MEMORY, idx);
    if ((pResIarBak == NULL) || (pResIarBak->pRes == NULL))
        {
        return -EINVAL;
        }

    pGicData->iarBakRegBase =
        ((VXB_RESOURCE_ADR *)(pResIarBak->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "backup_cpu%d vaddr=0x%llx paddr=0x%llx\n",
             cpuId, pGicData->iarBakRegBase,
             ((VXB_RESOURCE_ADR *)(pResIarBak->pRes))->start);

    return 0;
    }

//abox_gic_terminate_interrupt
LOCAL void aboxGicTerminateInterrupt
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    uint32_t        bakIar;

    if (pGicData->iarBakRegBase == NULL)
        {
        return;
        }

    bakIar = REG_READ_4 (pGicData, pGicData->iarBakRegBase, 0);

    if (bakIar != GICC_IAR_INT_ID_MASK)
        {
        REG_WRITE_4 (pGicData, pGicData->giccRegBase, GIC_CPU_EOI, bakIar);
        }
    }

//abox_gic_disableDistributor
LOCAL void aboxGicDisableDistributor
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    REG_WRITE_4 (pGicData, pGicData->gicdRegBase, GIC_DIST_CTRL, ~1);
    }

//abox_gic_distributorInfo
LOCAL uint32_t aboxGicDistributorInfo
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    return REG_READ_4 (pGicData, pGicData->gicdRegBase, GIC_DIST_CTR);
    }

//abox_gic_setPriority
LOCAL void aboxGicSetPriority
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq,
    uint32_t        priority
    )
    {
    uint32_t          regVal;

    regVal = REG_READ_4 (pGicData,
                         pGicData->gicdRegBase,
                         GIC_DIST_PRI + (0x4 * (irq / 4)))
             & (~(0xFF << ((irq % 4) * 8)));

    REG_WRITE_4 (pGicData,
                 pGicData->gicdRegBase,
                 GIC_DIST_PRI + (0x4 * (irq / 4)),
                 regVal | (priority << ((irq % 4) * 8)));
    }

//abox_gic_getPriority
LOCAL uint32_t aboxGicGetPriority
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq
    )
    {
    uint32_t        regVal;

    regVal = REG_READ_4 (pGicData,
                         pGicData->gicdRegBase,
                         GIC_DIST_PRI + (0x4 * (irq / 4)));

    return regVal & (0xFF << ((irq % 4) * 8));
    }

//abox_gic_disableIRQ
LOCAL void aboxGicDisableIrq
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq
    )
    {
    REG_WRITE_4 (pGicData,
                 pGicData->gicdRegBase,
                 GIC_DIST_ENABLE_CLEAR + (0x4 * (irq / 32)),
                 1 << (irq % 32));
    }

//abox_gic_enableIRQ
LOCAL void aboxGicEnableIrq
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq
    )
    {
    REG_WRITE_4 (pGicData,
                 pGicData->gicdRegBase,
                 GIC_DIST_ENABLE_SET + (0x4 * (irq / 32)),
                 1 << (irq % 32));
    }

//abox_gic_setLevelModel
LOCAL void aboxGicSetLevelModel
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        irq,
    uint32_t        edgeLevel,
    uint32_t        model
    )
    {
    uint32_t        regVal;
    uint32_t        bitShift;

    bitShift = (irq % 16) << 1;

    regVal = REG_READ_4 (pGicData,
                         pGicData->gicdRegBase,
                         GIC_DIST_CONFIG + (0x4 * (irq / 16)))
             & (~(0x3 << bitShift));

    regVal = regVal | (((edgeLevel << 1) | model) << bitShift);

    REG_WRITE_4 (pGicData,
                 pGicData->gicdRegBase,
                 GIC_DIST_CONFIG + (0x4 * (irq / 16)),
                 regVal);
    }

//abox_gic_enableDistributor
LOCAL void aboxGicEnableDistributor
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    REG_WRITE_4 (pGicData, pGicData->gicdRegBase, GIC_DIST_CTRL, 0x1);
    }

//abox_gic_disableInterface
LOCAL void aboxGicDisableInterface
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    REG_WRITE_4 (pGicData, pGicData->giccRegBase, GIC_CPU_CTRL, ~0x3);
    }

//abox_gic_enableInterface
LOCAL void aboxGicEnableInterface
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    REG_WRITE_4 (pGicData, pGicData->giccRegBase, GIC_CPU_CTRL, 0x3);
    }

//abox_gic_setBinaryPoint
LOCAL void aboxGicSetBinaryPoint
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        binPoint
    )
    {
    REG_WRITE_4 (pGicData, pGicData->giccRegBase,
                 GIC_CPU_BINPOINT, binPoint & 0x07);
    }

//abox_gic_InterfacePriorityMask
LOCAL void aboxGicInterfacePriorityMask
    (
    ABOX_GIC_DATA * pGicData,
    uint32_t        priVal
    )
    {
    REG_WRITE_4 (pGicData, pGicData->giccRegBase,
                 GIC_CPU_PRIMASK, priVal & 0xFF);
    }

//abox_gic_init_distributor
LOCAL int32_t aboxGicInitDistributor
    (
    ABOX_CTR_DATA *     pCtrData,
    ABOX_GIC_DATA *     pGicData,
    uint32_t            domain
    )
    {
    ABOX_MAILBOX_DATA * pMailboxRxData;
    uint32_t            irqNum;
    uint32_t            priVal;
    uint32_t            i;
    uint32_t            spiId;
    int32_t             ret = 0;

    pMailboxRxData = (ABOX_MAILBOX_DATA *) vxbDevSoftcGet (pCtrData->mboxRxDev);
    if (pMailboxRxData == NULL)
        {
        return -ENODATA;
        }

    pGicData->cpuId = aboxGicGetTarget (pGicData, 0) & 0xFF;

    ret = aboxGicMappingBackupIar (pGicData);
    if (ret < 0)
        {
        return ret;
        }
    aboxGicTerminateInterrupt (pGicData);

    aboxGicDisableDistributor (pGicData);
    irqNum = (ABOX_GIC_SPI_OFS) *
             ((aboxGicDistributorInfo (pGicData) & 0x1F) + 1);

    aboxGicSetPriority (pGicData, 0, 0xFF);
    priVal = aboxGicGetPriority (pGicData, 0);

    if (IS_SYS_DOMAIN (domain)
        && pGicData->gicdSettingMode == GICD_SET_ALL_WITH_DOMAINS)
        {
        for (i = ABOX_GIC_SPI_OFS; i < irqNum; i++)
            {
            if (IS_SPI_OF_ABOX_DMA (i) || IS_SPI_OF_ABOX_RX_MAILBOX (i))
                {
                continue;
                }

            aboxGicDisableIrq (pGicData, i);

            /* Set priority */

            aboxGicSetLevelModel (pGicData, i,
                                  GICD_LEVEL_SENSITIVE, GICD_1_N_MODEL);
            aboxGicSetPriority (pGicData, i, priVal);
            }
        }

    for (i = 0; i < pMailboxRxData->spiNum; i++)
        {
        spiId = pMailboxRxData->mboxSpi[i];
        if (spiId != 0)
            {
            spiId += ABOX_GIC_SPI_OFS;
            aboxGicSetTarget (pGicData, spiId, pGicData->cpuId);
            aboxGicSetLevelModel (pGicData, spiId,
                                  GICD_LEVEL_SENSITIVE, GICD_1_N_MODEL);
            aboxGicSetPriority (pGicData, spiId, priVal / 2);
            aboxGicEnableIrq (pGicData, spiId);
            }
        }

    aboxGicEnableDistributor (pGicData);

    return 0;
    }

//abox_gic_init_cpu_interface
LOCAL int32_t aboxGicInitCpuInterface
    (
    ABOX_GIC_DATA * pGicData
    )
    {
    uint32_t        priVal;
    uint32_t        i;

    aboxGicDisableInterface (pGicData);

    aboxGicSetPriority (pGicData, 0, 0xFF);
    priVal = aboxGicGetPriority(pGicData, 0);

    /* SGI and PPI */

    for (i = 0; i < ABOX_GIC_SPI_OFS; i++)
        {

        /* Set level-sensitive and 1-N model for PPI */

        if (i > 15)
            {
            aboxGicSetLevelModel (pGicData, i,
                                  GICD_LEVEL_SENSITIVE, GICD_1_N_MODEL);
            }

        /* Disable SGI and PPI interrupts */

        aboxGicDisableIrq (pGicData, i);

        /* Set priority */

        aboxGicSetPriority (pGicData, i, priVal / 2);
        aboxGicEnableIrq (pGicData, i);
        }

    aboxGicEnableInterface (pGicData);
    aboxGicSetBinaryPoint (pGicData, 0);
    aboxGicInterfacePriorityMask (pGicData, 0xFF);

    return 0;
    }


//__abox_gic_init
LOCAL int32_t _aboxGicInit
    (
    ABOX_CTR_DATA * pCtrData,
    ABOX_GIC_DATA * pGicData,
    uint32_t        domain
    )
    {
    int             ret = 0;

    ret = aboxGicInitDistributor (pCtrData, pGicData, domain);
    if (ret < 0)
        {
        return ret;
        }

    if (!IS_SYS_DOMAIN(domain)
        && pGicData->gicdSettingMode == GICD_SET_ALL_WITH_DOMAINS)
        {
        return 0;
        }

    ret = aboxGicInitCpuInterface (pGicData);
    if (ret < 0)
        {
        return ret;
        }

    return (REG_READ_4 (pGicData, pGicData->gicdRegBase, GIC_DIST_TARGET)
            & 0xFF);
    }

//__abox_gic_waiting_init
LOCAL int32_t _aboxGicWaitingInit
    (
    ABOX_CTR_DATA *     pCtrData,
    ABOX_GIC_DATA *     pGicData
    )
    {
    ABOX_MAILBOX_DATA * pMailboxRxData;
    int32_t             ret;

    pMailboxRxData = (ABOX_MAILBOX_DATA *) vxbDevSoftcGet (pCtrData->mboxRxDev);
    if (pMailboxRxData == NULL)
        {
        return -ENODATA;
        }

    if (aboxGicGetTarget (pGicData, pMailboxRxData->spi) == 0)
        {
        return -ENODEV;
        }
    pGicData->cpuId = aboxGicGetTarget (pGicData, 0) & 0xFF;

    DBG_MSG (DBG_INFO, "enter cpu_id:%x\n", pGicData->cpuId);

    ret = aboxGicMappingBackupIar (pGicData);
    if (ret < 0)
        {
        return ret;
        }

    aboxGicTerminateInterrupt (pGicData);

    ret = aboxGicInitCpuInterface (pGicData);
    if (ret < 0)
        {
        return ret;
        }

    return 0;
    }

/******************************************************************************
*
* aboxMailboxProbe - probe for device presence at specific address
*
* RETURNS: OK if probe passes and assumed a valid (or compatible) device.
* ERROR otherwise.
*
*/

LOCAL STATUS aboxGicProbe
    (
    VXB_DEV_ID pDev
    )
    {
    return vxbFdtDevMatch (pDev, aboxGicMatch, NULL);
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

LOCAL STATUS aboxGicAttach
    (
    VXB_DEV_ID      pDev
    )
    {
    VXB_FDT_DEV *   pFdtDev;
    ABOX_GIC_DATA * pGicData;
    VXB_RESOURCE *  pResGicd = NULL;
    VXB_RESOURCE *  pResGicc = NULL;
    uint32_t *      pValue;
    int32_t         len;
    UINT16          idx;
    ABOX_CTR_DATA * pCtrData;

    if (pDev == NULL)
        {
        return ERROR;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        return ERROR;
        }

    pGicData = (ABOX_GIC_DATA *) vxbMemAlloc (sizeof (ABOX_GIC_DATA));
    if (pGicData == NULL)
        {
        return ERROR;
        }

    /* DTS configuration */

    idx = (UINT16) vxFdtPropStrIndexGet (pFdtDev->offset, "reg-names", "gicd");
    pResGicd = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResGicd == NULL) || (pResGicd->pRes == NULL))
        {
        goto errOut;
        }

    pGicData->gicdRegBase = ((VXB_RESOURCE_ADR *)(pResGicd->pRes))->virtAddr;
    pGicData->regHandle = ((VXB_RESOURCE_ADR *)(pResGicd->pRes))->pHandle;

    DBG_MSG (DBG_INFO, "gicd reg vaddr=0x%llx paddr=0x%llx\n",
             pGicData->gicdRegBase,
             ((VXB_RESOURCE_ADR *)(pResGicd->pRes))->start);

    idx = (UINT16) vxFdtPropStrIndexGet (pFdtDev->offset, "reg-names", "gicc");
    pResGicc = vxbResourceAlloc (pDev, VXB_RES_MEMORY, idx);
    if ((pResGicc == NULL) || (pResGicc->pRes == NULL))
        {
        goto errOut;
        }

    pGicData->giccRegBase = ((VXB_RESOURCE_ADR *)(pResGicc->pRes))->virtAddr;

    DBG_MSG (DBG_INFO, "gicc reg vaddr=0x%llx paddr=0x%llx\n",
             pGicData->giccRegBase,
             ((VXB_RESOURCE_ADR *)(pResGicc->pRes))->start);

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "gicd-setting-mode",
                                      &len);
    if (pValue != NULL)
        {
        pGicData->gicdSettingMode = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    pValue = (uint32_t *) vxFdtPropGet (pFdtDev->offset, "samsung,num-of-irq",
                                      &len);
    if (pValue != NULL)
        {
        pGicData->pcmIrqNum = vxFdt32ToCpu (*pValue);
        }
    else
        {
        goto errOut;
        }

    pGicData->pcmIrqHandler = (ABOX_GIC_IRQ_HANDLER_T *)
        vxbMemAlloc (sizeof (ABOX_GIC_IRQ_HANDLER_T) * pGicData->pcmIrqNum);
    if (pGicData->pcmIrqHandler == NULL)
        {
        goto errOut;
        }

    SPIN_LOCK_ISR_INIT (&pGicData->irqQueSpinlockIsr, 0);

    pGicData->irqHandleSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pGicData->irqHandleSem == SEM_ID_NULL)
        {
        goto errOut;
        }

    pGicData->irqTask = taskSpawn (ABOX_GIC_IRQ_TASK_NAME,
                                   ABOX_GIC_IRQ_TASK_PRI,
                                   0,
                                   ABOX_GIC_IRQ_TASK_STACK,
                                   (FUNCPTR) aboxGicIrqTask,
                                   (_Vx_usr_arg_t) pGicData,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (pGicData->irqTask == TASK_ID_ERROR)
        {
        goto errOut;
        }

    pGicData->intRes = vxbResourceAlloc (pDev,
                                         VXB_RES_IRQ,
                                         ABOX_MAIN_CPU_SPI_IDX);
    if (pGicData->intRes == NULL)
        {
        goto errOut;
        }

    DBG_MSG (DBG_INFO, "get int ctrl:%s hVec=%d lVec=%d\n",
             vxFdtGetName(((VXB_RESOURCE_IRQ *)(pGicData->intRes->pRes))->pVxbIntrEntry->node,
                          &len),
             ((VXB_RESOURCE_IRQ *)(pGicData->intRes->pRes))->hVec,
             ((VXB_RESOURCE_IRQ *)(pGicData->intRes->pRes))->lVec);

    /* connect and enable interrupt */

    if (vxbIntConnect (pDev, pGicData->intRes,
                       (VOIDFUNCPTR) aboxGicIsr, pDev) != OK)
        {
        goto errOut;
        }

    if (vxbIntEnable (pDev, pGicData->intRes) == ERROR)
        {
        (void) vxbIntDisconnect (pDev, pGicData->intRes);
        goto errOut;
        }

    /* save pGicData in VXB_DEVICE structure */

    pGicData->pDev = pDev;
    vxbDevSoftcSet (pDev, pGicData);
    pAboxGicData = pGicData;

    pCtrData = ABOX_CTR_GLB_DATA;
    pCtrData->gicDev = pDev;

    return OK;

errOut:
    if (pGicData->intRes != NULL)
        {
        (void) vxbResourceFree (pDev, pGicData->intRes);
        }

    if (pGicData->irqTask != NULL)
        {
        (void) taskDelete (pGicData->irqTask);
        }

    if (pGicData->irqHandleSem != SEM_ID_NULL)
        {
        (void) semDelete (pGicData->irqHandleSem);
        }

    if (pGicData->pcmIrqHandler != NULL)
        {
        vxbMemFree (pGicData->pcmIrqHandler);
        }

    if (pResGicc != NULL)
        {
        (void) vxbResourceFree (pDev, pResGicc);
        }

    if (pResGicd != NULL)
        {
        (void) vxbResourceFree (pDev, pResGicd);
        }

    if (pGicData != NULL)
        {
        pAboxGicData = NULL;
        vxbDevSoftcSet (pDev, NULL);
        vxbMemFree (pGicData);
        }

    return ERROR;
}

/* APIs */

//abox_gic_register_spi_handler
STATUS aboxGicRegisterSpiHandler
    (
    VXB_DEV_ID                  gicDev,
    uint32_t                    spiId,
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc,
    VXB_DEV_ID                  devId
    )
    {
    ABOX_GIC_DATA *             pGicData;

    pGicData =  (ABOX_GIC_DATA *) vxbDevSoftcGet (gicDev);

    if (spiId >= ABOX_GIC_IRQ_MAXNUM)
        {
        return ERROR;
        }

    pGicData->gicIrqHandler[spiId].devId = devId;
    pGicData->gicIrqHandler[spiId].irqFunc = irqFunc;

    DBG_MSG (DBG_INFO, "pGicData->gicIrqHandler[%d].devId=0x%llx "
             "pGicData->gicIrqHandler[%d].irqFunc=0x%llx\n",
             spiId, devId, spiId, irqFunc);

    return OK;
    }

//abox_gic_set_pcm_irq_register
STATUS abox_gic_set_pcm_irq_register
    (
    uint32_t                    pcmIrqId,
    ABOX_GIC_IRQ_HANDLE_FUNC    irqFunc,
    VXB_DEV_ID                  devId
    )
    {
    if (pAboxGicData == NULL)
        {
        return ERROR;
        }

    if (pcmIrqId >= NUM_OF_PCM_IRQ)
        {
        return ERROR;
        }

    pAboxGicData->pcmIrqHandler[pcmIrqId].devId = devId;
    pAboxGicData->pcmIrqHandler[pcmIrqId].irqFunc = irqFunc;

    return OK;
    }

//abox_gic_init
STATUS aboxGicInit
    (
    ABOX_CTR_DATA * pCtrData,
    uint32_t        domain
    )
    {
    ABOX_GIC_DATA * pGicData;
    int32_t         ret;

    if (pCtrData == NULL)
        {
        return ERROR;
        }

    pGicData = (ABOX_GIC_DATA *) vxbDevSoftcGet (pCtrData->gicDev);
    if (pGicData == NULL)
        {
        return ERROR;
        }

    if (domain >= MAX_NUM_OF_DOMAIN)
        {
        return ERROR;
        }

    pGicData->pCtrData = pCtrData;
    pGicData->domain = domain;

    if (pGicData->gicdSettingMode == GICD_SET_ALL_WITHOUT_DOMAINS
        || pGicData->gicdSettingMode == GICD_SET_ALL_WITH_DOMAINS)
        {
        ret = _aboxGicInit (pCtrData, pGicData, domain);
        }
    else
        {
        ret = _aboxGicWaitingInit (pCtrData, pGicData);
        }

    if (ret < 0)
        {
        return ERROR;
        }
    else
        {
        return OK;
        }
    }

//abox_gic_forward_spi
void aboxGicForwardSpi
    (
    uint32_t        spiId
    )
    {
    uint32_t        cpuId;
    uint32_t        priVal;
    ABOX_GIC_DATA * pGicData;

    if (pAboxGicData == NULL)
        {
        return;
        }

    pGicData = pAboxGicData;

    cpuId = aboxGicGetTarget (pGicData, 0) & 0xFF;

    priVal = aboxGicGetPriority (pGicData, 0);

    /* DMA IPC setting: DomX */

    aboxGicSetLevelModel (pGicData, spiId, GICD_EDGE_TRIGGER, GICD_1_N_MODEL);
    aboxGicSetTarget (pGicData, spiId, cpuId);
    aboxGicSetPriority (pGicData, spiId, priVal / 2);
    aboxGicEnableIrq (pGicData, spiId);
    }

