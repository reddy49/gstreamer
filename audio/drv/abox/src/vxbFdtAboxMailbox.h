/* vxbFdtAboxMailbox.h - Samsung Abox mailbox driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Samsung Abox mailbox driver specific definitions.
*/

#ifndef __INCvxbFdtAboxMailboxh
#define __INCvxbFdtAboxMailboxh

#include <vxWorks.h>
#include <aboxIpc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DTS value defines */

#define ABOX_MAILBOX_TX             1
#define ABOX_MAILBOX_RX             2

/* DTS value defines end */

#define ABOX_MAILBOX_DRV_NAME       "aboxMailbox"

#define ABOX_MAILBOX_IRQ_TASK_NAME  "aboxMailboxIrqTask"
#define ABOX_MAILBOX_IRQ_TASK_PRI   79
#define ABOX_MAILBOX_IRQ_TASK_STACK (32 * 1024)

#define ABOX_MAILBOX_STR_MAX        64

#define ABOX_MAILBOX_TX_ID          0
#define ABOX_MAILBOX_RX_ID          4
#define ABOX_MAILBOX_SPI_QUE_SIZE   1000


/* register offset */

#define ABOX_MAILBOX_MCUCTRL        0x00

#define ABOX_MAILBOX_INTGR0         0x08
#define ABOX_MAILBOX_INTCR0         0x0C
#define ABOX_MAILBOX_INTMR0         0x10
#define ABOX_MAILBOX_INTSR0         0x14
#define ABOX_MAILBOX_INTMSR0        0x18

#define ABOX_MAILBOX_INTGR1         0x1C
#define ABOX_MAILBOX_INTCR1         0x20
#define ABOX_MAILBOX_INTMR1         0x24
#define ABOX_MAILBOX_INTSR1         0x28
#define ABOX_MAILBOX_INTMSR1        0x2C

#define ABOX_MAILBOX_INTGR2         0x30
#define ABOX_MAILBOX_INTCR2         0x34
#define ABOX_MAILBOX_INTMR2         0x38
#define ABOX_MAILBOX_INTSR2         0x3C
#define ABOX_MAILBOX_INTMSR2        0x40

#define ABOX_MAILBOX_INTGR3         0x44
#define ABOX_MAILBOX_INTCR3         0x48
#define ABOX_MAILBOX_INTMR3         0x4C
#define ABOX_MAILBOX_INTSR3         0x50
#define ABOX_MAILBOX_INTMSR3        0x54

typedef enum aboxMailboxGenId
    {
    ABOX_MAILBOX_GENERATE0 = 0,
    ABOX_MAILBOX_GENERATE1,
    ABOX_MAILBOX_GENERATE2,
    ABOX_MAILBOX_GENERATE3,
    } ABOX_MAILBOX_GEN_ID;

typedef struct aboxMailboxIpcHandler
    {
    ABOX_IPC_HANDLE_FUNC    ipcFunc;
    VXB_DEV_ID              devId;
    } ABOX_MAILBOX_IPC_HANDLER_T;

typedef struct aboxMailboxIpcQue
    {
    uint32_t        ipcId;
    ABOX_IPC_MSG    msg;
    } ABOX_MAILBOX_IPC_QUE;

typedef struct aboxMailboxData
    {
    VXB_DEV_ID                  pDev;
    VIRT_ADDR                   regBase;
    void *                      regHandle;
    VXB_DEV_ID                  gicDev;
    uint32_t                    id;
    uint32_t                    type;
    uint32_t                    spi; //This is from ADSP0 SPI
    uint32_t *                  mboxSpi;
    uint32_t                    spiNum;
    uint32_t                    ipcTxOfs;
    uint32_t                    ipcRxOfs;
    spinlockIsr_t               txMsgSpinlockIsr;
    spinlockIsr_t               rxMsgSpinlockIsr;
    spinlockIsr_t               ipcQueSpinlockIsr;
    ABOX_MAILBOX_IPC_QUE        ipcQue[ABOX_MAILBOX_SPI_QUE_SIZE];
    int32_t                     ipcQueStart;
    int32_t                     ipcQueEnd;
    SEM_ID                      irqHandleSem;
    TASK_ID                     irqTask;
    BOOL                        irqWaitFlag;
    ABOX_MAILBOX_IPC_HANDLER_T  ipcHandler;
    } ABOX_MAILBOX_DATA;

extern STATUS aboxMailboxRegisterIpcHandler
    (
    VXB_DEV_ID              pMboxDev,
    ABOX_IPC_HANDLE_FUNC    handler,
    VXB_DEV_ID              devId
    );

extern STATUS aboxMailboxUnregisterIpcHandler
    (
    VXB_DEV_ID  pDev
    );

extern STATUS aboxMailboxSendMsg
    (
    VXB_DEV_ID              pDev,
    uint32_t                ipcId,
    const ABOX_IPC_MSG *    pMsg,
    uint32_t                msgSize,
    uint32_t                timeoutUs,
    uint32_t                adsp
    );

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtAboxMailboxh */

