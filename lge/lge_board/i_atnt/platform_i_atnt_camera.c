/*
 * Copyright (C) 2011 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/bootmem.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include "board_i_atnt.h"
#include "devices_i_atnt.h"


/* IMX105 Main Camera - 8M Bayer Camera*/
#define GPIO_CAM_MCLK						32
#define GPIO_CAM_I2C_SDA					47
#define GPIO_CAM_I2C_SCL					48
#define GPIO_8M_CAM_RESET_N					157
#if defined(LGE_REV_A)
#define GPIO_8M_CAM_VCM_EN					156
#else
#define GPIO_8M_CAM_VCM_EN					94
#define GPIO_VT_OSC_EN						156
#endif
#define CAM_I2C_SLAVE_ADDR					0x1A>>1

/* MT9M114 VT Camera - 1.3M SOC Camera*/
#define GPIO_VT_CAM_RESET_N					57

#define VT_CAM_I2C_SLAVE_ADDR				0x48>>1

/* LM3559 LED Flash driver*/
#define GPIO_LED_FLASH_EN			    	154
#define LED_FLASH_I2C_SLAVE_ADDR			0xA6>>1

/* Seperate main/vt camera power*/
static struct regulator *ldo1_1p2;
static struct regulator *ldo9_2p8;
static struct regulator *lvs0_1p8;
static struct regulator *ldo8_2p8;
static struct regulator *vt_ldo4_2p8;
static struct regulator *vt_lvs3_1p8;
static struct regulator *vt_lvs1_1p8;
//========================================================================

