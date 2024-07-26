/* vxbFdtAboxMachine.h - Abox machine driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Abox machine driver specific definitions.
*/

#ifndef __INCvxbFdtAboxMachineh
#define __INCvxbFdtAboxMachineh

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>
#include <subsys/clk/vxbClkLib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ABOX_MACHINE_NAME "aboxMachine"

#define ABOX_MACHINE_ATTACH_TASK_NAME   "aboxMachAttachTask"
#define ABOX_MACHINE_ATTACH_TASK_PRI    (110)
#define ABOX_MACHINE_ATTACH_TASK_STACK  (128 * 1024)

typedef struct vxbAboxMachineDrvCtrl
    {
    VXB_DEV_ID  pDev;
    VXB_CLK_ID  mclk;   //clk_mclk
    uint64_t    mclkValue;  //mclk_value
    uint32_t    samplingRate;
    uint32_t    bitWidth;
    uint32_t    resetPinctrlId;
    uint32_t    idlePinctrlId;
    }VXB_ABOX_MACHINE_DRV_CTRL;

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtAboxMachineh */

