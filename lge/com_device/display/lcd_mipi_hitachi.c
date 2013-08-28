/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
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

#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "../../../drivers/video/msm/mdp4.h"
#include "lcd_mipi_hitachi.h"
#include <linux/gpio.h>

#define LCD_RESET_N	50
#define CABC FALSE

static struct msm_panel_common_pdata *mipi_hitachi_pdata;
static struct msm_fb_data_type *local_mfd;

static struct dsi_buf hitachi_tx_buf;
static struct dsi_buf hitachi_rx_buf;

#if 1 /*                                              */
static int mipi_hitachi_lcd_init(void);
#endif

//exit_sleep_mode
static char exit_sleep_mode[2] =  {0x11,0x00};

//Gamma setting
static char gamma_setting[25] = {
	0xC8, 0x00, 0x06, 0x0A, 0x0F, 0x14, 0x1F, 0x1F, 0x17, 0x12,
	0x0C, 0x09, 0x06, 0x00, 0x06, 0x0A, 0x0F, 0x14, 0x1F, 0x1F,
	0x17, 0x12, 0x0C, 0x09, 0x06
};

//set_display_on
static char set_address_mode[2] =  {0x36,0x00};
static char set_pixel_format[2] =  {0x3A,0x70};
static char set_display_on[2] =  {0x29,0x00};

//set_display_off
static char set_display_off[2] =  {0x28,0x00};

//enter_sleep_mode
static char enter_sleep_mode[2] =  {0x10,0x00};

//Colume inversion
//static char Panel_Driving_Setting_Colume_inversion [9] = {0xC1, 0x00, 0x50, 0x04, 0x22, 0x16, 0x08, 0x60, 0x01};

//2Dot inversion
//static char Panel_Driving_Setting_2Dot_inversion [9] = {0xC1, 0x00, 0x10, 0x04, 0x22, 0x16, 0x08, 0x60, 0x01};

#if CABC
//CABC
static char CABC_ON_Backlight_Control_1 [6] = {0xB8, 0x01, 0x1A, 0x18, 0x02, 0x40};
static char CABC_ON_Backlight_Control_2 [8] = {0xB9, 0x18, 0x00, 0x18, 0x18, 0x9F, 0x1F, 0x0F};

/*
static char CABC_ON_Backlight_Control_3_10 [25] = {
	0xBA, 0x00, 0x00, 0x0C, 0x12, 0xAC, 0x12, 0x6C, 0x12, 0x0C,
	0x12, 0x00, 0xDA, 0x6D, 0x03, 0xFF, 0xFF, 0x10, 0xD9, 0xE4,
	0xEE, 0xF7, 0xFF, 0x9F, 0x00
};
static char CABC_ON_Backlight_Control_3_20 [25] = {
	0xBA, 0x00, 0x00, 0x0C, 0x12, 0xAC, 0x12, 0x6C, 0x12, 0x0C,
	0x12, 0x00, 0xDA, 0x6D, 0x03, 0xFF, 0xFF, 0x10, 0xB3, 0xC9,
	0xDC, 0xEE, 0xFF, 0x9F, 0x00
};
*/
static char CABC_ON_Backlight_Control_3_30 [25] = {
	0xBA, 0x00, 0x00, 0x0C, 0x12, 0xAC, 0x12, 0x6C, 0x12, 0x0C,
	0x12, 0x00, 0xDA, 0x6D, 0x03, 0xFF, 0xFF, 0x10, 0x8C, 0xAA,
	0xC7, 0xE3, 0xFF, 0x9F, 0x00
};
/*
static char CABC_ON_Backlight_Control_3_40 [25] = {
	0xBA, 0x00, 0x00, 0x0C, 0x13, 0xAC, 0x13, 0x6C, 0x13, 0x0C,
	0x13, 0x00, 0xDA, 0x6D, 0x03, 0xFF, 0xFF, 0x10, 0x67, 0x89,
	0xAF, 0xD6, 0xFF, 0x9F, 0x00
};
static char CABC_ON_Backlight_Control_3_50 [25] = {
	0xBA, 0x00, 0x00, 0x0C, 0x14, 0xAC, 0x14, 0x6C, 0x14, 0x0C,
	0x14, 0x00, 0xDA, 0x6D, 0x03, 0xFF, 0xFF, 0x10, 0x37, 0x5A,
	0x87, 0xBD, 0xFF, 0x9F, 0x00
};
*/
static char CABC_OFF [6] = {0xB8, 0x00, 0x1A, 0x18, 0x02, 0x40};
#endif

