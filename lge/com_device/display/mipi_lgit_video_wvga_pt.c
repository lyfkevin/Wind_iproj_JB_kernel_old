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
#if 0 /*                                                       */
#include "mipi_lgit.h"
#else
#include "lcd_mipi_lgit.h"
#endif

#define LGIT_IEF
static struct msm_panel_info pinfo;

//#define USE_HW_VSYNC

#define DSI_BIT_CLK_337MHZ

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 480*8000, RGB888, 2 Lane 65 fps cmd mode */
#if	defined(DSI_BIT_CLK_337MHZ)
	/* 480*800, RGB888, 2 Lane 54 fps cmd mode */
	/* regulator */
	{0x03, 0x01,0x01, 0x00},	/* Fixed values */
	/* timing */
	{0x66, 0x26, 0x15, 0x00, 0x17, 0x8C, 0x1E, 0x8B,
		0x17, 0x03, 0x04},
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},	/* Fixed values */
	/* strength */
	{0xee, 0x03, 0x86, 0x03},	/* Fixed values */
	/* pll control */
	{0x41, 0x50, 0x31, 0xDA, 0x00, 0x50, 0x48, 0x63,
		0x31, 0x0f, 0x07, 0x05, 0x14, 0x03, 0x03, 0x03,
		0x54, 0x06, 0x10, 0x04, 0x03 },
#endif
};

static int __init mipi_video_lgit_wvga_pt_init(void)
{
	int ret;
	printk(KERN_ERR "%s: start!\n", __func__);

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_lgit_wvga"))
		return 0;
#endif

	pinfo.xres = 480;
	pinfo.yres = 800;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

	pinfo.lcdc.h_back_porch = 20;
	pinfo.lcdc.h_front_porch = 16;
	pinfo.lcdc.h_pulse_width = 4;//1;//32;//10;
	pinfo.lcdc.v_back_porch = 22;
	pinfo.lcdc.v_front_porch = 8;
	pinfo.lcdc.v_pulse_width = 1;

	/*
	   pinfo.lcdc.h_back_porch = 64;//64;//80;//16;
	   pinfo.lcdc.h_front_porch = 2;//48;//16;
	   pinfo.lcdc.h_pulse_width = 1;//32;//10;
	   pinfo.lcdc.v_back_porch = 15;//2;//8;
	   pinfo.lcdc.v_front_porch = 5;//8;
	   pinfo.lcdc.v_pulse_width = 3;//5;
	 */

	pinfo.lcdc.border_clr = 0;			/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;

	//                                                                     
	pinfo.bl_max = 0x7F;//22;
	pinfo.bl_min = 15;//1;
	pinfo.fb_num = 2;

#ifdef USE_HW_VSYNC
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.mipi.te_sel = 1; /* TE from vsync gpio */
#endif
	pinfo.mipi.mode = DSI_VIDEO_MODE;

	pinfo.mipi.pulse_mode_hsa_he = FALSE;
#if defined(LGIT_IEF)
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
#else
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
#endif
	/*
	   pinfo.mipi.pulse_mode_hsa_he = FALSE;
	   pinfo.mipi.hfp_power_stop = FALSE;
	   pinfo.mipi.hbp_power_stop = FALSE;
	   pinfo.mipi.hsa_power_stop = FALSE;
	 */
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_BGR;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
#if defined(DSI_BIT_CLK_337MHZ)
	pinfo.mipi.t_clk_post = 0x22;
	pinfo.mipi.t_clk_pre = 0x33;

	pinfo.clk_rate = 337000000;
	pinfo.mipi.frame_rate = 65;
#endif
	pinfo.mipi.stream = 0;		/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	/*to maintain MIPI clock in HS mode always */
	pinfo.mipi.force_clk_lane_hs = 1;
	ret = mipi_lgit_device_register(&pinfo, MIPI_DSI_PRIM,
			MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_lgit_wvga_pt_init);
