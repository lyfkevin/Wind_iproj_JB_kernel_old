/*
  * Copyright (C) 2009 LGE, Inc.
  *
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General Public
  * License version 2, as published by the Free Software Foundation, and
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  */

#ifdef CONFIG_LGE_ISA1200

#include <linux/types.h>
#include <linux/list.h>
#include <linux/err.h>
#include <mach/msm_iomap.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <board_lge.h>

#include "devices_lge_325.h"

#include <linux/i2c.h>
#include "lge_isa1200.h"


#define VIBE_IC_VOLTAGE		        3000000
#if defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
#define MSM_PDM_BASE_REG		           		0x18800040
#define GP_MN_CLK_MDIV_REG		                0xC
#define GP_MN_CLK_NDIV_REG		                0x10
#define GP_MN_CLK_DUTY_REG		                0x14

//#define GP_MN_M_DEFAULT			        2
//#define GP_MN_N_DEFAULT			        1304
#define GP_MN_M_DEFAULT			        7
#define GP_MN_N_DEFAULT			        1141

#define GP_MN_D_DEFAULT		            	(GP_MN_N_DEFAULT >> 1)
#define PWM_MAX_DUTY GP_MN_N_DEFAULT - GP_MN_M_DEFAULT
#define PWM_MIN_DUTY GP_MN_M_DEFAULT
#define PWM_MAX_HALF_DUTY		((GP_MN_D_DEFAULT >> 1) - 8) /* 127 - 8 = 119  minimum operating spec. should be checked */

#define GPMN_M_MASK				0x01FF
#define GPMN_N_MASK				0x1FFF
#define GPMN_D_MASK				0x1FFF
#else
#define REG_WRITEL(value, reg)	        writel(value, (MSM_CLK_CTL_BASE+reg))
#define REG_READL(reg)	        readl((MSM_CLK_CTL_BASE+reg))

#define GPn_MD_REG(n)		                (0x2D00+32*(n))
#define GPn_NS_REG(n)		                (0x2D24+32*(n))
#define GP1_MD_REG                                 GPn_MD_REG(1)
#define GP1_NS_REG                                  GPn_NS_REG(1)
#endif


static struct regulator *snddev_reg_l1 = NULL;


static int vibrator_power_set(int enable)
{
	int rc;

#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_DCM)
	if(lge_bd_rev > LGE_REV_B)
		return 0;
#endif

	//                                                           

	if (enable) {

		if (NULL == snddev_reg_l1) {

			snddev_reg_l1 = regulator_get(NULL, "8901_l1");

			if (IS_ERR(snddev_reg_l1)) {
				pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
				"l1", PTR_ERR(snddev_reg_l1));
				return -EBUSY;
			}

			rc = regulator_set_voltage(snddev_reg_l1, VIBE_IC_VOLTAGE, VIBE_IC_VOLTAGE);

			if (rc < 0)
				pr_err("%s: regulator_set_voltage(l1) failed (%d)\n",
			__func__, rc);

			rc = regulator_enable(snddev_reg_l1);

			if (rc < 0)
				pr_err("%s: regulator_enable(l1) failed (%d)\n", __func__, rc);
		}

	} else {
/*		if (regulator_is_enabled(snddev_reg_l1)) { */
		if (snddev_reg_l1) {
			rc = regulator_disable(snddev_reg_l1);

			if (rc < 0)
				pr_err("%s: regulator_enable(l1) failed (%d)\n", __func__, rc);

			regulator_put(snddev_reg_l1);
			snddev_reg_l1 = NULL;
		}
	}

/*	regulator_put(snddev_reg_l1); */

	return 0;
}