//Manufacture_Command_Access
static char Manufacture_Command_Access_Start[2] = {0xB0, 0x04};
static char Manufacture_Command_Access_End[2] = {0xB0, 0x03};


static struct dsi_cmd_desc hitachi_power_on_set[] = {
	//exit_sleep_mode
	{DTYPE_DCS_WRITE, 1, 0, 0, 140, sizeof(exit_sleep_mode), exit_sleep_mode},
	//gamma_setting
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting),gamma_setting},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},
#if CABC
	//CABC_ON
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_1),CABC_ON_Backlight_Control_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_2),CABC_ON_Backlight_Control_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_3_30),CABC_ON_Backlight_Control_3_30},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},
#endif
	//Column inversion
//	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},
//	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Panel_Driving_Setting_Colume_inversion),Panel_Driving_Setting_Colume_inversion},
//	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},
	//set_display_on
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_address_mode),set_address_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_pixel_format),set_pixel_format},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_display_on),set_display_on},
#if 0
	//Refresh
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep_mode), exit_sleep_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_address_mode),set_address_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_pixel_format),set_pixel_format},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_1),CABC_ON_Backlight_Control_1},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_2),CABC_ON_Backlight_Control_2},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_3_50),CABC_ON_Backlight_Control_3_50},//CABC ON
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},//CABC ON
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},//Column inversion
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Panel_Driving_Setting_Colume_inversion),Panel_Driving_Setting_Colume_inversion},//Column inversion
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},//Column inversion
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_display_on),set_display_on},
#endif
};
/*
static struct dsi_cmd_desc hitachi_refresh_set[] = {
	//Refresh
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep_mode), exit_sleep_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_address_mode),set_address_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_pixel_format),set_pixel_format},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_1),CABC_ON_Backlight_Control_1},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_2),CABC_ON_Backlight_Control_2},//CABC ON
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON_Backlight_Control_3_50),CABC_ON_Backlight_Control_3_50},//CABC ON
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},//CABC ON
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},//Column inversion
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Panel_Driving_Setting_Colume_inversion),Panel_Driving_Setting_Colume_inversion},//Column inversion
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},//Column inversion
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_display_on),set_display_on},
};
*/
static struct dsi_cmd_desc hitachi_display_off_deep_standby_set[] = {
/*
	//2Dot inversion
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Panel_Driving_Setting_2Dot_inversion),Panel_Driving_Setting_2Dot_inversion},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},
*/
	//set_display_off
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_display_off),set_display_off},
#if CABC
	//CABC_Off
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_Start),Manufacture_Command_Access_Start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_OFF),CABC_OFF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(Manufacture_Command_Access_End),Manufacture_Command_Access_End},
#endif
	//enter_sleep_mode
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(enter_sleep_mode), enter_sleep_mode},
};

