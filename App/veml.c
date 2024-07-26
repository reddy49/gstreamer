/***************************************************************************************************
 * Project        BJEVN60_Display_v1_00
 * (c) copyright  2013-2016
 * Company        Harman/Becker Infotainment ChengDu R&D
 *                All rights reserved
 * Secrecy Level  STRICTLY CONFIDENTIAL
 **************************************************************************************************/
/**
* @file          veml6060_drv.c
* @brief         light sensor driver
* @version       V0.1.0
* @date          2018-5-23
**************************************************************************************************/
/*
 * History:
 * Date          Author        Modification
 * 2018-5-23    TaLi         Create
**************************************************************************************************/

/*-------------------------------------------------------------------------------------------------
 * Include files
 *------------------------------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
// #include <ctypes.h>
#include "veml6060_drv.h"
// #include "flexio_i2c_drv.h"
// #include "flexio_drv.h"
// #include "ucos_ii.h"
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>

// #include "flexio_i2c_drv.h"
// #include "flexio_drv.h"
// #include "ucos_ii.h"

/*-------------------------------------------------------------------------------------------------
 * Macros definition for interanl usage
 *------------------------------------------------------------------------------------------------*/
#define VEML6030_I2C_ADDR     (0x10)


#define VEML6030_RES_FACTOR  (14)
/*-------------------------------------------------------------------------------------------------
 * Enum definition for internal usage
 *------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------
 * Type definition for internal usage
 *------------------------------------------------------------------------------------------------*/
typedef struct
{
    U16 als_sd          : 1; //ALS shut down setting. 0=power on, 1=shutdown
    U16 als_int_en      : 1; //ALS interrupt enable setting. 0 = ALS INT disable, 1 = ALS INT enable
    U16 reserved_2_3    : 2;
    U16 als_pers        : 2;
    U16 als_it          : 4;
    U16 reserved_10     : 1;
    U16 als_gain        : 2;
    U16 reserved_13_15  : 3;
    //    E_GAIN_T    gain;
    //    E_ALS_IT_T  alsIt;
} S_VEML_CFG_T;

typedef struct
{
    U16 valueRangL; /* min of value range */
    U16 valueRangH; /* max of value range */
    S8  step;       /* step need to adjust */
} S_VEML_ADJUST_TBL;

typedef struct
{
    E_GAIN_T    gain;
    E_ALS_IT_T  alsIt;
    U32 resolution;
    U32 asl_max;    /* accuracy ambient light */
    U16 refresh_time;
} S_VEML_RESOLUTION_TBL;


typedef struct
{
    //    E_GAIN_T    gain;
    //    E_ALS_IT_T  alsIt;
    S_VEML_CFG_T    veml_cfg_reg;
    U8 m_nCurResIdx;    //current resolution index
    U32 m_nCurResolution;    //current resolution index
    U32 m_nCurAsl;
    U32 m_nPrevAsl;
    U32 m_nIntEnTime;
} S_VEML_CCB;

/*-------------------------------------------------------------------------------------------------
 * Global variable declaration
 *------------------------------------------------------------------------------------------------*/
#define INTEGRATION_TIME       (550)   // at lease 500ms
static const S_VEML_RESOLUTION_TBL gResolutionInfTbl[] =
{
    // GAIN           IT           Resolution   Max ASL
    //    {E_GAIN_2,      E_ALS_IT_800,    36,        236},
    //    {E_GAIN_2,      E_ALS_IT_400,    72,        472},
    //    {E_GAIN_1,      E_ALS_IT_400,   144,        944},
    //    {E_GAIN_1,      E_ALS_IT_200,   288,       1887},
    //    {E_GAIN_1,      E_ALS_IT_400,   576,       3775},
    //    {E_GAIN_1_4,    E_ALS_IT_200,  1152,       7550},
    //    {E_GAIN_1_4,    E_ALS_IT_100,  2304,      15099},
    //    {E_GAIN_1_4,    E_ALS_IT_50,   4608,      30199},
    //    {E_GAIN_1_8,    E_ALS_IT_50,   9216,      60398},
    //    {E_GAIN_1_8,    E_ALS_IT_25,  18432,     120796}
    {E_GAIN_2,      E_ALS_IT_800, 0.0036 * (1 << VEML6030_RES_FACTOR),    236, 800 + INTEGRATION_TIME},
    {E_GAIN_2,      E_ALS_IT_400, 0.0072 * (1 << VEML6030_RES_FACTOR),    472, 400 + INTEGRATION_TIME},
    {E_GAIN_1,      E_ALS_IT_400, 0.0144 * (1 << VEML6030_RES_FACTOR),    944, 400 + INTEGRATION_TIME},
    {E_GAIN_1,      E_ALS_IT_200, 0.0288 * (1 << VEML6030_RES_FACTOR),   1887, 200 + INTEGRATION_TIME},
    {E_GAIN_1,      E_ALS_IT_100, 0.0576 * (1 << VEML6030_RES_FACTOR),   3775, 400 + INTEGRATION_TIME},
    {E_GAIN_1_4,    E_ALS_IT_200, 0.1152 * (1 << VEML6030_RES_FACTOR),   7550, 200 + INTEGRATION_TIME},
    {E_GAIN_1_4,    E_ALS_IT_100, 0.2304 * (1 << VEML6030_RES_FACTOR),  15099, 100 + INTEGRATION_TIME},
    {E_GAIN_1_4,    E_ALS_IT_50,  0.4608 * (1 << VEML6030_RES_FACTOR),  30199,  50 + INTEGRATION_TIME},
    {E_GAIN_1_8,    E_ALS_IT_50,  0.9216 * (1 << VEML6030_RES_FACTOR),  60398,  50 + INTEGRATION_TIME},
    {E_GAIN_1_8,    E_ALS_IT_25,  1.8432 * (1 << VEML6030_RES_FACTOR), 120796,  25 + INTEGRATION_TIME}
};