extern int vibe_level;
static int lge_isa1200_clock(int enable, int amp)
{
#if defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
	uint M_VAL = GP_MN_M_DEFAULT;
	uint N_VAL = GP_MN_N_DEFAULT;
	uint D_VAL = GP_MN_D_DEFAULT;
	void __iomem *vib_base_ptr = 0;

	vib_base_ptr = ioremap_nocache(MSM_PDM_BASE_REG,0x20 );

	writel((M_VAL & GPMN_M_MASK), vib_base_ptr + GP_MN_CLK_MDIV_REG );
	writel((~( N_VAL - M_VAL )&GPMN_N_MASK), vib_base_ptr + GP_MN_CLK_NDIV_REG);

	if (enable)
	{
		vibrator_power_set(1);
		D_VAL = ((GP_MN_D_DEFAULT*amp) >> 7)+ GP_MN_D_DEFAULT;
		gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_PWM, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), GPIO_CFG_ENABLE);
		if (D_VAL > PWM_MAX_DUTY ) D_VAL = PWM_MAX_DUTY;
		if (D_VAL < PWM_MIN_DUTY ) D_VAL = PWM_MIN_DUTY;

		writel(D_VAL & GPMN_D_MASK, vib_base_ptr + GP_MN_CLK_DUTY_REG);
	} else {
		vibrator_power_set(0);
		writel(GP_MN_D_DEFAULT & GPMN_D_MASK, vib_base_ptr + GP_MN_CLK_DUTY_REG);

		gpio_tlmm_config(GPIO_CFG(GPIO_LIN_MOTOR_PWM, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	}
	iounmap(vib_base_ptr);
#else
	if (enable) {
		if(lge_bd_rev == LGE_REV_B)
			vibrator_power_set(1);
		
                    REG_WRITEL( 
                    ((( 0 & 0xffU) <<16U) + /* N_VAL[23:16] */
                    (1U<<11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
                    (0U<<10U) +  /* CLK_INV[10]       : Disable(0) */
                    (1U<<9U) +	 /* CLK_BRANCH_ENA[9] : Enable(1) */
                    (0U<<8U) +   /* NMCNTR_EN[8]      : Enable(1) */
                    (0U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (0U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (0U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    //printk("GPIO_LIN_MOTOR_PWM is enabled. pxo clock.");
	} else {	
		if(lge_bd_rev == LGE_REV_B)
		vibrator_power_set(0);
                    REG_WRITEL( 
                    ((( 0 & 0xffU) <<16U) + /* N_VAL[23:16] */
                    (0U<<11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
                    (0U<<10U) +  /* CLK_INV[10] 	  : Disable(0) */
                    (0U<<9U) +	 /* CLK_BRANCH_ENA[9] : Disable(0) */
                    (0U<<8U) +   /* NMCNTR_EN[8]      : Disable(0) */
                    (1U<<7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
                    (0U<<5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
                    (0U<<3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
                    (0U<<0U)),   /* SRC_SEL[2:0]      : pxo (0)  */
                    GP1_NS_REG);
                    //printk("GPIO_LIN_MOTOR_PWM is disabled.");
	}
#endif

	return 0;
}

static struct isa1200_reg_cmd isa1200_init_seq[] = {
	{LGE_ISA1200_HCTRL2, 0x00},		/* bk : release sw reset */

	{LGE_ISA1200_SCTRL , 0x0F}, 		/* LDO:3V */ // 0x0F

	{LGE_ISA1200_HCTRL0, 0x10},		/* [4:3]10 : PWM Generation Mode [1:0]01 : Divider 1/256 */
#if defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
	{LGE_ISA1200_HCTRL1, 0x40},		/* [7] 1 : Ext. Clock Selection, [5] 0:LRA, 1:ERM */
#else
	{LGE_ISA1200_HCTRL1, 0xC0},		/* [7] 1 : Ext. Clock Selection, [5] 0:LRA, 1:ERM */
#endif
	{LGE_ISA1200_HCTRL3, 0x33},		/* [6:4] 1:PWM/SE Generation PLL Clock Divider */

	{LGE_ISA1200_HCTRL4, 0x81},		/* bk */
	{LGE_ISA1200_HCTRL5, 0x3a},		/* [7:0] PWM High Duty(PWM Gen) 0-6B-D6 */ /* TODO make it platform data? */
	{LGE_ISA1200_HCTRL6, 0x74},		/* [7:0] PWM Period(PWM Gen) */ /* TODO make it platform data? */

};

static struct isa1200_reg_seq isa1200_init = {
   .reg_cmd_list = isa1200_init_seq,
   .number_of_reg_cmd_list = ARRAY_SIZE(isa1200_init_seq),
};

static struct lge_isa1200_platform_data lge_isa1200_platform_data = {
	.vibrator_name = "vibrator",

	.gpio_hen = GPIO_LIN_MOTOR_EN,
	.gpio_len = GPIO_LIN_MOTOR_EN,

	.clock = lge_isa1200_clock,

	.max_timeout = 30000,
#if defined(CONFIG_MACH_LGE_325_BOARD_LGU) || defined(CONFIG_MACH_LGE_325_BOARD_VZW)
	.default_vib_strength = 102, // Duty : 1%
#else
	.default_vib_strength = 117,
#endif
	.init_seq = &isa1200_init,
};

#define ISA1200_SLAVE_ADDR 0x90

static struct i2c_board_info lge_i2c_isa1200_board_info[] = {
	{
		I2C_BOARD_INFO("lge_isa1200", ISA1200_SLAVE_ADDR>>1),
		.platform_data = &lge_isa1200_platform_data
	},
};


void __init lge_add_misc_devices(void)
{
	printk(KERN_INFO "LGE: %s \n", __func__);

	/* isa1200 initialization code */
	i2c_register_board_info(LGE_GSBI_BUS_ID_VIB_ISA1200,
		lge_i2c_isa1200_board_info,
		ARRAY_SIZE(lge_i2c_isa1200_board_info));
}
#endif
