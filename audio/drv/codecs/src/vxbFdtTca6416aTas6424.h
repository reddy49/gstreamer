/* vxbFdtTca6416aTas6424.h - Tca6416a driver header file */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides Tca6416a driver specific definitions.
*/

#ifndef __INCvxbFdtTca6416aTas6424h
#define __INCvxbFdtTca6416aTas6424h

#include <vxWorks.h>
#include <hwif/vxbus/vxbLib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCA6416A_DRV_NAME   "tca6416a"

#define TCA6416A_MUTE       (1)
#define TCA6416A_UNMUTE     (0)

/* registers */

#define TCA6416A_IN_PORT_0			0x00
#define TCA6416A_IN_PORT_1			0x01
#define TCA6416A_OUT_PORT_0			0x02
#define TCA6416A_OUT_PORT_1			0x03
#define TCA6416A_POLAR_INVERSION_0	0x04
#define TCA6416A_POLAR_INVERSION_1	0x05
#define TCA6416A_CONFIG_0			0x06
#define TCA6416A_CONFIG_1			0x07
#define TCA6416A_MAX				TCA6416A_CONFIG_1


typedef struct tca6416aData
    {
    VXB_DEV_ID          pDev;   //dev
    UINT16              i2cDevAddr;
    SEM_ID              i2cMutex;
    } TCA6416A_DATA;

extern STATUS tca6416aConfigAmpStandby
        (
        uint32_t      ampId,
        uint32_t      status
        );
extern STATUS tca6416aConfigAmpMute
        (
        uint32_t      ampId,
        uint32_t      mute
        );

#ifdef __cplusplus
    }
#endif

#endif /* __INCvxbFdtTca6416aTas6424h */