/* last one detect range */
#define ASL_DEFAULT_RES_IDX     (array_len(gResolutionInfTbl) - 1)
#define VELM_READ_VALUE_MAX     (0xFFFF)
static const S_VEML_ADJUST_TBL gVemlAdjTbl[] =
{
    {(VELM_READ_VALUE_MAX *    0), (VELM_READ_VALUE_MAX * 0.1), -2},/*   0% ~  10% */
    {(VELM_READ_VALUE_MAX *  0.1), (VELM_READ_VALUE_MAX * 0.2), -1},/*  10% ~  20% */
    {(VELM_READ_VALUE_MAX *  0.2), (VELM_READ_VALUE_MAX * 0.8),  0},/*  20% ~  80% */
    {(VELM_READ_VALUE_MAX *  0.8), (VELM_READ_VALUE_MAX * 0.9),  1},/*  80% ~  90% */
    {(VELM_READ_VALUE_MAX *  0.9), (VELM_READ_VALUE_MAX * 1.0),  2},/*  90% ~ 100% */
};


static S_VEML_CCB s_stVemlCcb;

/*-------------------------------------------------------------------------------------------------
 * Static variable definition
 *------------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------------
 * Function prototype declaration for internal usage
 *------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------
 * Function implemention
 *------------------------------------------------------------------------------------------------*/
/****************************************************************************
 * @brief    velm6030_drv_setting_gain
 *
 * @param[]  gain      :
 *
 * @returns:
 ****************************************************************************/


/****************************************************************************
 * @brief    velm6030_drv_power
 *
 * @param[]  pwr       :
 *
 * @returns:
 ****************************************************************************/
void velm6030_drv_power(E_PWR_T pwr)
{
    U16 val = 0;

    flexio_i2c_read(VEML6030_I2C_ADDR, CMD_CODE_ALS_CFG, &val,2);
    if (pwr)
    {
        val |= 0x01;
    }
    else
    {
        val &= 0xfffe;
    }
    //flexio_i2c_write(VEML6030_I2C_ADDR, CMD_CODE_ALS_CFG, 2);

}


/****************************************************************************
 * @brief    velm6030_drv_power_mode
 *
 * @param[]  pwrMode   :
 *
 * @returns:
 ****************************************************************************/
void velm6030_drv_power_mode(E_STATUS_T pwrMode)
{
    U16 val = 0;

    flexio_i2c_read(VEML6030_I2C_ADDR, CMD_CODE_POWER_SAVING, (U8 *)&val, 2);
    if (pwrMode)
    {
        val |= 0x01;
    }
    else
    {
        val &= 0xfffe;
    }
    //flexio_i2c_write(VEML6030_I2C_ADDR, CMD_CODE_POWER_SAVING, 2);

}

/*
static S32 velm6030_drv_setting(S_VEML_CFG_T *cfg)
{

    flexio_i2c_write(VEML6030_I2C_ADDR, CMD_CODE_ALS_CFG, cfg);
    return 0;
}
*/

static S32 velm6030_drv_update_cfg(S_VEML_CCB *pstCcb, const S_VEML_RESOLUTION_TBL *res)
{
    pstCcb->veml_cfg_reg.als_gain = res->gain;
    pstCcb->veml_cfg_reg.als_it = res->alsIt;
    pstCcb->m_nCurResolution = res->resolution;
    time_t currentTime = time(NULL);
    pstCcb->m_nIntEnTime = currentTime + res->refresh_time;

    return 0;
	    //velm6030_drv_setting(&pstCcb->veml_cfg_reg);
}


/****************************************************************************
 * @brief    velm6030_drv_init
 *
 *
 *
 * @returns:
 ****************************************************************************/
