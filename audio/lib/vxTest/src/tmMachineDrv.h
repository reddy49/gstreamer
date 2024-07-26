/* tmMachineDrv.h - test machine driver header file */


#ifndef __INCtestMachineh
#define __INCtestMachineh

#include <vxWorks.h>
#include <subsys/clk/vxbClkLib.h>
#include "tmAudioNew.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABOX_MACHINE_NAME "aboxMachine"

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


LOCAL int32_t testSocSndCardSuspendPost (VX_SND_SOC_CARD *card);
LOCAL int32_t testSocSndCardResumePre (VX_SND_SOC_CARD *card);
LOCAL int32_t dummyHwParams
    (
    SND_PCM_SUBSTREAM *         substream,
    VX_SND_PCM_HW_PARAMS *      params
    );

STATUS test_hw_params_fixup_helper
    (
    VX_SND_SOC_PCM_RUNTIME *    rtd,
    VX_SND_PCM_HW_PARAMS *      params
    );

#ifdef __cplusplus
    }
#endif

#endif /* __INCtestMachineh */

