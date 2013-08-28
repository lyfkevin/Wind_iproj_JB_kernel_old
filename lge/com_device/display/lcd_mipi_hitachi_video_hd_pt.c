/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
#include "lcd_mipi_hitachi.h"
#include <board_lge.h>

static struct msm_panel_info pinfo;

#define DSI_BIT_CLK_370MHZ	//darkwood

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps video mode */
#if defined(DSI_BIT_CLK_370MHZ)
	{0x03, 0x01, 0x01, 0x00},		/* regulator */
	{0x66, 0x26, 0x18, 0x00, 0x1A, 0x8F, 0x1E, 0x8C, 0x1A, 0x03, 0x04}, 	/* timing */
	{0x7f, 0x00, 0x00, 0x00},		/* phy ctrl */
	{0xee, 0x03, 0x86, 0x03},		/* strength */
	{0x41, 0x70, 0x31, 0xDA, 0x00, 0x50, 0x48, 0x63,
	 0x31, 0x0f, 0x03, 0x05, 0x14, 0x03, 0x03, 0x03,
	 0x54, 0x06, 0x10, 0x04, 0x03 },
#endif
};

static int __init mipi_video_hitachi_wvga_pt_init(void)
{
	int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_hitachi_wvga"))
		return 0;
#endif

	pinfo.xres = 768;		//ori 720
	pinfo.yres = 1024;	//ori 1280
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;

	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

#if defined(DSI_BIT_CLK_370MHZ)
	pinfo.lcdc.h_back_porch = 80;
	pinfo.lcdc.h_front_porch = 116;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 7;
	pinfo.lcdc.v_front_porch = 27;
	pinfo.lcdc.v_pulse_width = 2;
#endif

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
#if defined(CONFIG_MACH_LGE_325_BOARD_SKT) || defined(CONFIG_MACH_LGE_325_BOARD_LGU)
	if(lge_bd_rev >= LGE_REV_C){
#else
	if(lge_bd_rev >= LGE_REV_A){
#endif
		pinfo.bl_max = 0xFF;
	}
	else{
		pinfo.bl_max = 0x7F;
	}
	pinfo.bl_min = 0;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;

	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;

	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_BGR;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;

#if defined(DSI_BIT_CLK_370MHZ)	//darkwood
	pinfo.mipi.t_clk_post = 0x22;
	pinfo.mipi.t_clk_pre = 0x34;
	pinfo.clk_rate = 369390000;
	pinfo.mipi.frame_rate = 60;
#endif

	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;

	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	ret = mipi_hitachi_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}


module_init(mipi_video_hitachi_wvga_pt_init);