#ifdef CONFIG_LGE_CAMERA
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(GPIO_CAM_MCLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), 
	GPIO_CFG(GPIO_8M_CAM_RESET_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_VT_CAM_RESET_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_VT_OSC_EN, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
};
static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(GPIO_CAM_MCLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), 
	GPIO_CFG(GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), 
	GPIO_CFG(GPIO_8M_CAM_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_VT_CAM_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 
	GPIO_CFG(GPIO_VT_OSC_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static int config_camera_on_gpios(void)
{
	int rc = 0;

	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	return rc;
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

static int config_camera_on_gpios_vt_cam(void)
{
	int rc = 0;

	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	return rc;
}

static void config_camera_off_gpios_vt_cam(void)
{

	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

void  main_camera_power_on (void)
{
	/*Main Cam Power*/
	printk("%s: \n", __func__);
	
	/* +2V8_8MCAM_AVDD*/
	ldo9_2p8 = regulator_get(NULL, "8058_l9");
	if (IS_ERR(ldo9_2p8)) {
		pr_err("%s: VREG LDO9 get failed\n", __func__);
		ldo9_2p8 = NULL;
		return;
	}
	regulator_set_voltage(ldo9_2p8, 2800000, 2800000);
	regulator_enable(ldo9_2p8);

	/* +1V2_8MCAM_DVDD*/
	ldo1_1p2 = regulator_get(NULL, "8058_l1");
	if (IS_ERR(ldo1_1p2)) {
		pr_err("%s: VREG LDO1 get failed\n", __func__);
		ldo1_1p2 = NULL;
		return;
	}
	regulator_set_voltage(ldo1_1p2, 1200000, 1200000);
	regulator_enable(ldo1_1p2);

	/* +2V8_8MCAM_AF*/
	ldo8_2p8 = regulator_get(NULL, "8058_l8");
	if (IS_ERR(ldo8_2p8)) {
		pr_err("%s: VREG ldo8 get failed\n", __func__);
		ldo8_2p8 = NULL;
		return;
	}
	regulator_set_voltage(ldo8_2p8, 2800000, 2800000);
	regulator_enable(ldo8_2p8);

	/*I2C & Main Power*/
	/* +1V8_8MCAM_VIO*/
	lvs0_1p8 = regulator_get(NULL, "8058_lvs0");
	if (IS_ERR(lvs0_1p8)) {
		pr_err("%s: LVS0 get failed\n", __func__);
		lvs0_1p8 = NULL;
		return;
	}
	regulator_set_voltage(lvs0_1p8, 1800000, 1800000);
	regulator_enable(lvs0_1p8);

}


void  main_camera_power_off (void)
{
	printk("%s: \n", __func__);

	if (ldo1_1p2) {
		regulator_disable(ldo1_1p2);
		regulator_put(ldo1_1p2);
	}

	if (ldo9_2p8) {
		regulator_disable(ldo9_2p8);
		regulator_put(ldo9_2p8);
	}

	if (ldo8_2p8) {
		regulator_disable(ldo8_2p8);
		regulator_put(ldo8_2p8);
	}

	if (lvs0_1p8) {
		regulator_disable(lvs0_1p8);
		regulator_put(lvs0_1p8);
	}
}

void  vt_camera_power_on (void)
{

	printk("%s: \n", __func__);

	gpio_set_value(GPIO_VT_OSC_EN, 0);

	/*VT Cam Power*/
	/* +1V8_VTCAM_IOVDD*/
	vt_lvs1_1p8 = regulator_get(NULL, "8901_lvs1");
	if (IS_ERR(vt_lvs1_1p8)) {
		pr_err("%s: 8901 LVS1 get failed\n", __func__);
		vt_lvs1_1p8 = NULL;
		return;
	}
	regulator_set_voltage(vt_lvs1_1p8, 1800000, 1800000);
	regulator_enable(vt_lvs1_1p8);

	/* +1V8_VTCAM_DVDD*/
	vt_lvs3_1p8 = regulator_get(NULL, "8901_lvs3");
	if (IS_ERR(vt_lvs3_1p8)) {
		pr_err("%s: 8901 LVS3 get failed\n", __func__);
		vt_lvs3_1p8 = NULL;
		return;
	}
	regulator_set_voltage(vt_lvs3_1p8, 1800000, 1800000);
	regulator_enable(vt_lvs3_1p8);

	/* +2V8_VTCAM_AVDD*/
	vt_ldo4_2p8 = regulator_get(NULL, "8901_l4");
	if (IS_ERR(vt_ldo4_2p8)) {
		pr_err("%s: 8901 VREG LDO4 get failed\n", __func__);
		vt_ldo4_2p8 = NULL;
		return;
	}
	regulator_set_voltage(vt_ldo4_2p8, 2800000, 2800000);
	regulator_enable(vt_ldo4_2p8);

	mdelay(1);

	gpio_set_value(GPIO_VT_OSC_EN, 1);

	/* +1V8_8MCAM_VIO*/
	lvs0_1p8 = regulator_get(NULL, "8058_lvs0");
	if (IS_ERR(lvs0_1p8)) {
		pr_err("%s: LVS0 get failed\n", __func__);
		lvs0_1p8 = NULL;
		return;
	}
	regulator_set_voltage(lvs0_1p8, 1800000, 1800000);
	regulator_enable(lvs0_1p8);

}

void  vt_camera_power_off (void)
{

	printk("%s: \n", __func__);

	gpio_set_value(GPIO_VT_OSC_EN, 0);

	if (vt_ldo4_2p8) {
		regulator_disable(vt_ldo4_2p8);
		regulator_put(vt_ldo4_2p8);
	}
	if (vt_lvs3_1p8) {
		regulator_disable(vt_lvs3_1p8);
		regulator_put(vt_lvs3_1p8);
	}
	if (vt_lvs1_1p8) {
		regulator_disable(vt_lvs1_1p8);
		regulator_put(vt_lvs1_1p8);
	}
	
	if (lvs0_1p8) {
		regulator_disable(lvs0_1p8);
		regulator_put(lvs0_1p8);
	}

}
#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 283115520,
		.ib  = 452984832,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 283115520,
		.ib  = 452984832,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 283115520,
		.ib  = 452984832,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 319610880,
		.ib  = 511377408,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 566231040,
		.ib  = 905969664,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 69984000,
		.ib  = 111974400,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 566231040,
		.ib  = 905969664,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 706199040,
		.ib  = 1129918464,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_stereo_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 212336640,
		.ib  = 339738624,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 25090560,
		.ib  = 40144896,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 239708160,
		.ib  = 383533056,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 79902720,
		.ib  = 127844352,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_stereo_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 300902400,
		.ib  = 481443840,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 230307840,
		.ib  = 368492544,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 245113344,
		.ib  = 392181351,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 106536960,
		.ib  = 170459136,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 106536960,
		.ib  = 170459136,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_video_vectors),
		cam_stereo_video_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_snapshot_vectors),
		cam_stereo_snapshot_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};
#endif


struct msm_camera_device_platform_data msm_camera_device_data_main_cam = {
	.camera_power_on   = main_camera_power_on,
	.camera_power_off  = main_camera_power_off,
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.csiphy = 0x04800000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_0_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 228570000,
#ifdef CONFIG_MSM_BUS_SCALING
	.cam_bus_scale_table = &cam_bus_client_pdata,
#endif
};

struct msm_camera_device_platform_data msm_camera_device_data_vt_cam = {
	.camera_power_on   	= vt_camera_power_on,
	.camera_power_off  	= vt_camera_power_off,
	.camera_gpio_on  = config_camera_on_gpios_vt_cam,
	.camera_gpio_off = config_camera_off_gpios_vt_cam,
	.ioext.csiphy = 0x04900000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = CSI_1_IRQ,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 228570000,
#ifdef CONFIG_MSM_BUS_SCALING
	.cam_bus_scale_table = &cam_bus_client_pdata,
#endif
};

struct resource lge_camera_resources[] = {
	{
		.start	= 0x04500000,
		.end	= 0x04500000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= VFE_IRQ,
		.end	= VFE_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

/*                                                                        
                         
                                                                        */
#ifdef CONFIG_LGE_FLASH_LM3559
struct led_flash_platform_data {
	int gpio_en;
};

static struct led_flash_platform_data lm3559_flash_pdata = {
	.gpio_en		= GPIO_LED_FLASH_EN,
};
	
static struct i2c_board_info cam_i2c_flash_info[] = {
	{
		I2C_BOARD_INFO("lm3559",LED_FLASH_I2C_SLAVE_ADDR),
		.type = "lm3559",
		.platform_data = &lm3559_flash_pdata,
	}
};
#endif

/*                                                                        
                                                                  
                                                                        */
#ifdef CONFIG_LGE_SENSOR_IMX105
static struct msm_camera_sensor_flash_data flash_imx105 = {
	.flash_type		= MSM_CAMERA_FLASH_LED,
};

static struct msm_camera_sensor_info msm_camera_sensor_imx105_data = {
	.sensor_name	= "imx105",
	.sensor_reset	= GPIO_8M_CAM_RESET_N,
	.sensor_pwd		= 85,
	.vcm_pwd		= GPIO_8M_CAM_VCM_EN,
	.vcm_enable		= 1,
	.pdata			= &msm_camera_device_data_main_cam,
	.resource		= lge_camera_resources,
	.num_resources	= ARRAY_SIZE(lge_camera_resources),
	.flash_data		= &flash_imx105,
	.csi_if			= 1
};
struct platform_device msm_camera_sensor_imx105 = {
	.name	= "msm_camera_imx105",
	.dev	= {
		.platform_data = &msm_camera_sensor_imx105_data,
	},
};
#endif
#ifdef CONFIG_LGE_SENSOR_MT9M114
static struct msm_camera_sensor_flash_data flash_mt9m114 = {
	.flash_type		= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9m114_data = {
	.sensor_name	= "mt9m114",
	.sensor_reset	= GPIO_VT_CAM_RESET_N,
	.sensor_pwd		= 85,
	.vcm_pwd		= 1,
	.vcm_enable		= 0,
	.pdata			= &msm_camera_device_data_vt_cam,
	.resource		= lge_camera_resources,
	.num_resources	= ARRAY_SIZE(lge_camera_resources),
	.flash_data		= &flash_mt9m114,
	.csi_if			= 1
};

struct platform_device msm_camera_sensor_mt9m114 = {
	.id 	= 0,
	.name	= "msm_camera_mt9m114",
	.dev	= {
		.platform_data = &msm_camera_sensor_mt9m114_data,
	},
};
#endif

static struct i2c_board_info lge_camera_boardinfo[] __initdata = {
	#ifdef CONFIG_LGE_SENSOR_IMX105
	{
		I2C_BOARD_INFO("imx105", CAM_I2C_SLAVE_ADDR),
	},
	#endif
	#ifdef CONFIG_LGE_SENSOR_MT9M114
	{
		I2C_BOARD_INFO("mt9m114", VT_CAM_I2C_SLAVE_ADDR),
	},
	#endif
};
#endif /*                 */

static struct platform_device *camera_devices[] __initdata = {
#ifdef CONFIG_LGE_SENSOR_IMX105
		&msm_camera_sensor_imx105,
#endif
#ifdef CONFIG_LGE_SENSOR_MT9M114
		&msm_camera_sensor_mt9m114,
#endif
};

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct i2c_registry camera_i2c_devices[] __initdata = {
#ifdef CONFIG_LGE_FLASH_LM3559 
		{
			I2C_SURF | I2C_FFA | I2C_FLUID,
			MSM_GSBI3_QUP_I2C_BUS_ID,
			cam_i2c_flash_info,
			ARRAY_SIZE(cam_i2c_flash_info),
		},
#endif
#ifdef CONFIG_LGE_CAMERA
	 	{
			I2C_SURF | I2C_FFA | I2C_FLUID,
			MSM_GSBI4_QUP_I2C_BUS_ID,
			lge_camera_boardinfo,
			ARRAY_SIZE(lge_camera_boardinfo),
		},
#endif
};

void __init i2c_register_camera_info(void){

	int i;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(camera_i2c_devices); ++i) {
		i2c_register_board_info(camera_i2c_devices[i].bus,
						camera_i2c_devices[i].info,
						camera_i2c_devices[i].len);
	}
}
#endif /*CONFIG_I2C*/

void __init lge_camera_init(void){
	platform_add_devices(camera_devices, ARRAY_SIZE(camera_devices));
}