void velm6030_drv_init(void)
{
    S_VEML_CCB *pstCcb = &s_stVemlCcb;

    memset(pstCcb, 0, sizeof(S_VEML_CCB));

    pstCcb->m_nCurResIdx = ASL_DEFAULT_RES_IDX;
    // flexio_drv_init(E_FLEXIO_0);
    // flexio_drv_i2c_master_init();
    // flexio_drv_enable(E_FLEXIO_0);
    pstCcb->veml_cfg_reg.als_sd = E_PWR_ON;     //0 = ALS power on, 1 = ALS shut down
    pstCcb->veml_cfg_reg.als_int_en = E_DISABLE;//0 = ALS INT disable, 1 = ALS INT enable
    pstCcb->veml_cfg_reg.als_pers = 0;

    velm6030_drv_power(s_stVemlCcb.veml_cfg_reg.als_sd);
    sleep(3);
    velm6030_drv_power_mode(E_DISABLE);
    velm6030_drv_update_cfg(pstCcb, &gResolutionInfTbl[pstCcb->m_nCurResIdx]);
    //velm6030_drv_setting_gain(s_stVemlCcb.veml_cfg_reg.als_gain);
    //velm6030_drv_setting_als_it(s_stVemlCcb.veml_cfg_reg.als_it);
}


static S32 velm6030_drv_check_als(U16 value)
{
    U8 i;
    S8 adj_step = 0;
    S_VEML_CCB *pstCcb = &s_stVemlCcb;

    for (i = 0; i < array_len(gVemlAdjTbl); i++)
    {
        if ((value >= gVemlAdjTbl[i].valueRangL) && (value <= gVemlAdjTbl[i].valueRangH))
        {
            adj_step = gVemlAdjTbl[i].step;
            break;
        }
    }

    if (0 != adj_step)
    {
        S8 tgt_idx = (S8)pstCcb->m_nCurResIdx + adj_step;
        if ((tgt_idx >= 0) && (tgt_idx < array_len(gResolutionInfTbl)))
        {
            pstCcb->m_nCurResIdx = tgt_idx;
            velm6030_drv_update_cfg(pstCcb, &gResolutionInfTbl[pstCcb->m_nCurResIdx]);
            return TRUE;
        }
    }
    return FALSE;
}



/****************************************************************************
 * @brief    velm6030_drv_read_als
 *
 *
 *
 * @returns:
 ****************************************************************************/
S32 velm6030_drv_read_als(U32 *value)
{
    S_VEML_CCB *pstCcb = &s_stVemlCcb;
    U16 sersor_value = 0;
    status_t rtn = STATUS_SUCCESS;

    /* read asl after valid asl avaliable */
    if (time(NULL) < pstCcb->m_nIntEnTime)
    {
        *value = pstCcb->m_nPrevAsl;
        return -1;
    }
    rtn = flexio_i2c_read(VEML6030_I2C_ADDR, CMD_CODE_ALS, (U8 *)&sersor_value, 2);
    /* TODO: !!! if IIC read error, how to handle . currently no return state from Flexio_i2c */
    if (rtn == STATUS_SUCCESS)
    {
        *value = (U32)(((U32)sersor_value * pstCcb->m_nCurResolution) >> VEML6030_RES_FACTOR);
        pstCcb->m_nCurAsl = *value;
        /* adjust the Resution through IT & Gain  */
        velm6030_drv_check_als(sersor_value);
    }

    if (pstCcb->m_nPrevAsl != pstCcb->m_nCurAsl)
    {
        pstCcb->m_nPrevAsl = pstCcb->m_nCurAsl;
        return 0;
    }

    return 0;
}


int flexio_i2c_read(int addr, int reg, U8 *buff, int len)
{
    int fd;
    int ret;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[2];
    U8 reg_buff[1];

    fd = open("/dev/i2c-2", O_RDWR);
    if (fd < 0)
    {
        printf("open i2c-2 failed\n");
        return -1;
    }

    reg_buff[0] = reg;
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = reg_buff;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buff;

    data.msgs = msgs;
    data.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &data);
    if (ret < 0)
    {
        printf("read i2c-2 failed\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
/*
int flexio_i2c_write(int addr, int reg, int len) {
    int fd;
    int ret;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[1];
    U8 reg_buff[1];

    fd = open("/dev/i2c-2", O_RDWR);
    if (fd < 0)
    {
        printf("open i2c-2 failed\n");
        return -1;
    }

    reg_buff[0] = reg;
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = len + 1;
    msgs[0].buf = reg_buff;

    data.msgs = msgs;
    data.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &data);
    if (ret < 0)
    {
        printf("write i2c-2 failed\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
*/
int main(void)
{
	U32 value;
	velm6030_drv_init();
	while(1)
	{
		//velm6030_drv_read_als(&value);
		printf("velm6060 Address Value = %ls\n",velm6030_drv_read_als(&value));
		usleep(2000);

	}
}