static bool lcd_reset_boot = true;
void mipi_hitachi_lcd_reset(void)
{
	gpio_tlmm_config(GPIO_CFG(LCD_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(LCD_RESET_N,1);
	mdelay(5);
	gpio_set_value(LCD_RESET_N,0);
	mdelay(5);
	if( lcd_reset_boot ) {
		mdelay(10);
	}
	gpio_set_value(LCD_RESET_N,1);
	mdelay(10);
	if( lcd_reset_boot ) {
		mdelay(30);
		lcd_reset_boot = false;
		printk("#### first boot 40ms delayed !! \n");
	}
}

static int mipi_hitachi_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	local_mfd = mfd;
	printk(KERN_INFO"%s: mipi hitachi lcd on started \n", __func__);
	mipi_hitachi_lcd_reset();
	//mipi_dsi_op_mode_config(DSI_CMD_MODE);

#if 0 /*                                         */
	mipi_dsi_cmds_tx(mfd, &hitachi_tx_buf, hitachi_power_on_set, ARRAY_SIZE(hitachi_power_on_set));
#else
	mipi_dsi_cmds_tx(&hitachi_tx_buf, hitachi_power_on_set, ARRAY_SIZE(hitachi_power_on_set));
#endif

	return 0;
}

static int mipi_hitachi_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO"%s: mipi hitachi lcd off started \n", __func__);

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

#if 0 /*                                         */
	mipi_dsi_cmds_tx(mfd, &hitachi_tx_buf, hitachi_display_off_deep_standby_set, ARRAY_SIZE(hitachi_display_off_deep_standby_set));
#else
	mipi_dsi_cmds_tx(&hitachi_tx_buf, hitachi_display_off_deep_standby_set, ARRAY_SIZE(hitachi_display_off_deep_standby_set));
#endif

	gpio_tlmm_config(GPIO_CFG(LCD_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	gpio_set_value(LCD_RESET_N,0);

	return 0;
}

static void mipi_hitachi_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level=(int)mfd->bl_level;
	mipi_hitachi_pdata->backlight_level(level, 0, 0);
}

ssize_t msm_fb_hitachi_show_onoff(struct device *dev,  struct device_attribute *attr, char *buf)
{
	printk("%s : start\n", __func__);
	return 0;
}

ssize_t msm_fb_hitachi_store_onoff(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int onoff;
	sscanf(buf, "%d", &onoff);
	if(onoff) {
		//mipi_dsi_op_mode_config(DSI_CMD_MODE);
		//mipi_dsi_cmds_tx(local_mfd, &hitachi_tx_buf, hitachi_power_on_set, ARRAY_SIZE(hitachi_power_on_set));
	}
	else {
		printk("%s: buf %s, off : %d\n", __func__,buf, onoff);
	}
	return 0;

}
DEVICE_ATTR(lcd_refresh, 0644, msm_fb_hitachi_show_onoff, msm_fb_hitachi_store_onoff);

static int mipi_hitachi_lcd_probe(struct platform_device *pdev)
{
	int err;
	if (pdev->id == 0) {
		mipi_hitachi_pdata = pdev->dev.platform_data;
		return 0;
	}

	printk(KERN_INFO"%s: mipi hitachi lcd probe start\n", __func__);

	msm_fb_add_device(pdev);
	err = device_create_file(&pdev->dev, &dev_attr_lcd_refresh);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_hitachi_lcd_probe,
	.driver = {
		.name   = "mipi_hitachi",
	},
};

static struct msm_fb_panel_data hitachi_panel_data = {
	.on		= mipi_hitachi_lcd_on,
	.off		= mipi_hitachi_lcd_off,
	.set_backlight = mipi_hitachi_set_backlight_board,
};

static int ch_used[3];

int mipi_hitachi_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

#if 1 /*                                              */
	ret = mipi_hitachi_lcd_init();
	if (ret) {
		pr_err("mipi_toshiba_lcd_init() failed with ret %u\n", ret);
		return ret;
	}
#endif

	pdev = platform_device_alloc("mipi_hitachi", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hitachi_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hitachi_panel_data,
		sizeof(hitachi_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_hitachi_lcd_init(void)
{
	mipi_dsi_buf_alloc(&hitachi_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hitachi_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

#if 0 /*                                                */
module_init(mipi_hitachi_lcd_init);
#endif
