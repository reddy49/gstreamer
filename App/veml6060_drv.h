/***************************************************************************************************
 * Project        BJEVN60_Display_v1_00
 * (c) copyright  2013-2016
 * Company        Harman/Becker Infotainment ChengDu R&D
 *                All rights reserved
 * Secrecy Level  STRICTLY CONFIDENTIAL
 **************************************************************************************************/
/**
* @file          veml6060_drv.h
* @brief         light sensor driver
* @version       V0.1.0
* @date          2018-5-23
**************************************************************************************************/
/*
 * History:
 * Date          Author        Modification
 * 2018-5-23    TaLi         Create
**************************************************************************************************/
#ifndef __VEML6060_DRV_H__
#define __VEML6060_DRV_H__


/*-------------------------------------------------------------------------------------------------
 * Include files
 *------------------------------------------------------------------------------------------------*/
#include <stdint.h>

typedef uint16_t U16;
typedef int32_t S32;
typedef uint32_t U32;
typedef uint8_t U8;
typedef int8_t S8;

#define TRUE 1
#define FALSE 0

typedef int status_t;
#define STATUS_SUCCESS 0

#define array_len(x) (sizeof(x) / sizeof((x)[0]))
/*-------------------------------------------------------------------------------------------------
 * Macros definition for global usage
 *------------------------------------------------------------------------------------------------*/
#define CMD_CODE_ALS_CFG    (0)
#define CMD_CODE_ALS_WH     (1)
#define CMD_CODE_ALS_WL     (2)
#define CMD_CODE_POWER_SAVING       (3)
#define CMD_CODE_ALS        (4)
#define CMD_CODE_WHITE      (5)
#define CMD_CODE_ALS_INT    (6)

/*-------------------------------------------------------------------------------------------------
 * Enum definition for global usage
 *------------------------------------------------------------------------------------------------*/

typedef enum
{
    E_GAIN_2   = 0b01,/* als gain 2 */
    E_GAIN_1   = 0b00,/* als gain 1 */
    E_GAIN_1_4 = 0b11,/* als gain 1/4 */
    E_GAIN_1_8 = 0b10,/* als gain 1/8 */
    E_GAIN_END
} E_GAIN_T;



typedef enum
{
    E_ALS_IT_800 = 0b0011,      /* 800ms */
    E_ALS_IT_400 = 0b0010,     /* 400ms */
    E_ALS_IT_200 = 0b0001,     /* 200ms */
    E_ALS_IT_100 = 0b0000,     /* 100ms */
    E_ALS_IT_50  = 0b1000,     /* 50ms */
    E_ALS_IT_25  = 0b1100     /* 25ms */
} E_ALS_IT_T;

typedef enum
{
    E_DISABLE = 0X0,
    E_ENABLE = ! E_DISABLE
} E_STATUS_T;


typedef enum
{
    E_PWR_ON = 0X0,
    E_PWR_DOWN = ! E_PWR_ON
} E_PWR_T;


/*-------------------------------------------------------------------------------------------------
 * Type definition for global usage
 *------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------
 * Global variable declaration
 *------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------
 * Function prototype declaration for global usage
 *------------------------------------------------------------------------------------------------*/

/****************************************************************************
 * @brief    velm6030_drv_init
 *
 *
 *
 * @returns:
 ****************************************************************************/
void velm6030_drv_init(void);
/****************************************************************************
 * @brief    velm6030_drv_read_als
 *
 *
 *
 * @returns:
 ****************************************************************************/
extern S32 velm6030_drv_read_als(U32 *value);

int flexio_i2c_read(int addr, int reg, U8 *buff, int len);
int flexio_i2c_write(int addr, int reg, U8 * val, int len);
#endif  /* __VEML6060_DRV_H__ */

